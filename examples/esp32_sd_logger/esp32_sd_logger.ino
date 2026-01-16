/**
 * ESP32 SD Logger - Example Firmware for mcp-arduino-server v1.0
 *
 * This sketch demonstrates the two-phase experiment protocol:
 * - Phase 1 (experiment): Write CSV data to SD card on START/STOP commands
 * - Phase 2 (collection): Export SD data over serial on EXPORT command
 *
 * Hardware Requirements:
 * - ESP32 development board
 * - SD card module (SPI interface)
 * - SD card (formatted as FAT32)
 *
 * Wiring (adjust pin definitions below as needed):
 * - SD_CS   -> GPIO 5
 * - SD_MOSI -> GPIO 23
 * - SD_MISO -> GPIO 19
 * - SD_SCK  -> GPIO 18
 *
 * Serial Commands:
 * - START run_id=YYYYMMDD_HHMMSS_runNNN  - Begin experiment recording
 * - STOP                                  - End experiment recording
 * - EXPORT run_id=YYYYMMDD_HHMMSS_runNNN - Export SD log over serial
 */

#include <SD.h>
#include <SPI.h>

// ========== Configuration ==========
#define SERIAL_BAUD 115200
#define SD_CS_PIN 5
#define SAMPLE_INTERVAL_MS 100  // Sample every 100ms during experiment

// ========== Global State ==========
String currentRunId = "";
bool experimentRunning = false;
File logFile;
unsigned long lastSampleTime = 0;
unsigned long sampleCount = 0;

// ========== Setup ==========
void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 3000) {
    delay(10);  // Wait for serial connection (max 3s)
  }

  Serial.println("ESP32 SD Logger v1.0");
  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("ERROR: SD card initialization failed!");
    Serial.println("Please check:");
    Serial.println("  - SD card is inserted");
    Serial.println("  - SD card is formatted as FAT32");
    Serial.println("  - Wiring connections are correct");
    while (1) {
      delay(1000);  // Halt on SD init failure
    }
  }

  Serial.println("SD card initialized successfully");
  Serial.print("SD card type: ");
  uint8_t cardType = SD.cardType();
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC"); break;
    case CARD_SD:   Serial.println("SD"); break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("UNKNOWN"); break;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD card size: %llu MB\n", cardSize);

  Serial.println("Ready. Waiting for commands...");
  Serial.println("Commands: START run_id=..., STOP, EXPORT run_id=...");
}

// ========== Main Loop ==========
void loop() {
  // Handle serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    handleCommand(command);
  }

  // Collect sensor data during experiment
  if (experimentRunning) {
    unsigned long now = millis();
    if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
      lastSampleTime = now;
      collectSample();
    }
  }
}

// ========== Command Handler ==========
void handleCommand(String cmd) {
  Serial.print("Received: ");
  Serial.println(cmd);

  if (cmd.startsWith("START")) {
    handleStart(cmd);
  } else if (cmd == "STOP") {
    handleStop();
  } else if (cmd.startsWith("EXPORT")) {
    handleExport(cmd);
  } else {
    Serial.println("ERROR: Unknown command");
  }
}

// ========== START Command ==========
void handleStart(String cmd) {
  if (experimentRunning) {
    Serial.println("ERROR: Experiment already running. Send STOP first.");
    return;
  }

  // Parse run_id from "START run_id=YYYYMMDD_HHMMSS_runNNN"
  int equalPos = cmd.indexOf('=');
  if (equalPos == -1) {
    Serial.println("ERROR: Missing run_id parameter. Format: START run_id=...");
    return;
  }

  currentRunId = cmd.substring(equalPos + 1);
  currentRunId.trim();

  if (currentRunId.length() == 0) {
    Serial.println("ERROR: Empty run_id");
    return;
  }

  // Create log file: /logs/{run_id}.csv
  String logPath = "/logs/" + currentRunId + ".csv";

  // Ensure /logs directory exists
  if (!SD.exists("/logs")) {
    SD.mkdir("/logs");
  }

  // Open log file for writing
  logFile = SD.open(logPath, FILE_WRITE);
  if (!logFile) {
    Serial.printf("ERROR: Failed to create log file: %s\n", logPath.c_str());
    currentRunId = "";
    return;
  }

  // Write CSV header
  logFile.println("timestamp_ms,sample_count,analog_a0,analog_a1,temperature_c");
  logFile.flush();

  // Start experiment
  experimentRunning = true;
  lastSampleTime = millis();
  sampleCount = 0;

  Serial.printf("OK: Experiment started (run_id=%s)\n", currentRunId.c_str());
  Serial.printf("Logging to: %s\n", logPath.c_str());
}

// ========== STOP Command ==========
void handleStop() {
  if (!experimentRunning) {
    Serial.println("ERROR: No experiment running");
    return;
  }

  // Close log file
  if (logFile) {
    logFile.flush();
    logFile.close();
  }

  Serial.printf("OK: Experiment stopped (run_id=%s, samples=%lu)\n",
                currentRunId.c_str(), sampleCount);

  // Reset state
  experimentRunning = false;
  currentRunId = "";
  sampleCount = 0;
}

// ========== EXPORT Command ==========
void handleExport(String cmd) {
  if (experimentRunning) {
    Serial.println("ERROR: Cannot export while experiment running. Send STOP first.");
    return;
  }

  // Parse run_id from "EXPORT run_id=YYYYMMDD_HHMMSS_runNNN"
  int equalPos = cmd.indexOf('=');
  if (equalPos == -1) {
    Serial.println("ERROR: Missing run_id parameter. Format: EXPORT run_id=...");
    return;
  }

  String exportRunId = cmd.substring(equalPos + 1);
  exportRunId.trim();

  if (exportRunId.length() == 0) {
    Serial.println("ERROR: Empty run_id");
    return;
  }

  String logPath = "/logs/" + exportRunId + ".csv";

  // Check if log file exists
  if (!SD.exists(logPath)) {
    Serial.printf("ERROR: Log file not found: %s\n", logPath.c_str());
    return;
  }

  // Open log file for reading
  File file = SD.open(logPath, FILE_READ);
  if (!file) {
    Serial.printf("ERROR: Failed to open log file: %s\n", logPath.c_str());
    return;
  }

  // Get file size
  size_t fileSize = file.size();

  // Send framed export data (protocol: docs/serial-export.md)
  // Use SIZE= prefix for binary mode transfer (more reliable)
  // IMPORTANT: Do NOT send BEGIN/END after SIZE= to avoid corrupting CSV data
  Serial.printf("SIZE=%u\n", fileSize);

  // Stream file contents as raw bytes (exactly fileSize bytes)
  size_t bytesWritten = 0;
  while (file.available() && bytesWritten < fileSize) {
    uint8_t b = file.read();
    Serial.write(b);
    bytesWritten++;
  }

  file.close();

  // Send confirmation AFTER the CSV data is complete
  Serial.println();  // Add newline after CSV data
  Serial.printf("OK: Exported %u bytes from %s\n", fileSize, logPath.c_str());
}

// ========== Data Collection ==========
void collectSample() {
  sampleCount++;
  unsigned long timestamp = millis();

  // Read analog sensors (example: A0, A1)
  // ESP32 ADC is 12-bit (0-4095), voltage range 0-3.3V
  int analogA0 = analogRead(36);  // GPIO36 = ADC1_CH0
  int analogA1 = analogRead(39);  // GPIO39 = ADC1_CH3

  // Simulate temperature reading (replace with real sensor)
  float temperature = 20.0 + (sampleCount % 100) * 0.1;  // Fake varying temperature

  // Write CSV row: timestamp_ms,sample_count,analog_a0,analog_a1,temperature_c
  logFile.printf("%lu,%lu,%d,%d,%.2f\n",
                 timestamp, sampleCount, analogA0, analogA1, temperature);

  // Flush periodically (every 10 samples) to ensure data is written to SD
  if (sampleCount % 10 == 0) {
    logFile.flush();
  }
}

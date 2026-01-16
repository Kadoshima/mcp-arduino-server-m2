# ESP32 SD Logger Example

This example demonstrates the two-phase experiment protocol for autonomous ESP32 data collection with the mcp-arduino-server v1.0 pipeline.

## Hardware Requirements

- **ESP32 development board** (ESP32-DevKitC, ESP32-WROOM, or similar)
- **SD card module** (SPI interface)
- **MicroSD card** (formatted as FAT32, 32GB or less recommended)
- **Breadboard and jumper wires**

## Wiring Diagram

Connect the SD card module to the ESP32 using SPI:

| SD Module Pin | ESP32 Pin | Notes                  |
|---------------|-----------|------------------------|
| CS            | GPIO 5    | Chip Select            |
| MOSI          | GPIO 23   | Master Out Slave In    |
| MISO          | GPIO 19   | Master In Slave Out    |
| SCK           | GPIO 18   | SPI Clock              |
| VCC           | 3.3V      | **Use 3.3V, not 5V!**  |
| GND           | GND       | Ground                 |

**Important**: Most SD card modules are **3.3V only**. Using 5V may damage the SD card or module.

## SD Card Preparation

1. Format the SD card as **FAT32** (not exFAT or NTFS)
2. Insert the SD card into the SD card module
3. Connect the module to ESP32 following the wiring diagram above

## Installation

1. Open the `esp32_sd_logger.ino` sketch in Arduino IDE or upload via `mcp-arduino-server`
2. Install required ESP32 board support:
   ```bash
   arduino-cli core install esp32:esp32
   ```
3. Select your ESP32 board:
   - Board: "ESP32 Dev Module" (or your specific board)
   - Upload Speed: 115200
   - Port: (your ESP32 serial port, e.g., COM7 on Windows or /dev/ttyUSB0 on Linux)
4. Upload the sketch

## Usage

### Serial Commands

The firmware accepts three commands over serial (115200 baud):

#### 1. START - Begin Experiment Recording
```
START run_id=20260116_120000_run001
```
- Creates a new log file: `/logs/{run_id}.csv`
- Begins collecting sensor data every 100ms
- Writes CSV with columns: `timestamp_ms,sample_count,analog_a0,analog_a1,temperature_c`

#### 2. STOP - End Experiment Recording
```
STOP
```
- Closes the current log file
- Stops data collection
- Reports total number of samples collected

#### 3. EXPORT - Transfer Log to PC
```
EXPORT run_id=20260116_120000_run001
```
- Reads the specified log file from SD card
- Sends framed data over serial (BEGIN_EXPORT...END_EXPORT)
- PC-side MCP server receives and saves to `artifacts/{run_id}/sd/log.csv`

### Manual Testing (Serial Monitor)

1. Open Serial Monitor at 115200 baud
2. Send: `START run_id=test001`
3. Wait 5-10 seconds (data is being collected)
4. Send: `STOP`
5. Send: `EXPORT run_id=test001`
6. Observe CSV data output between `BEGIN_EXPORT` and `END_EXPORT` markers

### Automated Testing (via MCP Server)

Use the `pipeline_run_and_collect` tool for a full automated experiment:

```json
{
  "run_id": "auto",
  "sketch_name": "esp32_sd_logger",
  "board_fqbn": "esp32:esp32:esp32",
  "port": "COM7",
  "baud": 115200,
  "experiment": {
    "start_cmd": "START run_id={run_id}",
    "stop_cmd": "STOP",
    "duration_sec": 30
  },
  "export": {
    "enabled": true,
    "export_cmd": "EXPORT run_id={run_id}",
    "timeout_sec": 120
  }
}
```

This will:
1. Compile and upload the sketch
2. Send START command
3. Wait 30 seconds
4. Send STOP command
5. Send EXPORT command and collect SD data to `artifacts/{run_id}/sd/log.csv`

## Customization

### Change Sampling Rate
Modify `SAMPLE_INTERVAL_MS` in the sketch (default: 100ms):
```cpp
#define SAMPLE_INTERVAL_MS 100  // Sample every 100ms
```

### Add Real Sensors
Replace the simulated data in `collectSample()`:
```cpp
// Example: Add DHT22 temperature/humidity sensor
#include <DHT.h>
DHT dht(4, DHT22);  // GPIO4

void collectSample() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  // Write to CSV...
}
```

### Change SD Card Pins
Modify pin definitions at the top of the sketch:
```cpp
#define SD_CS_PIN 5  // Change to your CS pin
// SPI pins (MOSI, MISO, SCK) are typically fixed on ESP32
```

## Troubleshooting

### SD Card Not Detected
- Check wiring connections (especially CS, MOSI, MISO, SCK)
- Ensure SD card is formatted as FAT32
- Try a different SD card (some cards are incompatible)
- Verify 3.3V power supply (not 5V)

### Export Times Out
- Increase timeout in pipeline config (`export.timeout_sec`)
- Check serial connection is stable
- Reduce log file size by decreasing experiment duration or sampling rate

### Data Not Saved
- Ensure `SD.mkdir("/logs")` succeeds on first START command
- Check SD card has sufficient free space
- Verify `logFile.flush()` is called periodically (every 10 samples by default)

## File Format

### CSV Output Format
```csv
timestamp_ms,sample_count,analog_a0,analog_a1,temperature_c
1234,1,512,1024,20.10
1334,2,515,1022,20.20
1434,3,518,1025,20.30
```

### Export Protocol Frame

**This firmware uses Binary Mode (SIZE= prefix) only:**

```
SIZE=1234
[exactly 1234 bytes of raw CSV data - no framing, no encoding]
OK: Exported 1234 bytes from /logs/20260116_120000_run001.csv
```

**Example output:**
```
SIZE=256
timestamp_ms,sample_count,analog_a0,analog_a1,temperature_c
1234,1,512,1024,20.10
1334,2,515,1022,20.20
[... more CSV rows totaling exactly 256 bytes ...]
OK: Exported 256 bytes from /logs/20260116_120000_run001.csv
```

**Important Notes:**
- The `SIZE=` line specifies the exact byte count of the CSV data that follows
- After `SIZE=`, send exactly that many bytes of raw CSV data (no BEGIN/END markers)
- Do NOT send `BEGIN`/`END` markers - they will corrupt the CSV data
- After the CSV data, send a newline and confirmation message
- This is the recommended mode for reliability and binary-safety

**Alternative Text Mode (not used in this firmware):**
The MCP server also supports a fallback Text Mode with `BEGIN`/`END` markers, but this ESP32 firmware does NOT use it. See `docs/serial-export.md` for details.

## License

MIT (same as mcp-arduino-server)

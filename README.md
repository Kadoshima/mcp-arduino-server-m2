# MCP Arduino Server (mcp-arduino-server)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PyPI version](https://img.shields.io/pypi/v/mcp-arduino-server.svg)](https://pypi.org/project/mcp-arduino-server/)

A FastMCP-powered bridge exposing `arduino-cli` functionality via the Model Context Protocol (MCP). Manage sketches, boards, libraries, files, plus generate WireViz schematics from YAML or natural language.

## Fork Scope (v1.0)

This fork targets autonomous experiment runs for ESP32: send START/STOP/EXPORT over serial, collect SD logs, run analysis, and upload artifacts, all via a single `pipeline_run_and_collect` tool. The documents in `docs/` define the requirements and tool contracts; implementation is complete.

## Docs

- `docs/README.md` - documentation index
- `docs/requirements-v1.md` - v1.0 requirements and constraints
- `docs/pipeline-run-and-collect.md` - pipeline tool specification
- `docs/artifacts.md` - artifact layout and manifest fields
- `docs/serial-export.md` - serial command and export protocol
- `docs/fork-todo.md` - implementation plan and minimum-diff order

## Requirements

- **Python ≥3.10**
- **arduino-cli** in `PATH`
- **MCP SDK** (`mcp[cli]`)
- **WireViz** (optional; for diagram generation)
- **OPENAI_API_KEY** (for AI‑powered WireViz)
- **thefuzz[speedup]** (optional; enables fuzzy local library search)

## Installation

**From PyPI**:
```bash
# Basic installation
pip install mcp-arduino-server

# With cloud upload support (S3-compatible storage)
pip install mcp-arduino-server[cloud]

# With all optional features
pip install mcp-arduino-server[all]
```

**From source**:
```bash
git clone https://github.com/Volt23/mcp-arduino-server.git
cd mcp-arduino-server
pip install .

# Or with cloud support
pip install .[cloud]
```

## Configuration

Environment variables override defaults:

| Variable             | Default / Description                              |
|----------------------|-----------------------------------------------------|
| ARDUINO_CLI_PATH     | auto-detected                                       |
| WIREVIZ_PATH         | auto-detected                                       |
| MCP_SKETCH_DIR       | `~/Documents/Arduino_MCP_Sketches/`                 |
| MCP_ARTIFACTS_DIR    | `./artifacts`                                       |
| LOG_LEVEL            | `INFO`                                              |
| OPENAI_API_KEY       | your OpenAI API key (required for AI‑powered WireViz)|
| OPENROUTER_API_KEY   | optional alternative to `OPENAI_API_KEY`            |
| ARDUINO_SERIAL_LOG_MAX_BYTES | max size per serial log file (bytes; 0 disables) |
| ARDUINO_SERIAL_LOG_ROTATE_COUNT | number of rotated serial logs to keep      |

## Quick Start

```bash
mcp-arduino-server
```

Server listens on STDIO for JSON-RPC MCP calls. Key methods:

### Sketches
- `create_new_sketch(name)`
- `list_sketches()`
- `read_file(path)`
- `write_file(path, content[, board_fqbn])` _(auto-compiles & opens `.ino`)_

### Build & Deploy
- `verify_code(sketch, board_fqbn)`
- `upload_sketch(sketch, port, board_fqbn)`

### Libraries
- `lib_search(name[, limit])`
- `lib_install(name)`
- `list_library_examples(name)`

### Boards
- `list_boards()`
- `board_search(query)`
- `auto_detect_port([prefer_fqbn])` _(auto-detect connected board port; useful for Windows)_

### Serial Monitor
- `serial_monitor_start(port, baud, buffer_lines, log_to_file, ...)`
- `serial_monitor_read(monitor_id, lines)`
- `serial_monitor_list()`
- `serial_monitor_stop(monitor_id)`

### Serial Control (v1.0)
- `serial_write(port, baud, data, append_newline=true)` _(send control commands to device)_
- `serial_request_export(run_id, port, baud)` _(request SD log export from device)_
- `serial_export_collect(run_id, port, baud, timeout_sec)` _(send EXPORT command + collect data + save to artifacts; useful for retry)_

### Pipeline & Automation (v1.0)
- `pipeline_run_and_collect(config_json)` _(full experiment cycle: compile → upload → run → export → analysis → cloud)_
- `analysis_run(script_path, args, timeout_sec, output_dir)` _(run Python analysis script via subprocess)_
- `cloud_upload(src_dir, provider, bucket, prefix)` _(upload artifacts to S3-compatible storage)_

### File Ops
- `rename_file(src, dest)`
- `remove_file(path)` _(destructive; operations sandboxed to home & sketch directories)_

### WireViz Diagrams
- `generate_circuit_diagram_from_description(desc, sketch="", output_base="circuit")` _(AI‑powered; requires `OPENAI_API_KEY`, opens PNG automatically)_

## MCP Client Configuration

To integrate with MCP clients (e.g., Claude Desktop), set your OpenAI API key in the environment (or alternatively `OPENROUTER_API_KEY` for OpenRouter):

```json
{
  "mcpServers": {
    "arduino": {
      "command": "/path/to/mcp-arduino-server",
      "args": [],
      "env": {
        "WIREVIZ_PATH": "/path/to/wireviz",
        "OPENAI_API_KEY": "<your-openai-api-key>"
      }
    }
  }
}
```

## Examples

### Example 1: Auto-Detect Port and Upload
```bash
# Auto-detect connected ESP32
auto_detect_port(prefer_fqbn="esp32:esp32:esp32")
# Returns: {"port": "COM7", "fqbn": "esp32:esp32:esp32", ...}

# Upload sketch
upload_sketch("MySketch", "COM7", "esp32:esp32:esp32")
```

### Example 2: Full Automated Experiment (Pipeline)
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
    "duration_sec": 60
  },
  "serial_capture": {
    "enabled": false
  },
  "export": {
    "enabled": true,
    "export_cmd": "EXPORT run_id={run_id}",
    "timeout_sec": 120
  },
  "analysis": {
    "enabled": true,
    "script_path": "scripts/analyze.py",
    "args": ["--input", "{artifact_dir}/sd", "--output", "{artifact_dir}/analysis"]
  },
  "cloud": {
    "enabled": false
  }
}
```

This pipeline will:
1. Generate a unique `run_id` (e.g., `20260116_120000_run001`)
2. Compile and upload `esp32_sd_logger` sketch
3. Send `START` command
4. Wait 60 seconds
5. Send `STOP` command
6. Send `EXPORT` command and collect SD data to `artifacts/{run_id}/sd/log.csv`
7. Run `scripts/analyze.py` with the collected data
8. Save all logs and outputs to `artifacts/{run_id}/`

### Example 3: Retry Failed Export
If the export step failed in the pipeline, you can retry it without re-running the experiment:
```bash
serial_export_collect(
  run_id="20260116_120000_run001",
  port="COM7",
  baud=115200,
  timeout_sec=120
)
```

## Testing

### Manual Testing with Real Hardware

The pipeline and export functionality requires real hardware testing:

1. **Hardware Setup**:
   - Connect an ESP32 board with SD card module (see `examples/esp32_sd_logger/README.md`)
   - Format SD card as FAT32
   - Upload the `esp32_sd_logger` sketch

2. **Test Export Protocol**:
   ```bash
   # Test manual START/STOP/EXPORT
   serial_write(port="COM7", baud=115200, data="START run_id=test001")
   # Wait 10 seconds
   serial_write(port="COM7", baud=115200, data="STOP")
   serial_export_collect(run_id="test001", port="COM7", baud=115200)
   ```

3. **Test Full Pipeline**:
   ```bash
   # Run complete automated experiment
   pipeline_run_and_collect(config_json={...})  # See Example 2 above
   ```

4. **Verify Exported Data**:
   - Check `artifacts/{run_id}/sd/log.csv` for valid CSV data
   - Ensure no corruption or truncation
   - Verify file size matches SIZE= header from device

### Known Limitations (Untested)

- **Export reliability**: CSV corruption during serial transfer has not been thoroughly tested at scale
- **Large files**: Export timeout may need tuning for log files >1MB
- **Concurrent experiments**: Running multiple experiments in parallel is untested
- **Error recovery**: Retry behavior for partial exports needs validation

## Security Notes

The MCP server implements several safety measures:

- **File Operations**: All file operations are sandboxed to allowed directories (home, sketch, Arduino data directories)
- **No Arbitrary Commands**: Only `arduino-cli` and `wireviz` commands are executed; no arbitrary shell commands
- **Destructive Operations**: `remove_file` and `rename_file` tools are disabled by default in the codebase
- **Path Validation**: All paths are validated and resolved before operations
- **Serial Port Access**: Requires explicit port specification; no automatic privilege escalation

## Troubleshooting

- Set `LOG_LEVEL=DEBUG` for verbose logs.
- Verify file and serial-port permissions.
- Install missing cores: `arduino-cli core install <spec>`.
- Run `arduino-cli` commands manually to debug.
- **COM port not found (Windows)**: Use `auto_detect_port()` to find the correct port automatically
- **Export timeout**: Increase `timeout_sec` in export config or check serial connection
- **SD card not detected**: Verify wiring and ensure SD card is formatted as FAT32 (see `examples/esp32_sd_logger/README.md`)

## License

MIT

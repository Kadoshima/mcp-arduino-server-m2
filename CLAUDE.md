# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is a FastMCP server that bridges `arduino-cli` functionality via the Model Context Protocol (MCP). The fork extends the upstream with autonomous experiment automation for ESP32 devices: send START/STOP/EXPORT over serial, collect SD logs, run analysis, and upload artifacts, all via a single `pipeline.run_and_collect` tool.

## Development Commands

### Installation and Setup
```bash
# Install from source
pip install .

# Run the MCP server (listens on STDIO for JSON-RPC MCP calls)
mcp-arduino-server

# Install from PyPI (for parity checks)
pip install mcp-arduino-server
```

### Testing
No automated test framework is configured yet. If you add tests:
- Use `pytest` and place them under `tests/` with `test_*.py` naming
- Run with: `pytest`
- Update this section with the command

### Debugging
```bash
# Enable debug logging
LOG_LEVEL=DEBUG mcp-arduino-server

# Verify arduino-cli is accessible
arduino-cli version

# Test arduino-cli commands manually
arduino-cli board list
arduino-cli core list
```

## Architecture

### Core Components

**Single-File Architecture**: All server logic lives in `src/mcp_arduino_server/server.py` (large file, ~3800+ lines). The file is organized into:
- Configuration constants and environment detection (lines ~145-340)
- MCP server initialization with FastMCP (line ~326)
- Helper functions prefixed with `_` for CLI commands, file operations, serial I/O
- MCP tool definitions decorated with `@mcp.tool()`
- Serial monitor state management via `SerialMonitorState` dataclass

**MCP Tools**: All tools are defined using FastMCP's `@mcp.tool()` decorator. The server exposes ~25 tools grouped into:
- Sketches: `create_new_sketch`, `list_sketches`, `read_file`, `write_file`
- Build/Deploy: `verify_code`, `upload_sketch`
- Libraries: `lib_search`, `lib_install`, `list_library_examples`
- Boards: `list_boards`, `board_search`
- Serial: `serial_monitor_start`, `serial_monitor_read`, `serial_monitor_list`, `serial_monitor_stop`, `serial_write`, `serial_request_export`
- File Ops: `rename_file`, `remove_file` (sandboxed to home/sketch directories)
- WireViz: `generate_circuit_diagram_from_description`, `generate_diagram_from_yaml`
- **Pipeline (v1.0 additions)**: `pipeline_run_and_collect`, `analysis_run`, `cloud_upload`

### Key Abstractions

**SerialMonitorState**: Manages serial monitor instances with auto-reconnect, buffering, and optional log rotation. Each monitor has a unique `monitor_id` (UUID).

**Artifact Management**: All experiment outputs are stored under `artifacts/{run_id}/` (path set by `ARTIFACTS_BASE_DIR`):
```
artifacts/{run_id}/
  manifest.json       # Run metadata, step results, timestamps
  compile.log         # Compilation output
  upload.log          # Upload output
  control.log         # START/STOP/EXPORT commands sent via serial.write
  serial.log          # Optional serial capture during experiment
  sd/log.csv          # Exported from device SD card
  analysis/...        # Analysis script outputs
```

**Pipeline Tool**: `pipeline_run_and_collect` orchestrates the full experiment workflow:
1. Generate/validate `run_id` and create artifact directory
2. Verify/compile sketch
3. Upload to device
4. Send START command, wait duration, send STOP
5. Optional serial capture
6. Send EXPORT and collect SD data
7. Run analysis script (Python subprocess)
8. Upload artifacts to cloud (S3-compatible)
9. Write final `manifest.json`

The pipeline returns step-by-step results with `next_action_hint` for retry guidance.

### Path Validation & Sandboxing

File operations are restricted to:
- User home directory (`Path.home()`)
- Sketch base directory (`SKETCHES_BASE_DIR`: `~/Documents/Arduino_MCP_Sketches/`)
- Arduino data directory (`ARDUINO_DATA_DIR`: `~/.arduino15` or platform equivalent)
- Arduino user directory (`ARDUINO_USER_DIR`: `~/Documents/Arduino/`)

The `_resolve_and_validate_path` function enforces these boundaries.

### Serial Export Protocol

Two-phase separation (see `docs/serial-export.md`):
- **Phase 1 (experiment)**: ESP32 writes CSV to SD card only
- **Phase 2 (collection)**: PC sends EXPORT command, device streams SD logs back

Commands are ASCII strings (typically newline-terminated):
- `START run_id={run_id}` - begin experiment recording
- `STOP` - end experiment recording
- `EXPORT run_id={run_id}` - begin SD log transfer

Export framing requires deterministic end detection (BEGIN/END markers or size-prefixed header).

## Configuration

### Environment Variables
| Variable | Default | Description |
|----------|---------|-------------|
| `ARDUINO_CLI_PATH` | auto-detected | Path to `arduino-cli` executable |
| `WIREVIZ_PATH` | auto-detected | Path to `wireviz` executable |
| `MCP_SKETCH_DIR` | `~/Documents/Arduino_MCP_Sketches/` | Base directory for sketches |
| `MCP_ARTIFACTS_DIR` | `./artifacts` | Base directory for experiment artifacts |
| `LOG_LEVEL` | `INFO` | Logging level (DEBUG, INFO, WARNING, ERROR) |
| `OPENAI_API_KEY` | - | Required for AI-powered WireViz diagram generation |
| `OPENROUTER_API_KEY` | - | Alternative to `OPENAI_API_KEY` for OpenRouter |
| `ARDUINO_SERIAL_LOG_MAX_BYTES` | 5MB | Max size per serial log file (0 disables rotation) |
| `ARDUINO_SERIAL_LOG_ROTATE_COUNT` | 3 | Number of rotated serial logs to keep |

### Platform Detection
- Windows: `ARDUINO_DATA_DIR = ~/AppData/Local/Arduino15`
- macOS/Linux: `ARDUINO_DATA_DIR = ~/.arduino15` (macOS overrides to `~/Library/Arduino15` if it exists)

### Default Constants
- `DEFAULT_FQBN`: `"arduino:avr:uno"` (used for auto-compile in `write_file`)
- `SERIAL_DEFAULT_BAUD`: `115200`
- `SERIAL_DEFAULT_BUFFER_LINES`: `2000`
- `FUZZY_SEARCH_THRESHOLD`: `75` (for library search fallback)

## Implementation Notes

### Adding New MCP Tools
1. Define an async function decorated with `@mcp.tool()`
2. Use type hints for parameters and return types
3. Include comprehensive docstring with description, parameters, returns, raises, examples
4. Log errors with `log.error()` and return user-friendly error messages
5. For file operations, use `_resolve_and_validate_path` for path validation
6. For CLI commands, use `_run_arduino_cli_command` helper
7. Consider auto-opening generated files with `_open_file_in_default_app` if appropriate

### Serial Monitor Implementation
- Each monitor runs in a dedicated thread (`_serial_monitor_reader_thread`)
- Auto-reconnect with exponential backoff (1s → 5s delays)
- Circular buffer (`deque`) for line storage with configurable max lines
- Optional log rotation for file output
- Port conflicts are handled by pausing existing monitors during upload

### Auto-Compile Behavior
`write_file` automatically compiles `.ino` files when written to the sketch directory using the provided `board_fqbn` parameter (defaults to `DEFAULT_FQBN`).

### WireViz Integration
Two approaches:
1. **AI-powered**: `generate_circuit_diagram_from_description` uses GPT-4 to convert natural language → WireViz YAML → PNG
2. **Direct**: `generate_diagram_from_yaml` accepts WireViz YAML directly

Both auto-open generated PNG files in the default image viewer.

## Fork-Specific Implementation Plan

See `docs/fork-todo.md` for the v1.0 implementation roadmap. Current status:
- ✅ `serial.write` - implemented
- ✅ `serial.request_export` - implemented
- ✅ Artifact directory structure - implemented
- ✅ `pipeline.run_and_collect` - implemented
- ✅ `analysis.run` - implemented
- ✅ `cloud.upload` - implemented

Key contracts defined in `docs/`:
- `requirements-v1.md` - v1.0 goals, constraints, and scope
- `pipeline-run-and-collect.md` - pipeline tool input/output schema and step order
- `artifacts.md` - artifact directory layout and manifest fields
- `serial-export.md` - serial command protocol and export framing

## Dependencies

**Required**:
- Python ≥3.10
- `arduino-cli` in PATH
- `mcp[cli]` (FastMCP SDK)

**Optional**:
- `wireviz` for circuit diagram generation
- `openai` for AI-powered WireViz (requires `OPENAI_API_KEY`)
- `thefuzz[speedup]` for faster fuzzy library search

## Common Patterns

### Error Handling
```python
try:
    result = await _run_arduino_cli_command(["board", "list"])
    # Parse and return result
except RuntimeError as e:
    log.error(f"Command failed: {e}")
    return f"Error: {e}"
```

### Path Resolution
```python
resolved_path = await _resolve_and_validate_path(
    user_input_path,
    must_exist=True,
    allow_write=False
)
```

### Async File Operations
```python
content = await _async_file_op(_sync_read_file, file_path)
await _async_file_op(_sync_write_file, file_path, content)
```

### CLI Command Execution
```python
stdout, stderr = await _run_arduino_cli_command(
    ["compile", "--fqbn", fqbn, str(sketch_path)]
)
```

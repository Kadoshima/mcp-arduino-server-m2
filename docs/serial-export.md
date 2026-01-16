# Serial Control and Export Protocol

## Two-Phase Contract
- Phase 1 (experiment): the device writes CSV to SD only.
- Phase 2 (collection): the PC requests export and retrieves SD logs.

## MCP Tool Contracts
### serial.write
Send ASCII control commands to the device.

Input:
- `port` (example: `COM7`)
- `baud` (example: `115200`)
- `data` (string or bytes)
- `append_newline` (bool, default `true`)

Output:
- `ok`
- `bytes_written`

### serial.request_export
Explicitly request export and guide the caller to the next read step.

Input:
- `run_id`
- `port`
- `baud`

Output:
- `ok`
- `hint` (example: `monitor_read_next`)

## Control Commands
Commands are ASCII strings, typically newline-terminated.
- `START run_id={run_id}`: begin experiment recording.
- `STOP`: end experiment recording.
- `EXPORT run_id={run_id}`: begin SD log transfer.

Example send sequence:
```
START run_id=20260115_2112_run001
STOP
EXPORT run_id=20260115_2112_run001
```

## Export Framing
The export stream must have a deterministic end. Two modes are supported:

### Binary Mode (SIZE= prefix - recommended)
```
SIZE=12345\n
[exactly 12345 bytes of raw CSV data]
```
- More reliable for large files and binary-safe
- PC reads exactly the specified number of bytes
- **CRITICAL**: Do NOT send `BEGIN`/`END` after `SIZE=` - they will corrupt the CSV data

### Text Mode (BEGIN/END markers - fallback)
```
BEGIN\n
[CSV data line by line]\nEND\n
```
- Used when SIZE= is not available or for debugging
- PC collects lines between BEGIN and END markers
- Less efficient for large files

**Important**: These modes are mutually exclusive. If `SIZE=` is sent, BEGIN/END must NOT be sent.

The PC side should not assume idle time is a reliable end condition.

## PC-Side Expectations
- Use `serial.write` for control commands (or `serial.request_export` for intent clarity).
- Read data via serial monitor or a dedicated serial read path.
- Save exported CSV to `artifacts/{run_id}/sd/log.csv`.
- Apply a timeout (default 120s) and return a retry hint on failure.

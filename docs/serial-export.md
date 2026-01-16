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
The export stream must have a deterministic end. Recommended options:
- `BEGIN` / `END` markers around CSV content, or
- a size-prefixed header (`SIZE=12345`) followed by raw bytes.

The PC side should not assume idle time is a reliable end condition.

## PC-Side Expectations
- Use `serial.write` for control commands (or `serial.request_export` for intent clarity).
- Read data via serial monitor or a dedicated serial read path.
- Save exported CSV to `artifacts/{run_id}/sd/log.csv`.
- Apply a timeout (default 120s) and return a retry hint on failure.

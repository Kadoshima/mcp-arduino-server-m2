# v1.0 Requirements (Volt23 Fork + Pipeline Extensions)

Last updated: 2026-01-15 21:12 JST

## 0. Goal
Enable an AI MCP client to complete one experiment run end-to-end in a single call: sketch setup, compile/upload, run control, SD log collection, analysis, and cloud upload. All outputs are organized per `run_id`.

## 1. Fixed Constraints
### 1.1 Phase Separation (Required)
- Phase 1 (experiment only): the ESP32 writes CSV to SD; no transfer or cloud upload.
- Phase 2 (collection only): the PC triggers export and retrieves SD logs.

### 1.2 No SD Removal (Required)
- The SD card is never removed by a human operator.

## 2. Reuse from Upstream (Volt23)
- Sketch creation/listing.
- File read/write/rename/remove (sandboxed).
- Verify/compile and upload.
- Serial monitor read and log capture.

## 3. Additions in v1.0 (Implementation Targets)
- Serial send for START/STOP/EXPORT commands.
- `pipeline.run_and_collect` to automate a full run.
- Artifacts stored under `artifacts/{run_id}`.
- `analysis.run` to invoke a Python script and save outputs.
- `cloud.upload` for S3-compatible targets (MinIO included).

## 4. New MCP Tools (v1.0)
### 4.1 `serial.write` (Required)
Purpose: send control commands to the device.

Input:
- `port` (example: `COM7`)
- `baud` (example: `115200`)
- `data` (string or bytes)
- `append_newline` (bool, default `true`)

Output:
- `ok`
- `bytes_written`

### 4.2 `serial.request_export` (Required)
Purpose: send `EXPORT run_id=...` and make intent explicit.

Input:
- `run_id`
- `port`
- `baud`

Output:
- `ok`
- `hint` (example: `monitor_read_next`)

### 4.3 `pipeline.run_and_collect` (Required)
Purpose: compile -> upload -> run -> collect -> analysis -> upload.
See `docs/pipeline-run-and-collect.md` for JSON schema and step order.

## 5. Artifacts and Manifest (Required)
- Outputs live under `artifacts/{run_id}`.
- `manifest.json` must include: `run_id`, `time_start`, `time_end`, sketch, fqbn, port, baud, export files, analysis outputs, and cloud prefix.
See `docs/artifacts.md`.

## 6. Non-Functional Requirements
- Reproducibility: all logs and outputs are stored per `run_id`.
- Safety: file operations stay within the sandbox; external commands are restricted.
- Failure policy: EXPORT may be retried without rerunning the experiment.

## 7. Out of Scope (v1.0)
- Auto-fix loops or self-correcting runs (v2).
- Direct Wi-Fi transfer from the device (v2).
- gRPC daemon or long-lived service architecture changes (v2).

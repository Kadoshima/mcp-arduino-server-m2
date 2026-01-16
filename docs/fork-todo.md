# Fork Implementation TODO (v1.0)

## Phase 0: Bootstrap
- Fork and clone the upstream repo.
- Start the MCP server and confirm it runs locally.
- Verify `arduino-cli` is reachable in `PATH`.
- Set `MCP_SKETCH_DIR` for the local environment.

## Phase 1: Design Alignment
- Confirm tool names: `serial.write`, `serial.request_export`, `pipeline.run_and_collect`.
- Finalize `artifacts/{run_id}` layout and `manifest.json` fields.
- Define the `run_id` format (recommended: `YYYYMMDD_HHMMSS_runNNN`).

## Phase 2: Serial Write (Must-Have)
- Add `serial.write(port, baud, data, append_newline=true)`.
- Log commands to `control.log` and return `bytes_written`.
- Use this for START/STOP/EXPORT commands.

## Phase 3: Artifacts (Must-Have)
- Implement `create_artifact_dir(run_id)` and manifest skeleton.
- Route compile/upload/serial logs into the run directory.

## Phase 4: Export (Core v1.0)
- Device firmware: implement `START`, `STOP`, and `EXPORT` handling.
- Export must be framed (BEGIN/END or size-prefixed).
- PC side: add an export receiver that saves `sd/log.csv`.
- Allow export-only retry without rerunning the experiment.
- Optional: expose export receive as a dedicated MCP tool if it helps callers.

## Phase 5: Analysis Runner
- Add `analysis.run(script_path, args, timeout)` via Python subprocess.
- Save stdout/stderr to `analysis.log` and outputs to `analysis/`.

## Phase 6: Cloud Upload
- Add `cloud.upload(src_dir, provider, bucket, prefix)` for S3-compatible targets.
- Record upload metadata in `manifest.json`.

## Phase 7: Pipeline Integration
- Add `pipeline.run_and_collect(config_json)` with fixed step order.
- Return step results and `next_action_hint`.

## Phase 8: Polish
- Optional COM auto-detection on Windows.
- Guard rails for external command execution.
- Update README and docs with real examples.

## Minimum-Diff Order (Recommended)
`serial.write` -> artifacts -> export -> pipeline -> analysis -> cloud

# pipeline.run_and_collect

## Purpose
Run a full experiment cycle in one call: compile, upload, START/STOP control, optional serial capture, EXPORT, analysis, and cloud upload.

## Input JSON (v1.0)
```json
{
  "run_id": "auto",
  "sketch_name": "MySketch",
  "sketch_path": "optional",
  "board_fqbn": "esp32:esp32:esp32",
  "port": "COM7",
  "baud": 115200,
  "experiment": {
    "start_cmd": "START run_id={run_id}",
    "stop_cmd": "STOP",
    "duration_sec": 30
  },
  "serial_capture": {
    "enabled": true,
    "capture_sec": 10,
    "buffer_lines": 5000,
    "log_to_file": true
  },
  "export": {
    "enabled": true,
    "export_cmd": "EXPORT run_id={run_id}",
    "timeout_sec": 120
  },
  "analysis": {
    "enabled": true,
    "script_path": "scripts/analyze.py",
    "args": [
      "--input",
      "{artifact_dir}/sd",
      "--output",
      "{artifact_dir}/analysis"
    ]
  },
  "cloud": {
    "enabled": true,
    "provider": "s3",
    "bucket": "bucket-name",
    "prefix": "experiments/{run_id}"
  }
}
```

Notes:
- If `run_id` is `auto`, generate a unique ID (recommended format: `YYYYMMDD_HHMMSS_runNNN`).
- `sketch_path` overrides `sketch_name` when provided.
- `{run_id}` and `{artifact_dir}` placeholders are expanded before execution.
- `serial_capture.log_to_file=true` writes to `artifacts/{run_id}/serial.log`.

## Step Order (Fixed)
1. Resolve `run_id` and create `artifact_dir`.
2. Verify/compile.
3. Upload.
4. Send START, wait `duration_sec`, send STOP.
5. Optional serial capture.
6. Send EXPORT and collect SD data.
7. Run analysis.
8. Upload artifacts.
9. Write final `manifest.json`.

## Output JSON (v1.0)
```json
{
  "status": "success|failure",
  "run_id": "20260115_2112_run001",
  "artifact_dir": "artifacts/20260115_2112_run001",
  "steps": [
    { "name": "compile", "ok": true },
    { "name": "upload", "ok": true },
    { "name": "run", "ok": true },
    { "name": "export", "ok": true, "files": ["sd/log.csv"] },
    { "name": "analysis", "ok": true },
    { "name": "cloud", "ok": true }
  ],
  "errors": [],
  "next_action_hint": "none|retry_export|check_port|reupload"
}
```

## Failure Handling
- EXPORT is the only step that may be retried without rerunning the experiment.
- `next_action_hint` should guide the caller (`retry_export`, `check_port`, `reupload`).

# Artifacts Layout

## Root
All run outputs are stored under:
```
artifacts/{run_id}/
```

## Expected Files
```
artifacts/{run_id}/
  manifest.json
  compile.log
  upload.log
  control.log
  serial.log        (optional)
  sd/log.csv        (exported from device)
  analysis/...
```

Notes:
- `control.log` records START/STOP/EXPORT commands sent via `serial.write`.
- `serial.log` is only present when serial capture is enabled.

## manifest.json (Minimum Fields)
- `run_id`, `time_start`, `time_end`
- `sketch`, `board_fqbn`, `port`, `baud`
- `export_files` (relative to `artifact_dir`)
- `analysis_outputs` (relative to `artifact_dir`)
- `cloud_prefix` (if uploaded)
- `status` and `errors` summary

## run_id Format
Use a sortable, unique pattern such as `YYYYMMDD_HHMMSS_runNNN`.

## Storage Guidance
Artifacts are generated outputs and should not be committed to git.

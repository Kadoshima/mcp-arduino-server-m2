# Repository Guidelines

## Project Structure & Module Organization
- `src/mcp_arduino_server/` contains the FastMCP server implementation. `server.py` holds the tools, CLI wiring, and runtime config.
- `docs/` tracks the fork requirements and design notes for the v1.0 automation pipeline.
- Runtime outputs are planned under `artifacts/{run_id}/` (generated data, not committed).
- Packaging metadata and the CLI entrypoint live in `pyproject.toml` (`mcp-arduino-server` -> `mcp_arduino_server.server:main`).
- No `tests/` directory is present today.

## Build, Test, and Development Commands
- `pip install .` installs the package from this repo for local development.
- `mcp-arduino-server` starts the MCP server on STDIO.
- `pip install mcp-arduino-server` installs the released package from PyPI for parity checks.
- There is no test runner configured yet; see Testing Guidelines.

## Configuration & Dependencies
- Required: Python >= 3.10, `arduino-cli` in `PATH`, and the MCP SDK (`mcp[cli]`).
- Optional: `wireviz` plus `OPENAI_API_KEY` for diagram generation, and `thefuzz[speedup]` for faster library search.
- Common env overrides: `ARDUINO_CLI_PATH`, `WIREVIZ_PATH`, `MCP_SKETCH_DIR`, `LOG_LEVEL`, `OPENAI_API_KEY`, `OPENROUTER_API_KEY`, `ARDUINO_SERIAL_LOG_MAX_BYTES`, `ARDUINO_SERIAL_LOG_ROTATE_COUNT`.

## Coding Style & Naming Conventions
- Python with 4-space indentation; keep imports grouped (stdlib, third-party, local).
- Use `snake_case` for functions/variables and `PascalCase` for classes.
- Favor explicit logging via the module logger (`log`) and descriptive error messages.
- Keep platform-sensitive paths in `pathlib.Path` and avoid hard-coded separators.

## Documentation Guidelines
- Keep requirement changes in `docs/` and update `docs/README.md` when adding new files.
- Document new MCP tools with input/output examples and failure modes.

## Testing Guidelines
- No automated test framework is configured (no `tool.pytest` in `pyproject.toml` and no `tests/` tree).
- If you add tests, prefer `pytest` and place them under `tests/` with `test_*.py` names, then document the command (e.g., `pytest`) in `README.md`.

## Commit & Pull Request Guidelines
- Commit history favors short, imperative summaries (e.g., "Add serial monitor tools"); documentation changes sometimes use a `docs:` prefix.
- Pull requests should include a concise summary, manual test notes (board/FQBN, port, OS), and any new env vars or dependency changes.

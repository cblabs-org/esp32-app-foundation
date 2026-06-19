# 04-MCP Tooling Priority

For this project (esp32-app-foundation), we use the `platformio` MCP server for all PlatformIO-related tasks. 

**Rule:**
When compiling, building, cleaning, uploading firmware/filesystem, or performing other PlatformIO operations, you **MUST** prioritize using the `platformio` MCP tools (e.g., `build_project`, `clean_project`, `upload_firmware`, `run_tests`) over running direct `pio` CLI commands in the terminal.

**Why:** 
This ensures the MCP server correctly manages the project state, locks, and environment across different agents. If you are delegating tasks to worker agents, you must explicitly instruct them to use these MCP tools rather than raw CLI commands.

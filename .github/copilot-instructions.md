<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->
- [x] Verify that the copilot-instructions.md file in the .github directory is created.

- [x] Clarify Project Requirements
- Summary: Two tank nodes (ESP32-C3 + Heltec) send level+battery over LoRa to a Heltec gateway that hosts a web UI with online/offline + time-to-empty and a setup page.

- [x] Scaffold the Project
- Summary: Added PlatformIO projects for `node-esp32c3`, `node-heltec`, and `gateway-heltec`, plus UI + docs.

- [x] Customize the Project
- Summary: Implemented gateway web API + WebSocket state push, config persistence, offline detection, and dashboard/setup UI.

- [ ] Install Required Extensions

- [x] Compile the Project
- Summary: `firmware/gateway-heltec` builds successfully with pinned Async WebServer deps.

- [ ] Create and Run Task

- [ ] Launch the Project

- [ ] Ensure Documentation is Complete

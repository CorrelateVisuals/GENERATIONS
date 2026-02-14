# Steam Integration

This document describes the minimal Steam integration added to GENERATIONS.

## Overview

The Steam integration provides:
- Detection of Steam runtime environment
- Steam Deck hardware detection with automatic resolution optimization
- Steam callback processing in the main loop
- Foundation for future Steam features (achievements, cloud saves, etc.)

## Implementation

### Files Added

- `src/platform/SteamIntegration.h` - Steam integration interface
- `src/platform/SteamIntegration.cpp` - Steam integration implementation
- `steam_appid.txt` - Steam app ID file (placeholder ID: 480)

### Files Modified

- `src/app/CapitalEngine.h` - Added Steam include
- `src/app/CapitalEngine.cpp` - Initialize Steam and run callbacks in main loop
- `src/platform/Window.cpp` - Optimize window resolution for Steam Deck

## Steam Deck Support

When running on Steam Deck, the application automatically:
- Detects the hardware via multiple methods:
  - `SteamDeck` environment variable
  - Gamescope compositor detection (`GAMESCOPE_DISPLAY` env var)
  - Hardware product name checking (Jupiter/Galileo codenames)
- Sets native resolution to 1280x800
- Logs the detection for debugging

## Usage

### Running on Steam

The application will automatically detect if it's running under Steam by checking for `SteamAppId` or `SteamGameId` environment variables.

### Running Standalone

Without Steam, the application runs normally without Steam features. The integration gracefully handles the absence of Steam.

### Testing Steam Deck Mode

To test Steam Deck detection without actual hardware:

```bash
export SteamDeck=1
./bin/CapitalEngine
```

Or:

```bash
export GAMESCOPE_DISPLAY=:1
./bin/CapitalEngine
```

## Future Enhancements

This minimal integration provides the foundation for:
- Steam achievements
- Steam cloud saves
- Steam overlay
- Steam Input API
- Steam Rich Presence
- Full Steamworks SDK integration

To enable full Steamworks features:
1. Download the Steamworks SDK
2. Link against libsteam_api.so (Linux) or steam_api64.lib (Windows)
3. Replace placeholder `steam_appid.txt` with actual app ID
4. Uncomment SteamAPI_Init() and SteamAPI_Shutdown() calls in SteamIntegration.cpp
5. Uncomment SteamAPI_RunCallbacks() call in run_callbacks()

## Configuration

### Steam App ID

Update `steam_appid.txt` with your actual Steam App ID when you have one. The current placeholder (480) is the Spacewar test app ID used for development.

## Notes

- The current implementation does not link against the Steamworks SDK
- It provides detection and foundation without external dependencies
- All Steam-specific features gracefully degrade when Steam is not present
- The integration follows the established architecture patterns (platform layer)

#include "SteamIntegration.h"
#include "core/Log.h"

#include <cstdlib>
#include <cstring>

namespace Steam {

Integration::Integration()
    : status(InitStatus::NotInitialized), steam_deck_detected(false) {
  Log::text("{ [-] }", "Initializing Steam integration");

  // Check if running under Steam
  const char *steam_app_id = std::getenv("SteamAppId");
  const char *steam_game_id = std::getenv("SteamGameId");

  if (!steam_app_id && !steam_game_id) {
    Log::text("{ [!] }", "Steam not detected - running in standalone mode");
    status = InitStatus::NotInitialized;
    return;
  }

  // Detect Steam Deck
  detect_steam_deck();

  // In a real implementation, this would call SteamAPI_Init()
  // For minimal integration, we just track that Steam is present
  status = InitStatus::InitSucceeded;

  if (steam_deck_detected) {
    Log::text("{ [*] }", "Running on Steam Deck - optimizations enabled");
  } else {
    Log::text("{ [*] }", "Steam integration initialized");
  }
}

Integration::~Integration() {
  if (status == InitStatus::InitSucceeded) {
    Log::text("{ [-] }", "Shutting down Steam integration");
    // In a real implementation, this would call SteamAPI_Shutdown()
  }
}

void Integration::detect_steam_deck() {
  steam_deck_detected = false;

  // Method 1: Check for Steam Deck specific environment variable
  const char *steam_deck_env = std::getenv("SteamDeck");
  if (steam_deck_env && std::strcmp(steam_deck_env, "1") == 0) {
    steam_deck_detected = true;
    return;
  }

  // Method 2: Check for gamescope (Steam Deck compositor)
  const char *gamescope_env = std::getenv("GAMESCOPE_DISPLAY");
  if (gamescope_env) {
    steam_deck_detected = true;
    return;
  }

#ifdef __linux__
  // Method 3: Check for Neptune hardware (Steam Deck codename)
  const char *check_commands[] = {
      "grep -q 'Jupiter' /sys/devices/virtual/dmi/id/product_name 2>/dev/null",
      "grep -q 'Galileo' /sys/devices/virtual/dmi/id/product_name 2>/dev/null"};

  for (const char *cmd : check_commands) {
    if (std::system(cmd) == 0) {
      steam_deck_detected = true;
      return;
    }
  }
#endif
}

void Integration::run_callbacks() {
  if (status != InitStatus::InitSucceeded) {
    return;
  }
  // In a real implementation, this would call SteamAPI_RunCallbacks()
  // This should be called periodically in the main loop
}

} // namespace Steam

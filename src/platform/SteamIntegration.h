#pragma once

namespace Steam {

// Steam initialization state
enum class InitStatus {
  NotInitialized,
  InitSucceeded,
  InitFailed
};

class Integration {
public:
  Integration();
  ~Integration();

  Integration(const Integration &) = delete;
  Integration &operator=(const Integration &) = delete;

  static Integration &get() {
    static Integration instance;
    return instance;
  }

  bool is_available() const {
    return status == InitStatus::InitSucceeded;
  }

  bool is_steam_deck() const {
    return steam_deck_detected;
  }

  void run_callbacks();

private:
  InitStatus status;
  bool steam_deck_detected;

  void detect_steam_deck();
};

} // namespace Steam

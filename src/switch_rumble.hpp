#ifdef __SWITCH__

#include <chrono>
#include <vector>

union HidVibrationDeviceHandle;

namespace sw {

// HD-rumble driven straight through libnx. We can't use SDL_JoystickRumble: the
// devkitPro SDL port only initializes vibration handles for HidNpadIdType_No1
// (player 1), never HidNpadIdType_Handheld -- so in handheld mode the send goes
// to a slot with no motor and nothing happens. We instead init both targets
// (two actuators each: left + right) and send to whichever npad is active.
// Unlike SDL, libnx vibration is set-and-hold, so we stop it ourselves once the
// server report's duration elapses (tick()).
struct SwitchRumble
{
    using time_point = std::chrono::steady_clock::time_point;

    //std::vector<HidVibrationDeviceHandle> m_handheld;
    std::vector<HidVibrationDeviceHandle> m_player1;
    bool m_active = false;
    bool m_ready = false;
    time_point m_expire;

    SwitchRumble(bool initialize = false);

    void init();

    // Handheld mode -> built-in rails; otherwise the player-1 controller.
    const HidVibrationDeviceHandle* target() const;

    void send(float low, float high);

    // Start a burst scaled by the user's intensity gain; auto-stops after
    // duration_ms (floored so a 0-length report is still felt, and the server
    // re-sends to sustain longer effects). The high band gets an extra trim --
    // it is inherently louder/harsher and is what you hear humming.
    void play(float low, float high, float gain, std::chrono::milliseconds duration, time_point now);

    void tick(time_point now);

    void stop();
};

} // namespace

#endif

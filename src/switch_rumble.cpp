#ifdef __SWITCH__

#include "switch_rumble.hpp"

#include <array>

#include <switch.h>

using namespace std::chrono_literals;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

namespace sw {

// HD-rumble driven straight through libnx. We can't use SDL_JoystickRumble: the
// devkitPro SDL port only initializes vibration handles for HidNpadIdType_No1
// (player 1), never HidNpadIdType_Handheld -- so in handheld mode the send goes
// to a slot with no motor and nothing happens. We instead init both targets
// (two actuators each: left + right) and send to whichever npad is active.
// Unlike SDL, libnx vibration is set-and-hold, so we stop it ourselves once the
// server report's duration elapses (tick()).
SwitchRumble::SwitchRumble(bool initialize)
{
    if (initialize) {
        init();
    }
}

void SwitchRumble::init()
 {
    // m_handheld.resize(2);    
    // const Result r1 = hidInitializeVibrationDevices(
    //     m_handheld.data(), m_handheld.size(), HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    
    m_player1.resize(2);
    const Result r2 = hidInitializeVibrationDevices(
        m_player1.data(), m_player1.size(), HidNpadIdType_No1, HidNpadStyleTag_NpadFullKey);
    
    m_ready = /*R_SUCCEEDED(r1) &&*/ R_SUCCEEDED(r2);
}

// Handheld mode -> built-in rails; otherwise the player-1 controller.
const HidVibrationDeviceHandle *SwitchRumble::target() const
{
    return /*hidGetNpadStyleSet(HidNpadIdType_Handheld) & HidNpadStyleTag_NpadHandheld ? m_handheld.data()
                                                                                     : */m_player1.data();
}

void SwitchRumble::send(float low, float high)
{
    if (!m_ready) { return; }

    auto clamp01 = [](float v) {
        return v < 0 ? 0.f : (v > 1 ? 1.f : v);
    };
    HidVibrationValue v[2];
    v[0].amp_low   = clamp01(low);
    v[0].freq_low  = 160.0f; // HD-rumble low-band centre
    v[0].amp_high  = clamp01(high);
    v[0].freq_high = 320.0f; // HD-rumble high-band centre
    v[1]           = v[0];   // both actuators together
    hidSendVibrationValues(target(), v, 2);
}

// Start a burst scaled by the user's intensity gain; auto-stops after
// duration_ms (floored so a 0-length report is still felt, and the server
// re-sends to sustain longer effects). The high band gets an extra trim --
// it is inherently louder/harsher and is what you hear humming.
void SwitchRumble::play(float low, float high, float gain, std::chrono::milliseconds duration, steady_clock::time_point now)
{
    if (!m_ready) {
        return;
    }

    float lo = low * gain;
    float hi = high * gain * 0.75f;
    send(lo, hi);
    m_active = lo > 0.0f || hi > 0.0f;
    m_expire = now + (duration.count() ? duration : 150ms);
}

void SwitchRumble::tick(steady_clock::time_point now)
{
    if (m_ready && m_active && (now - m_expire) >= 0ms)
    {
        send(0.0f, 0.0f);
        m_active = false;
    }
}

void SwitchRumble::stop()
{
    if (m_ready) {
        send(0.0f, 0.0f);
    }
    m_active = false;
}

} // namespace

#endif

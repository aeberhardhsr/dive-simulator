#pragma once

namespace Dive
{
enum class Mode { Exploration, Simulation, Hardcore };

// One simulation, with explicit rules shared by physics, interface and recording.
struct ModeRules
{
    bool LimitedGas, Physiology, Decompression, EquipmentFailures, PermanentHUD, AutomaticHints;
    static constexpr ModeRules For(Mode mode)
    {
        return mode==Mode::Exploration ? ModeRules{false,false,false,false,true,true} :
            ModeRules{true,true,true,mode==Mode::Hardcore,false,mode!=Mode::Hardcore};
    }
};
}

#include "DiveCelestialActor.h"

void ADiveCelestialActor::ApplyEphemerisPhase(float Phase)
{
    EphemerisPhase=Phase;bHasEphemerisPhase=true;SetMoonDiscAge(Phase);
}

void ADiveCelestialActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Native Tick can call UpdateBodiesMotion when the observer moves over
    // its 10 km threshold, even while the sequence clock is paused. That
    // evaluation writes its mean-period phase. Restore our Sun/Moon/observer
    // phase and matching moonlight before the frame is rendered.
    if(bHasEphemerisPhase) SetMoonDiscAge(EphemerisPhase);
}

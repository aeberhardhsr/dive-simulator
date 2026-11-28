#pragma once
#include "CoreMinimal.h"
#include "CelestialVaultDaySequenceActor.h"
#include "DiveCelestialActor.generated.h"

// Preserve the ephemeris phase after the native observer-recentering tick.
UCLASS()
class DIVESIM_API ADiveCelestialActor : public ACelestialVaultDaySequenceActor
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaSeconds) override;
    void ApplyEphemerisPhase(float Phase);
private:
    float EphemerisPhase=0;
    bool bHasEphemerisPhase=false;
};

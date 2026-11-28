#pragma once
#include "Commandlets/Commandlet.h"
#include "DiveLandscapeBakeCommandlet.generated.h"

/** Reproducible editor-only conversion of the verified height raster to Nanite tiles. */
UCLASS()
class DIVESIM_API UDiveLandscapeBakeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UDiveLandscapeBakeCommandlet();
    virtual int32 Main(const FString& Params) override;
};

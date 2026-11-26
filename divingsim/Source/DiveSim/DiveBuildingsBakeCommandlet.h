#pragma once
#include "Commandlets/Commandlet.h"
#include "DiveBuildingsBakeCommandlet.generated.h"

UCLASS()
class DIVESIM_API UDiveBuildingsBakeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UDiveBuildingsBakeCommandlet();
    virtual int32 Main(const FString& Params) override;
};

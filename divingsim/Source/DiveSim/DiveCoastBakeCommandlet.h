#pragma once
#include "Commandlets/Commandlet.h"
#include "DiveCoastBakeCommandlet.generated.h"

UCLASS()
class DIVESIM_API UDiveCoastBakeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UDiveCoastBakeCommandlet();
    virtual int32 Main(const FString& Params) override;
};

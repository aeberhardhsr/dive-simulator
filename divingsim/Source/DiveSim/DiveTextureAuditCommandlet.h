#pragma once
#include "Commandlets/Commandlet.h"
#include "DiveTextureAuditCommandlet.generated.h"

/** Read-only check of the actual platform formats used by the benthic maps. */
UCLASS()
class DIVESIM_API UDiveTextureAuditCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UDiveTextureAuditCommandlet();
    virtual int32 Main(const FString& Params) override;
};

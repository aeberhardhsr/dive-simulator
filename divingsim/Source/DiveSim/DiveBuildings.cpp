#include "DiveWorld.h"
#include "DiveBuildings.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"

void ADiveWorld::CreateBuildings()
{
    TArray<FDiveBuildingTile> tiles;FString error;int32 count=0;
    auto fail=[&](const FString& message)
    { bTerrainReady=false;Terrain.Error=message;UE_LOG(LogTemp,Error,TEXT("DIVE_BUILDINGS_FAIL %s"),*message); };
    if(!DiveBuildings::Read(tiles,count,error)) { fail(error);return; }
    auto* material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_Buildings.M_Buildings"));
    if(!material) { fail(TEXT("Gebaeudematerial fehlt: Content -OnlyBuildings."));return; }
    int64 triangles=0;
    for(const auto& tile:tiles)
    {
        auto* mesh=LoadObject<UStaticMesh>(nullptr,*tile.Asset);
        if(!mesh||mesh->GetNumTriangles(0)<=0||!mesh->GetBodySetup()||mesh->GetBodySetup()->CollisionTraceFlag!=CTF_UseComplexAsSimple)
        { fail(TEXT("Gebaeudemesh/Kollision fehlt: Tools/Build.ps1 -Target Buildings."));return; }
        auto* component=NewObject<UStaticMeshComponent>(this,*tile.Name());
        component->SetupAttachment(RootComponent);component->SetMobility(EComponentMobility::Static);
        component->SetRelativeLocation(tile.Origin);component->SetStaticMesh(mesh);
        component->SetMaterial(0,material);component->SetCollisionProfileName(TEXT("BlockAll"));
        component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);component->RegisterComponent();
        BuildingMeshes.Add(component);triangles+=mesh->GetNumTriangles(0);
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_BUILDINGS_READY buildings=%d cells=%d fallback_triangles=%lld"),count,BuildingMeshes.Num(),triangles);
}

bool ADiveWorld::ValidateBuildings(FString& Error) const
{
    TArray<FDiveBuildingTile> tiles;int32 count=0;
    if(!DiveBuildings::Read(tiles,count,Error)) return false;
    if(tiles.Num()!=BuildingMeshes.Num()) { Error=TEXT("Building coverage incomplete");return false; }
    for(int i=0;i<tiles.Num();++i)
    {
        auto* component=BuildingMeshes[i].Get();const auto* mesh=component?component->GetStaticMesh().Get():nullptr;
        if(!mesh||mesh->GetPathName()!=tiles[i].Asset||mesh->GetNumTriangles(0)<=0||
            !component->GetRelativeLocation().Equals(tiles[i].Origin,.01)||
            !component->GetComponentScale().Equals(FVector::OneVector)||
            !component->GetComponentRotation().IsNearlyZero()||
            component->GetCollisionEnabled()!=ECollisionEnabled::QueryOnly||
            component->GetCollisionResponseToChannel(ECC_Pawn)!=ECR_Block||
            !mesh->GetBodySetup()||mesh->GetBodySetup()->CollisionTraceFlag!=CTF_UseComplexAsSimple||
            !component->GetMaterial(0)||component->GetMaterial(0)->GetPathName()!=TEXT("/Game/Diving/Materials/M_Buildings.M_Buildings"))
        { Error=TEXT("Building geometry/registration/collision mismatch");return false; }
        const FVector center=tiles[i].Origin+tiles[i].Probe;
        FHitResult hit;FCollisionQueryParams query;query.bTraceComplex=true;
        if(!component->LineTraceComponent(hit,center+tiles[i].Normal*100,center-tiles[i].Normal*100,query))
        { Error=FString::Printf(TEXT("Building collision probe failed: %s"),*tiles[i].Name());return false; }
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_BUILDINGS_PASS buildings=%d cells=%d"),count,tiles.Num());
    Error.Empty();return true;
}

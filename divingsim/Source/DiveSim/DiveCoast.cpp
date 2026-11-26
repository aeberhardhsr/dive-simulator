#include "DiveWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void ADiveWorld::CreateCoast()
{
    FString text;TSharedPtr<FJsonObject> root;const TArray<TSharedPtr<FJsonValue>>* tiles=nullptr;FString hash;
    auto fail=[this](){bTerrainReady=false;Terrain.Error=TEXT("Ufermeshes fehlen oder sind veraltet: Tools/Build.ps1 -Target Coast.");UE_LOG(LogTemp,Error,TEXT("DIVE_COAST_FAIL %s"),*Terrain.Error);};
    if(!FFileHelper::LoadFileToString(text,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/Coast/meshes.json")))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),root)||!root||
       !root->TryGetStringField(TEXT("source_md5"),hash)||hash!=Terrain.CoastSourceHash||
       !root->TryGetArrayField(TEXT("tiles"),tiles)||tiles->Num()!=static_cast<int>(Terrain.Coast.Tiles.size())) { fail();return; }
    auto* material=ImageryMaterial(TEXT("M_ShorePhoto"),Terrain.LandscapeImagery);
    if(!material) { fail();return; }
    TSet<FIntPoint> seen;int64 triangles=0;
    for(const auto& value:*tiles)
    {
        const auto tile=value->AsObject();double west=0,south=0,count=0;FString path;
        if(!tile||!tile->TryGetNumberField(TEXT("west"),west)||!tile->TryGetNumberField(TEXT("south"),south)||
           !tile->TryGetNumberField(TEXT("triangles"),count)||!tile->TryGetStringField(TEXT("asset"),path)||
           !FMath::IsFinite(west)||!FMath::IsFinite(south)||!FMath::IsFinite(count)||count<0||count>32768||
           west<2720000||west>2750000||south<1210000||south>1230000||
           FMath::Fmod(west-4,128.)!=0||FMath::Fmod(south-4,128.)!=0) { fail();return; }
        const auto key=Dive::CoastGrid::Key(west,south);const FIntPoint cell(key.first,key.second);
        if(seen.Contains(cell)||Terrain.Coast.Tiles.find(key)==Terrain.Coast.Tiles.end()) { fail();return; }seen.Add(cell);
        if(count==0) { if(!path.IsEmpty()) { fail();return; }continue; }
        const FString name=FString::Printf(TEXT("SM_Coast_%d_%d"),key.first,key.second);
        if(path!=TEXT("/Game/Diving/Coast/")+name+TEXT(".")+name) { fail();return; }
        auto* mesh=LoadObject<UStaticMesh>(nullptr,*path);
        if(!mesh||mesh->GetNumTriangles(0)!=count||!mesh->GetBodySetup()||mesh->GetBodySetup()->CollisionTraceFlag!=CTF_UseComplexAsSimple) { fail();return; }
        auto* component=NewObject<UStaticMeshComponent>(this,*name);
        component->SetupAttachment(RootComponent);component->SetMobility(EComponentMobility::Static);
        FVector origin=Terrain.ToWorld(west,south+128);origin.Z=0;
        component->SetRelativeLocation(origin);component->SetStaticMesh(mesh);component->SetMaterial(0,material);
        component->SetCollisionProfileName(TEXT("BlockAll"));component->RegisterComponent();CoastMeshes.Add(component);triangles+=static_cast<int64>(count);
        if(bOpticsSmoke)
        {
            double x=0,y=0,z=0;float expected=0;
            if(!tile->TryGetNumberField(TEXT("probe_x_cm"),x)||!tile->TryGetNumberField(TEXT("probe_y_cm"),y)||
               !tile->TryGetNumberField(TEXT("probe_z_cm"),z)||!FMath::IsFinite(x)||!FMath::IsFinite(y)||!FMath::IsFinite(z)||
               !Terrain.SampleGround(x,y,expected)||FMath::Abs(expected-z)>.1) { fail();return; }
            FHitResult hit;FCollisionQueryParams params(SCENE_QUERY_STAT(CoastProbe),true);
            if(!component->LineTraceComponent(hit,FVector(x,y,z+10000),FVector(x,y,z-10000),params)||FMath::Abs(hit.ImpactPoint.Z-z)>2)
            { UE_LOG(LogTemp,Error,TEXT("DIVE_COAST_COLLISION_FAIL %s expected=%.4f hit=%.4f"),*name,z,hit.ImpactPoint.Z);fail();return; }
        }
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_READY tiles=%d assets=%d triangles=%lld"),tiles->Num(),CoastMeshes.Num(),triangles);
    if(bOpticsSmoke) UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_COLLISION_PASS probes=%d"),CoastMeshes.Num());
}

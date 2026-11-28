#include "DiveWorld.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void ADiveWorld::CreateLawn()
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DiveLawnStudy"))) return;
    auto fail=[this]() {bTerrainReady=false;Terrain.Error=TEXT("Wiesendaten ungueltig.");UE_LOG(LogTemp,Error,TEXT("DIVE_LAWN_FAIL"));};
    if(!FParse::Param(FCommandLine::Get(),TEXT("DiveShoreStones"))) {fail();return;}
    FString text,crs,site;double schema=0;TSharedPtr<FJsonObject> root;const TArray<TSharedPtr<FJsonValue>>* candidates=nullptr;
    if(!FFileHelper::LoadFileToString(text,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/lawn.json")))||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),root)||!root||
        !root->TryGetNumberField(TEXT("schema"),schema)||schema!=1||!root->TryGetStringField(TEXT("crs"),crs)||crs!=TEXT("EPSG:2056")||
        !root->TryGetStringField(TEXT("site"),site)||site!=TEXT("weesen_strandbad")||
        !root->TryGetArrayField(TEXT("candidates"),candidates)||candidates->Num()<1||candidates->Num()>10000) {fail();return;}
    auto* mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Diving/Vegetation/Meshes/SM_LawnGrass.SM_LawnGrass"));
    if(!mesh||mesh->GetNumLODs()!=3||mesh->GetNumTriangles(0)!=960||mesh->GetBounds().BoxExtent.Z>10) {fail();return;}
    TArray<FTransform> transforms;int excluded=0;double minimum=DBL_MAX,minRootGap=DBL_MAX,maxRootGap=-DBL_MAX;
    for(const auto& value:*candidates)
    {
        auto item=value->AsObject();double east=0,north=0,yaw=0,scale=0;
        if(!item||!item->TryGetNumberField(TEXT("east"),east)||!item->TryGetNumberField(TEXT("north"),north)||
            !item->TryGetNumberField(TEXT("yaw"),yaw)||!item->TryGetNumberField(TEXT("scale"),scale)||
            !FMath::IsFinite(east)||!FMath::IsFinite(north)||!FMath::IsFinite(yaw)||!FMath::IsFinite(scale)||
            east<2727675||east>2727770||north<1222290||north>1222330||yaw<0||yaw>360||scale<.8||scale>1.2) {fail();return;}
        FVector p=Terrain.ToWorld(east,north);bool blocked=false;
        for(const auto& access:Accesses)
        {
            const FVector d=p-access.Origin;const double along=FVector::DotProduct(d,access.Forward);
            const double across=FMath::Abs(d.X*access.Forward.Y-d.Y*access.Forward.X);
            // The landing starts 1m behind the origin and is 1.8m wide.
            // Reserve the actual platform plus the tuft radius, not a 7m lawn gap.
            if(along>-140&&along<access.Length+40&&across<130) {blocked=true;break;}
        }
        float z=0,n=0,s=0,e=0,w=0;
        if(blocked||!Terrain.SampleGround(p.X,p.Y,z)||z<120||z>1400||
            !Terrain.SampleGround(p.X+30,p.Y,n)||!Terrain.SampleGround(p.X-30,p.Y,s)||
            !Terrain.SampleGround(p.X,p.Y+30,e)||!Terrain.SampleGround(p.X,p.Y-30,w)) {++excluded;continue;}
        const FVector normal=FVector(-(n-s)/60,-(e-w)/60,1).GetSafeNormal();
        if(normal.Z<.7) {++excluded;continue;}
        const FRotator rotation=FRotationMatrix::MakeFromZX(normal,FRotator(0,yaw,0).Vector()).Rotator();
        p.Z=z-1;
        double lowest=DBL_MAX,highest=-DBL_MAX;bool valid=true;
        for(int x=-1;x<=1;++x) for(int y=-1;y<=1;++y)
        {
            const FVector rootPoint=p+rotation.RotateVector(FVector(x*24*scale,y*24*scale,0));float ground=0;
            if(!Terrain.SampleGround(rootPoint.X,rootPoint.Y,ground)) {valid=false;continue;}
            lowest=FMath::Min(lowest,rootPoint.Z-ground);highest=FMath::Max(highest,rootPoint.Z-ground);
        }
        const double lower=FMath::Max(0.,highest-.5);
        if(!valid||lowest-lower<-4) {++excluded;continue;}
        p.Z-=lower;minRootGap=FMath::Min(minRootGap,lowest-lower);maxRootGap=FMath::Max(maxRootGap,highest-lower);
        transforms.Add(FTransform(rotation,p,FVector(scale)));minimum=FMath::Min(minimum,static_cast<double>(z));
    }
    if(transforms.Num()<100) {fail();return;}
    auto* component=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
    component->SetupAttachment(RootComponent);component->SetMobility(EComponentMobility::Static);
    component->SetStaticMesh(mesh);component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    component->SetCullDistances(4500,6000);component->bAutoRebuildTreeOnInstanceChanges=false;
    component->RegisterComponent();component->AddInstances(transforms,false,false,false);component->BuildTreeIfOutdated(false,true);
    LawnGrass=component;
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAWN_READY authored=%d placed=%d excluded=%d minimum_ground_cm=%.3f lods=3 collision=0"),candidates->Num(),transforms.Num(),excluded,minimum);
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAWN_ROOTS_PASS probes=%d min_gap_cm=%.3f max_gap_cm=%.3f"),transforms.Num()*9,minRootGap,maxRootGap);
}

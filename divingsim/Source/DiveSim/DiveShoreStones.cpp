#include "DiveWorld.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

void ADiveWorld::CreateShoreStones()
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DiveShoreStones"))) return;
    auto fail=[this]() { bTerrainReady=false;Terrain.Error=TEXT("Ufersteindaten sind ungueltig.");UE_LOG(LogTemp,Error,TEXT("DIVE_SHORE_STONES_FAIL")); };
    FString text;TSharedPtr<FJsonObject> root;const TArray<TSharedPtr<FJsonValue>>* stones=nullptr;
    double schema=0;FString crs,site;
    if(!FFileHelper::LoadFileToString(text,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/shore_stones.json")))||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),root)||!root||
        !root->TryGetNumberField(TEXT("schema"),schema)||schema!=1||!root->TryGetStringField(TEXT("crs"),crs)||crs!=TEXT("EPSG:2056")||
        !root->TryGetStringField(TEXT("site"),site)||site!=TEXT("weesen_strandbad")||
        !root->TryGetArrayField(TEXT("stones"),stones)||stones->Num()<1||stones->Num()>1000)
    { fail();return; }
    if(!FParse::Param(FCommandLine::Get(),TEXT("DiveNaturalGround"))) {fail();return;}
    TArray<FTransform> transforms[3];UStaticMesh* meshes[3];UMaterialInterface* materials[3];
    for(int i=0;i<3;++i)
    {
        const FString path=FString::Printf(TEXT("/Game/Diving/Rocks/SM_LakeRock%d.SM_LakeRock%d"),i+1,i+1);
        meshes[i]=LoadObject<UStaticMesh>(nullptr,*path);
        const FString materialPath=FString::Printf(TEXT("/Game/Diving/Materials/M_ShoreBankRock%d.M_ShoreBankRock%d"),i+1,i+1);
        materials[i]=LoadObject<UMaterialInterface>(nullptr,*materialPath);
        if(!meshes[i]||!meshes[i]->GetBodySetup()||!materials[i]) { fail();return; }
    }
    int wet=0,accessSkipped=0,placed=0;double minimumZ=DBL_MAX;
    for(const auto& value:*stones)
    {
        const auto item=value->AsObject();double east=0,north=0,width=0,length=0,height=0,yaw=0,variant=0;
        if(!item||!item->TryGetNumberField(TEXT("east"),east)||!item->TryGetNumberField(TEXT("north"),north)||
            !item->TryGetNumberField(TEXT("width_m"),width)||!item->TryGetNumberField(TEXT("length_m"),length)||
            !item->TryGetNumberField(TEXT("height_m"),height)||!item->TryGetNumberField(TEXT("yaw"),yaw)||
            !item->TryGetNumberField(TEXT("variant"),variant)||
            !FMath::IsFinite(east)||!FMath::IsFinite(north)||!FMath::IsFinite(width)||!FMath::IsFinite(length)||
            !FMath::IsFinite(height)||!FMath::IsFinite(yaw)||!FMath::IsFinite(variant)||
            east<2727675||east>2727770||north<1222290||north>1222320||
            width<.4||width>1.5||length<.4||length>1.5||height<.2||height>1||yaw<0||yaw>360||variant<1||variant>3||variant!=FMath::FloorToDouble(variant))
        { fail();return; }
        const int model=static_cast<int>(variant)-1;
        FVector location=Terrain.ToWorld(east,north);
        const auto bounds=meshes[model]->GetBounds();
        const FVector scale(length*100/(bounds.BoxExtent.X*2),width*100/(bounds.BoxExtent.Y*2),height*100/(bounds.BoxExtent.Z*2));
        const double radius=FMath::Sqrt(length*length+width*width)*50;
        bool blocked=false;
        for(const auto& access:Accesses)
        {
            const FVector delta=location-access.Origin;
            const double along=FVector::DotProduct(delta,access.Forward);
            const double across=FMath::Abs(delta.X*access.Forward.Y-delta.Y*access.Forward.X);
            if(along>-350-radius&&along<access.Length+350+radius&&across<300+radius) {blocked=true;break;}
        }
        if(blocked) {++accessSkipped;continue;}
        float centre=0,n=0,s=0,e=0,w=0;
        if(!Terrain.SampleGround(location.X,location.Y,centre)||
            !Terrain.SampleGround(location.X+100,location.Y,n)||!Terrain.SampleGround(location.X-100,location.Y,s)||
            !Terrain.SampleGround(location.X,location.Y+100,e)||!Terrain.SampleGround(location.X,location.Y-100,w)) {++wet;continue;}
        const FVector normal=FVector(-(n-s)/200,-(e-w)/200,1).GetSafeNormal();
        const FRotator rotation=FRotationMatrix::MakeFromZX(normal,FRotator(0,yaw,0).Vector()).Rotator();
        // Align the embedded stone with the slope instead of burying almost
        // its whole height at the downhill corner of a steep footprint.
        double low=DBL_MAX;bool valid=true;
        for(int x=-1;x<=1;++x) for(int y=-1;y<=1;++y)
        {
            const FVector p=location+rotation.RotateVector(FVector(x*length*50,y*width*50,0));float ground=0;
            if(!Terrain.SampleGround(p.X,p.Y,ground)) {valid=false;continue;}
            low=FMath::Min(low,static_cast<double>(ground));
        }
        location.Z=centre;
        location+=normal*(height*10)-rotation.RotateVector(bounds.Origin*scale);
        const FTransform transform(rotation,location,scale);
        const double bottom=meshes[model]->GetBoundingBox().TransformBy(transform).Min.Z;
        if(!valid||bottom<5||low<5||low>1000) {++wet;continue;}
        transforms[model].Add(transform);++placed;minimumZ=FMath::Min(minimumZ,bottom);
        UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_STONE east=%.4f north=%.4f variant=%d bottom_cm=%.3f terrain_min_cm=%.3f height_cm=%.3f"),east,north,model+1,bottom,low,height*100);
    }
    if(placed<10) {fail();return;}
    int collisionProbes=0;
    for(int i=0;i<3;++i)
    {
        auto* component=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        component->SetupAttachment(RootComponent);component->SetMobility(EComponentMobility::Static);
        component->SetStaticMesh(meshes[i]);component->SetCollisionProfileName(TEXT("BlockAll"));
        component->SetMaterial(0,materials[i]);
        component->SetCullDistances(18000,22000);component->bAutoRebuildTreeOnInstanceChanges=false;
        component->RegisterComponent();component->AddInstances(transforms[i],false,false,false);
        component->BuildTreeIfOutdated(false,true);ShoreStones.Add(component);
        for(const auto& transform:transforms[i])
        {
            const auto box=meshes[i]->GetBoundingBox().TransformBy(transform);
            const FVector centre=box.GetCenter();FHitResult hit;
            if(!component->LineTraceComponent(hit,FVector(centre.X,centre.Y,box.Max.Z+10),FVector(centre.X,centre.Y,box.Min.Z-10),FCollisionQueryParams(SCENE_QUERY_STAT(ShoreStoneProbe),false)))
            {fail();return;}
            ++collisionProbes;
        }
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_STONES_READY authored=%d placed=%d wet_rejected=%d access_rejected=%d minimum_z_cm=%.3f collision=1"),stones->Num(),placed,wet,accessSkipped,minimumZ);
    UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_STONES_COLLISION_PASS probes=%d"),collisionProbes);
}

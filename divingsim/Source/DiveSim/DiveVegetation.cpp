#include "DiveVegetation.h"
#include "DiveWorld.h"
#include "DivePawn.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/PlayerController.h"
#include "StaticMeshResources.h"
#include "ProceduralMeshComponent.h"

namespace
{
constexpr double CellWidth=25600;
constexpr int Radius=5;
FIntPoint CellAt(const FVector& P) { return FIntPoint(FMath::FloorToInt32(P.X/CellWidth),FMath::FloorToInt32(P.Y/CellWidth)); }
const TCHAR* Names[]={TEXT("SM_Beech"),TEXT("SM_FirA"),TEXT("SM_FirB"),TEXT("SM_FirC"),TEXT("SM_LeafTree")};
int Model(const FDiveTree& Tree)
{
    if(Tree.Height<8||Tree.Kind==6||Tree.Kind==14) return 4;
    // Authored mixture, not a species attribute in the topographic source.
    const int conifers=Tree.NativeGround+419>1100?70:25;
    return static_cast<int>(Tree.Seed%100)<conifers?1+static_cast<int>((Tree.Seed/100)%3):0;
}
}

void ADiveWorld::CreateVegetation()
{
    auto fail=[this](const TCHAR* message)
    { bTerrainReady=false;Terrain.Error=message;UE_LOG(LogTemp,Error,TEXT("DIVE_VEGETATION_FAIL %s"),message); };
    const FString folder=FPaths::ProjectContentDir()/TEXT("Diving/Geodata");
    FString text;TSharedPtr<FJsonObject> manifest;TArray<uint8> bytes;double schema=0,count=0;FString expected;
    if(!FFileHelper::LoadFileToString(text,*(folder/TEXT("vegetation.json")))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),manifest)||!manifest||
       !manifest->TryGetNumberField(TEXT("schema"),schema)||schema!=1||
       !manifest->TryGetNumberField(TEXT("count"),count)||count<1||count>2000000||
       !manifest->TryGetStringField(TEXT("md5"),expected)||
       !FFileHelper::LoadFileToArray(bytes,*(folder/TEXT("vegetation.trees")))||bytes.Num()!=8+count*24||
       FMemory::Memcmp(bytes.GetData(),"DVT1",4)!=0)
    { fail(TEXT("Vegetationsdaten fehlen oder sind ungueltig."));return; }
    uint32 encoded=0;FMemory::Memcpy(&encoded,bytes.GetData()+4,4);
    FMD5 md5;md5.Update(bytes.GetData(),bytes.Num());uint8 hash[16];md5.Final(hash);
    if(encoded!=count||!BytesToHex(hash,16).Equals(expected,ESearchCase::IgnoreCase))
    { fail(TEXT("Vegetationspruefsumme stimmt nicht."));return; }
    for(uint32 i=0;i<encoded;++i)
    {
        FDiveTree tree;FMemory::Memcpy(&tree,bytes.GetData()+8+i*24,24);
        if(!FMath::IsFinite(tree.East)||!FMath::IsFinite(tree.North)||!FMath::IsFinite(tree.Height)||!FMath::IsFinite(tree.NativeGround)||
            tree.East<-3000||tree.East>19000||tree.North<-4000||tree.North>9000||tree.Height<3||tree.Height>45||
            tree.Source>1||tree.Reserved||!(tree.Kind==0||tree.Kind==6||tree.Kind==12||tree.Kind==13||tree.Kind==14))
        { fail(TEXT("Ungueltiger Baumdatensatz."));return; }
        TreeSources.FindOrAdd(CellAt(FVector(tree.North*100.,tree.East*100.,0))).Add(tree);
    }
    TMap<UMaterialInterface*,UMaterialInstanceDynamic*> instances;
    for(const TCHAR* name:Names)
    {
        const FString path=FString(TEXT("/Game/Diving/Vegetation/Meshes/"))+name+TEXT(".")+name;
        auto* mesh=LoadObject<UStaticMesh>(nullptr,*path);
        if(!mesh||mesh->GetNumLODs()<2||mesh->GetNumTriangles(0)<=0||mesh->GetBounds().BoxExtent.Z<100)
        { fail(TEXT("Baummesh oder Detailstufen fehlen."));return; }
        TreeModels.Add(mesh);
        for(int slot=0;slot<mesh->GetStaticMaterials().Num();++slot)
        {
            auto* base=mesh->GetMaterial(slot);
            if(!base) { fail(TEXT("Baummaterial fehlt."));return; }
            if(!instances.Contains(base))
            {
                auto* material=UMaterialInstanceDynamic::Create(base,this);
                instances.Add(base,material);TreeMaterials.Add(material);
            }
        }
    }
    TreeMaterialLookup=MoveTemp(instances);
    UE_LOG(LogTemp,Display,TEXT("DIVE_VEGETATION_READY candidates=%u cells=%d models=%d"),encoded,TreeSources.Num(),TreeModels.Num());
}

void ADiveWorld::CreateTreeCell(FIntPoint Key)
{
    const auto* sources=TreeSources.Find(Key);
    if(!sources) return;
    const double start=FPlatformTime::Seconds();
    auto& cell=TreeCells.Add(Key);
    const FVector origin(Key.X*CellWidth,Key.Y*CellWidth,0);
    TArray<FTransform> transforms[5];
    for(const auto& tree:*sources)
    {
        FVector location(tree.North*100.,tree.East*100.,0);float ground=0;
        if(!Terrain.SampleGround(location.X,location.Y,ground))
        {
            double height=0;
            if(!Terrain.Landscape.SampleTriangle(Terrain.OriginEast+tree.East,Terrain.OriginNorth+tree.North,height)) continue;
            ground=static_cast<float>(height*100);
        }
        if(ground<90) continue;
        bool blocked=false;
        for(const auto& access:Accesses)
        {
            const FVector delta=location-access.Origin;
            const double along=FVector::DotProduct(delta,access.Forward);
            const double across=FMath::Abs(delta.X*access.Forward.Y-delta.Y*access.Forward.X);
            if(along>-600&&along<access.Length+600&&across<700) { blocked=true;break; }
        }
        if(blocked) continue;
        const int model=Model(tree);const auto* mesh=TreeModels[model].Get();
        const double scale=tree.Height*100/(mesh->GetBounds().BoxExtent.Z*2);
        const double yaw=(tree.Seed%36000)*.01;
        location.Z=ground-8;
        transforms[model].Add(FTransform(FRotator(0,yaw,0),location-origin,FVector(scale)));
        const double trunkHeight=FMath::Min(tree.Height*.6,12.)*100;
        const double trunkRadius=FMath::Clamp(tree.Height*1.25,10.,50.);
        cell.TrunkTransforms.Add(FTransform(FRotator::ZeroRotator,location-origin+FVector(0,0,trunkHeight*.5),
            FVector(trunkRadius/50,trunkRadius/50,trunkHeight/100)));
    }
    for(int model=0;model<5;++model)
    {
        auto* component=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        component->SetupAttachment(RootComponent);component->SetMobility(EComponentMobility::Static);
        component->SetRelativeLocation(origin);component->SetStaticMesh(TreeModels[model]);
        component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        component->SetCullDistances(100000,115000);
        component->SetBoundsScale(1.05f);
        for(int slot=0;slot<TreeModels[model]->GetStaticMaterials().Num();++slot)
            component->SetMaterial(slot,TreeMaterialLookup.FindChecked(TreeModels[model]->GetMaterial(slot)));
        component->bAutoRebuildTreeOnInstanceChanges=false;
        component->RegisterComponent();component->AddInstances(transforms[model],false,false,false);
        component->BuildTreeIfOutdated(false,true);cell.Trees.Add(component);
    }
    UE_LOG(LogTemp,Verbose,TEXT("DIVE_VEGETATION_CELL x=%d y=%d trees=%d milliseconds=%.3f"),Key.X,Key.Y,cell.TrunkTransforms.Num(),(FPlatformTime::Seconds()-start)*1000);
}

void ADiveWorld::StreamVegetation(const FVector& Player,bool Immediate)
{
    if(TreeModels.Num()!=5) return;
    const FIntPoint center=CellAt(Player);
    for(auto it=TreeCells.CreateIterator();it;++it)
        if(FMath::Abs(it.Key().X-center.X)>Radius||FMath::Abs(it.Key().Y-center.Y)>Radius)
        {
            for(auto& component:it.Value().Trees) component->DestroyComponent();
            if(it.Value().Trunks) it.Value().Trunks->DestroyComponent();
            it.RemoveCurrent();
        }
    TArray<FIntPoint> needed;
    for(int y=-Radius;y<=Radius;++y) for(int x=-Radius;x<=Radius;++x)
    {
        const auto key=center+FIntPoint(x,y);
        if(TreeSources.Contains(key)&&!TreeCells.Contains(key)) needed.Add(key);
    }
    needed.Sort([center](FIntPoint A,FIntPoint B){return (A-center).SizeSquared()<(B-center).SizeSquared();});
    const int additions=Immediate?needed.Num():FMath::Min(1,needed.Num());
    for(int i=0;i<additions;++i) CreateTreeCell(needed[i]);
    if(center!=TreeCenter||Immediate||additions)
    {
        TreeCenter=center;
        for(auto& pair:TreeCells)
        {
            auto& cell=pair.Value;
            const bool near=FMath::Abs(pair.Key.X-center.X)<=1&&FMath::Abs(pair.Key.Y-center.Y)<=1;
            if(!near&&cell.Trunks) { cell.Trunks->DestroyComponent();cell.Trunks=nullptr; }
            else if(near&&!cell.Trunks)
            {
                auto* trunks=NewObject<UInstancedStaticMeshComponent>(this);trunks->SetupAttachment(RootComponent);
                trunks->SetRelativeLocation(FVector(pair.Key.X*CellWidth,pair.Key.Y*CellWidth,0));
                trunks->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
                trunks->SetCollisionProfileName(TEXT("BlockAll"));trunks->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                trunks->SetVisibility(false);trunks->SetCastShadow(false);trunks->RegisterComponent();
                trunks->AddInstances(cell.TrunkTransforms,false,false,false);cell.Trunks=trunks;
            }
        }
    }
    const double angle=FMath::DegreesToRadians(WeatherSettings.WindDirectionDegrees);
    const double strength=FMath::Clamp(WeatherSettings.WindMetresPerSecond/8.,0.,3.);
    for(auto& material:TreeMaterials)
    {
        material->SetScalarParameterValue(TEXT("TreeTime"),WorldTime);
        material->SetVectorParameterValue(TEXT("TreeWind"),FLinearColor(FMath::Cos(angle)*strength,FMath::Sin(angle)*strength,strength*.3,0));
    }
}

bool ADiveWorld::SaveVegetationAnchorProbe(const FString& Directory) const
{
    auto* pc=GetWorld()->GetFirstPlayerController();if(!pc) return false;
    FVector eye;FRotator view;pc->GetPlayerViewPoint(eye,view);
    int32 width=0,height=0;pc->GetViewportSize(width,height);
    TArray<TSharedPtr<FJsonValue>> records;
    for(const auto& pair:TreeCells) for(const auto& component:pair.Value.Trees)
    {
        UStaticMesh* mesh=component->GetStaticMesh();if(!mesh) continue;
        for(int32 index=0;index<component->GetInstanceCount();++index)
        {
            FTransform transform;component->GetInstanceTransform(index,transform,true);
            const FVector root=transform.GetLocation();FVector2D screen;
            if(FVector::Distance(root,eye)>30000||!pc->ProjectWorldLocationToScreen(root,screen)||
               screen.X<0||screen.X>width||screen.Y<0||screen.Y>height) continue;
            auto row=MakeShared<FJsonObject>();
            row->SetStringField(TEXT("mesh"),mesh->GetName());
            for(const auto& n:{TPair<const TCHAR*,double>(TEXT("screen_x"),screen.X),{TEXT("screen_y"),screen.Y},
                {TEXT("east"),Terrain.OriginEast+root.Y/100},{TEXT("north"),Terrain.OriginNorth+root.X/100},
                {TEXT("root_z_cm"),root.Z},{TEXT("distance_m"),FVector::Distance(root,eye)/100},
                {TEXT("scale"),transform.GetScale3D().Z}}) row->SetNumberField(n.Key,n.Value);
            row->SetBoolField(TEXT("has_nanite"),mesh->HasValidNaniteData());
            float ground=0;row->SetBoolField(TEXT("sample_ground_valid"),Terrain.SampleGround(root.X,root.Y,ground));
            row->SetNumberField(TEXT("sample_ground_cm"),ground);
            // Independently ray-test the actual procedural shore collision.
            for(const auto& shore:ShoreMeshes)
            {
                FHitResult hit;FCollisionQueryParams query;
                if(shore->LineTraceComponent(hit,root+FVector(0,0,50000),root-FVector(0,0,50000),query))
                    row->SetNumberField(TEXT("shore_collision_z_cm"),hit.ImpactPoint.Z);
            }
#if WITH_EDITOR
            TArray<TSharedPtr<FJsonValue>> bottoms;
            if(const auto* data=mesh->GetRenderData()) for(const auto& lod:data->LODResources)
            {
                float bottom=MAX_flt;
                const auto& vertices=lod.VertexBuffers.PositionVertexBuffer;
                for(uint32 i=0;i<vertices.GetNumVertices();++i) bottom=FMath::Min(bottom,vertices.VertexPosition(i).Z);
                bottoms.Add(MakeShared<FJsonValueNumber>(bottom));
            }
            row->SetArrayField(TEXT("lod_min_local_z_cm"),bottoms);
#endif
            records.Add(MakeShared<FJsonValueObject>(row));
        }
    }
    auto json=MakeShared<FJsonObject>();json->SetArrayField(TEXT("trees"),records);
    FString text;return !records.IsEmpty()&&FJsonSerializer::Serialize(json,TJsonWriterFactory<>::Create(&text))&&
        FFileHelper::SaveStringToFile(text,*(Directory/TEXT("TreeAnchors.json")));
}

bool ADiveWorld::ValidateVegetation(FString& Error) const
{
    int instances=0,probes=0;
    if(TreeModels.Num()!=5||TreeCells.IsEmpty()) { Error=TEXT("Vegetation not loaded");return false; }
    for(const auto& pair:TreeCells)
    {
        const auto& cell=pair.Value;
        for(auto& mesh:cell.Trees)
        {
            instances+=mesh->GetInstanceCount();
            if(mesh->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||mesh->GetStaticMesh()->GetNumLODs()<2)
            { Error=TEXT("Tree rendering/LOD state wrong");return false; }
        }
        if(cell.Trunks&&cell.Trunks->GetInstanceCount())
        {
            FTransform transform;cell.Trunks->GetInstanceTransform(0,transform,true);
            const FVector p=transform.GetLocation();FHitResult hit;FCollisionQueryParams query;
            if(!cell.Trunks->LineTraceComponent(hit,p+FVector(200,0,0),p-FVector(200,0,0),query))
            { Error=TEXT("Trunk collision missed");return false; }
            ++probes;
        }
    }
    if(!instances) { Error=TEXT("No visible tree instances");return false; }
    const double angle=FMath::DegreesToRadians(WeatherSettings.WindDirectionDegrees);
    const double strength=FMath::Clamp(WeatherSettings.WindMetresPerSecond/8.,0.,3.);
    const FLinearColor wind(FMath::Cos(angle)*strength,FMath::Sin(angle)*strength,strength*.3,0);
    for(const auto& material:TreeMaterials)
        if(!material->K2_GetVectorParameterValue(TEXT("TreeWind")).Equals(wind,1.e-5f)||
            FMath::Abs(material->K2_GetScalarParameterValue(TEXT("TreeTime"))-WorldTime)>1.e-4)
        { Error=TEXT("Tree wind/time disconnected from simulation");return false; }
    UE_LOG(LogTemp,Display,TEXT("DIVE_VEGETATION_PASS cells=%d instances=%d trunk_probes=%d"),TreeCells.Num(),instances,probes);
    Error.Empty();return true;
}

bool ADiveWorld::TestVegetationStreaming(FString& Detail)
{
    const FVector player=Diver?Diver->GetActorLocation():PopulationOrigin;
    StreamVegetation(player,true);
    TArray<FIntPoint> keys;TreeCells.GetKeys(keys);
    const FIntPoint center=CellAt(player);
    keys.Sort([center](FIntPoint A,FIntPoint B){return (A-center).SizeSquared()<(B-center).SizeSquared();});
    const FIntPoint* chosen=keys.FindByPredicate([this](FIntPoint Key){return !TreeCells.FindChecked(Key).TrunkTransforms.IsEmpty();});
    if(!chosen) { Detail=TEXT("No cell to inspect");return false; }
    const FIntPoint key=*chosen;
    auto& initial=TreeCells.FindChecked(key);TArray<FTransform> snapshot[5];
    for(int model=0;model<5;++model)
        for(int i=0;i<initial.Trees[model]->GetInstanceCount();++i)
        { FTransform t;initial.Trees[model]->GetInstanceTransform(i,t,true);snapshot[model].Add(t); }
    const TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> retained=initial.Trees[0];
    StreamVegetation(player+FVector(30000,0,0),true);
    const auto* nearby=TreeCells.Find(key);
    if(!nearby||nearby->Trees[0].Get()!=retained.Get())
    { Detail=TEXT("Nearby cell recreated during a boundary crossing");return false; }
    StreamVegetation(player+FVector(0,350000,0),true);
    if(TreeCells.Contains(key)) { Detail=TEXT("Distant cell was not retired");return false; }
    StreamVegetation(player,true);
    const auto* returned=TreeCells.Find(key);
    if(!returned) { Detail=TEXT("Original cell did not return");return false; }
    int verified=0;
    for(int model=0;model<5;++model)
    {
        if(returned->Trees[model]->GetInstanceCount()!=snapshot[model].Num())
        { Detail=TEXT("Tree count changed after return");return false; }
        for(int i=0;i<snapshot[model].Num();++i)
        {
            FTransform actual;returned->Trees[model]->GetInstanceTransform(i,actual,true);
            if(!actual.Equals(snapshot[model][i],1.e-4))
            { Detail=TEXT("A tree moved or changed size after return");return false; }
            ++verified;
        }
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_VEGETATION_STREAM_PASS retained=1 retired=1 returned=1 transforms=%d active_cells=%d"),verified,TreeCells.Num());
    Detail.Empty();return true;
}

#include "DiveHUD.h"
#include "DivePawn.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Async/Async.h"

namespace
{
const FLinearColor White(.87,.94,.92,1),Muted(.42,.59,.57,1),Accent(.43,.87,.69,1),Amber(1,.73,.3,1);
}

void ADiveHUD::OpenDiscoveries(bool SelectedDive)
{
    if(!Diver) Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver) return;
    bDiscoveriesPage=true;bDiscoveryLog=SelectedDive;DiscoverySpecies=-1;DiscoveryPhoto=0;DiscoveryUnreadable=0;
    Gallery.Reset();PhotographTexture=nullptr;DiveDiscoveries::LoadCatalog(SpeciesCatalog);Diver->SetMenu(true);
    if(SelectedDive)
    {
        bDiscoveryLoading=false;
        for(const auto& photo:SelectedLog.Photographs) Gallery.Add({photo,SelectedLog.Location,SelectedLog.bTest});
        SelectPhotograph();return;
    }
    const FString directory=FPaths::ProjectSavedDir()/TEXT("DiveLogs");const bool tests=Diver->TestPhotographs();
    bDiscoveryLoading=true;
    DiscoveryTask=Async(EAsyncExecution::ThreadPool,[directory,tests](){return DiveLogbook::Scan(directory,tests);});
}
void ADiveHUD::CloseDiscoveries()
{ bDiscoveriesPage=false;PhotographTexture=nullptr;Gallery.Reset(); }
TArray<int32> ADiveHUD::VisiblePhotos() const
{
    TArray<int32> result;for(int i=0;i<Gallery.Num();++i)
        if(DiscoverySpecies<0||Gallery[i].Photo.Species.Contains(DiveDiscoveries::SpeciesId(DiscoverySpecies))) result.Add(i);
    return result;
}
void ADiveHUD::SelectPhotograph()
{
    PhotographTexture=nullptr;const auto visible=VisiblePhotos();DiscoveryPhoto=FMath::Clamp(DiscoveryPhoto,0,FMath::Max(0,visible.Num()-1));
    if(visible.IsEmpty()) return;
    const auto& p=Gallery[visible[DiscoveryPhoto]];const FString path=DiveDiscoveries::PhotoPath(p.Photo.Id,p.Test);
    // Do not decode unbounded or non-PNG data after a local file was replaced.
    const int64 size=IFileManager::Get().FileSize(*path);if(size<24||size>64*1024*1024) return;
    TArray<uint8> bytes;if(!FFileHelper::LoadFileToArray(bytes,*path)||bytes.Num()<24) return;
    const uint8 magic[]={137,80,78,71,13,10,26,10};if(FMemory::Memcmp(bytes.GetData(),magic,8)) return;
    auto integer=[&](int offset){return (uint32(bytes[offset])<<24)|(uint32(bytes[offset+1])<<16)|(uint32(bytes[offset+2])<<8)|bytes[offset+3];};
    const uint32 width=integer(16),height=integer(20);
    if(!width||!height||width>8192||height>8192||uint64(width)*height>16777216) return;
    PhotographTexture=FImageUtils::ImportBufferAsTexture2D(bytes);
}
void ADiveHUD::DiscoveryClick(FName Id)
{
    if(Id==TEXT("discovery_back")) { CloseDiscoveries();return; }
    if(Id==TEXT("discovery_language")) { Diver->bEnglish=!Diver->bEnglish;Diver->SaveSettings();return; }
    if(bDiscoveryLoading) return;
    if(Id==TEXT("discovery_all")) { DiscoverySpecies=-1;DiscoveryPhoto=0; }
    else if(Id.ToString().StartsWith(TEXT("discovery_species_"))) { DiscoverySpecies=FMath::Clamp(FCString::Atoi(*Id.ToString().Mid(18)),0,7);DiscoveryPhoto=0; }
    else if(Id==TEXT("discovery_prev")) --DiscoveryPhoto;
    else if(Id==TEXT("discovery_next")) ++DiscoveryPhoto;
    else if(Id==TEXT("discovery_source")&&SpeciesCatalog.IsValidIndex(DiscoverySpecies))
    { FPlatformProcess::LaunchURL(*SpeciesCatalog[DiscoverySpecies].Source,nullptr,nullptr);return; }
    SelectPhotograph();
}
void ADiveHUD::Discoveries()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    if(bDiscoveryLoading&&DiscoveryTask.IsReady())
    {
        const auto index=DiscoveryTask.Get();bDiscoveryLoading=false;DiscoveryUnreadable=index.Unreadable;
        TSet<FString> ids;
        // The current dive's in-memory snapshot wins over an older autosave.
        for(const auto& photo:Diver->Photographs) { Gallery.Add({photo,Diver->EntryName(),Diver->TestPhotographs()});ids.Add(photo.Id.ToLower()); }
        for(const auto& log:index.Entries)
            for(const auto& photo:log.Photographs) if(!ids.Contains(photo.Id.ToLower()))
            { Gallery.Add({photo,log.Location,log.bTest});ids.Add(photo.Id.ToLower()); }
        Gallery.Sort([](const FGalleryPhoto& a,const FGalleryPhoto& b){return a.Photo.UTC>b.Photo.UTC;});SelectPhotograph();
    }
    Rect(40,25,1520,855,FLinearColor(.008,.026,.027,.98));
    Text(L(TEXT("D I V E   /   E N T D E C K E N"),TEXT("D I V E   /   D I S C O V E R")),70,52,20,Accent);
    Text(L(TEXT("Arten und Fotos"),TEXT("Species and photos")),70,95,35,White);
    Text(bDiscoveryLog?L(TEXT("Fotos dieses Tauchgangs"),TEXT("Photos from this dive")):L(TEXT("Deine lokale Fundsammlung"),TEXT("Your local discovery collection")),70,151,17,Muted);
    Button(TEXT("discovery_all"),L(TEXT("Alle Fotos"),TEXT("All photographs"))+FString::Printf(TEXT("  (%d)"),Gallery.Num()),70,190,390,42,DiscoverySpecies<0);
    for(int i=0;i<SpeciesCatalog.Num();++i)
    {
        int count=0;for(const auto& p:Gallery) if(p.Photo.Species.Contains(SpeciesCatalog[i].Id)) ++count;
        const FString name=count?(Diver->bEnglish?SpeciesCatalog[i].NameEN:SpeciesCatalog[i].NameDE):L(TEXT("Noch nicht entdeckt"),TEXT("Not discovered yet"));
        Button(FName(*FString::Printf(TEXT("discovery_species_%d"),i)),FString::Printf(TEXT("%d  %s"),i+1,*name),70,247+i*57,390,49,DiscoverySpecies==i);
    }
    Text(L(TEXT("Mit [ "),TEXT("Use [ "))+Diver->KeyText(EDiveAction::Photo)+L(TEXT(" ] unter Wasser fotografieren."),TEXT(" ] to photograph underwater.")),70,729,15,Accent);
    Text(L(TEXT("Nah, mittig, beleuchtet und unverdeckt."),TEXT("Close, centered, lit and unobstructed.")),70,756,15,Muted);
    if(DiscoveryUnreadable) Text(FString::Printf(TEXT("%d "),DiscoveryUnreadable)+L(TEXT("Logdateien nicht lesbar."),TEXT("log files unreadable.")),70,790,14,Amber);
    Button(TEXT("discovery_back"),L(TEXT("< Zurück  (Esc)"),TEXT("< Back  (Esc)")),70,825,390,40);
    Button(TEXT("discovery_language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),1270,825,260,40);
    if(bDiscoveryLoading) { Text(L(TEXT("Sammlung wird geladen ..."),TEXT("Loading collection ...")),520,210,25,Muted);return; }
    if(SpeciesCatalog.IsEmpty()) { Text(L(TEXT("Artenkatalog nicht lesbar."),TEXT("Species catalog could not be loaded.")),520,210,25,Amber);return; }
    const auto visible=VisiblePhotos();
    if(DiscoverySpecies>=0&&visible.IsEmpty())
    {
        Text(L(TEXT("Noch keine Aufnahme dieser Art / Gruppe."),TEXT("No photograph of this species / group yet.")),520,210,25,White);
        Text(L(TEXT("Steckbriefe werden durch deine Fotos freigeschaltet."),TEXT("Your photographs unlock the field guide entries.")),520,262,18,Muted);return;
    }
    if(SpeciesCatalog.IsValidIndex(DiscoverySpecies))
    {
        const auto& species=SpeciesCatalog[DiscoverySpecies];
        Text(Diver->bEnglish?species.NameEN:species.NameDE,520,100,30,White);Text(species.Scientific,520,148,21,Accent);
        FString remaining=Diver->bEnglish?species.DescriptionEN:species.DescriptionDE;
        for(int line=0;!remaining.IsEmpty()&&line<3;++line)
        {
            int length=FMath::Min(90,remaining.Len());if(length<remaining.Len()) { while(length>0&&remaining[length]!=' ') --length;if(length==0) length=90; }
            Text(remaining.Left(length),520,192+line*25,17,Muted);remaining=remaining.Mid(length).TrimStart();
        }
        Button(TEXT("discovery_source"),L(TEXT("Quelle öffnen"),TEXT("Open source")),1270,262,260,38);
    }
    else
    {
        Text(L(TEXT("Dein Blick unter die Oberfläche"),TEXT("Your view below the surface")),520,110,30,White);
        Text(L(TEXT("Arten links auswählen: Steckbrief und zugehörige Fotos."),TEXT("Choose a species on the left for its guide and photographs.")),520,169,18,Muted);
        Text(L(TEXT("Arterkennung ist eine Spielregel, keine biologische Bildanalyse."),TEXT("Identification is a game rule, not biological image analysis.")),520,211,16,Muted);
    }
    if(visible.IsEmpty()) { Text(L(TEXT("Dein erstes Foto wartet auf dich."),TEXT("Your first photograph awaits.")),520,390,27,White);return; }
    const auto& p=Gallery[visible[DiscoveryPhoto]];
    Rect(520,314,1010,420,FLinearColor::Black);
    if(PhotographTexture)
    {
        const double fit=FMath::Min(1010./PhotographTexture->GetSizeX(),420./PhotographTexture->GetSizeY());
        const double w=PhotographTexture->GetSizeX()*fit,h=PhotographTexture->GetSizeY()*fit;
        FCanvasTileItem tile(FVector2D(OffsetX+(520+(1010-w)/2)*Scale,OffsetY+(314+(420-h)/2)*Scale),PhotographTexture->GetResource(),FVector2D(w*Scale,h*Scale),FLinearColor::White);
        tile.BlendMode=SE_BLEND_Opaque;Canvas->DrawItem(tile);
    }
    else Text(L(TEXT("Bild fehlt oder ist nicht lesbar. Funddaten bleiben erhalten."),TEXT("Image missing or unreadable. Discovery records are retained.")),550,490,20,Amber);
    const auto& photo=p.Photo;
    Text(p.Location.Left(35)+TEXT("  |  ")+photo.UTC.Left(19)+TEXT(" UTC"),520,747,16,Muted);
    Text(FString::Printf(TEXT("%02d:%02d   /   %.1f m   /   %d "),int(photo.Seconds)/60,int(photo.Seconds)%60,photo.Depth,photo.Species.Num())+L(TEXT("Arten / Gruppen"),TEXT("species / groups")),520,777,17,Accent);
    Button(TEXT("discovery_prev"),TEXT("<"),520,825,65,40);
    Text(FString::Printf(TEXT("%d / %d"),DiscoveryPhoto+1,visible.Num()),610,839,17,White);
    Button(TEXT("discovery_next"),TEXT(">"),735,825,65,40);
}

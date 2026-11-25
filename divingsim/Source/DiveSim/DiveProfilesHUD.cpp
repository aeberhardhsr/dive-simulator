#include "DiveHUD.h"
#include "DiveTextDialog.h"
#include "Simulation/ConfigInput.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveConfigIO.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

namespace
{
const FLinearColor White(.87f,.94f,.92f,1),Muted(.42f,.59f,.57f,1),Accent(.43f,.87f,.69f,1),Amber(1,.73f,.3f,1);
const FLinearColor Panel(.025f,.065f,.063f,.96f);
FString ShortName(const FString& Name)
{
    if(Name.Len()<=20) return Name;
    int end=19; if(Name[end-1]>=0xD800&&Name[end-1]<=0xDBFF) --end;
    return Name.Left(end)+TEXT("…");
}

}
void ADiveHUD::EndPlay(const EEndPlayReason::Type Reason)
{ CloseConfigInput(false); CloseProfileName(false); Super::EndPlay(Reason); }
void ADiveHUD::OpenProfiles()
{
    Diver=Cast<ADivePawn>(GetOwningPawn()); if(!Diver) return;
    Diver->SetMenu(true); bControlsPage=false; bProfilesPage=true; ProfileMessage.Empty();
    const bool test=FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"));
    Profiles.Load(FPaths::ProjectSavedDir()/(test?TEXT("DiveProfilesTest.json"):TEXT("DiveProfiles.json")));
    RefreshProfileSelection();
}
void ADiveHUD::CloseProfiles(bool IncludingDialog)
{
    if(ProfileDialog) { CloseProfileName(); if(!IncludingDialog) return; }
    bProfilesPage=false;
}
TArray<int32> ADiveHUD::VisibleProfiles() const
{
    TArray<int32> visible;
    for(int i=0;i<Profiles.Items.Num();++i) if(Profiles.Items[i].Archived==bProfileArchive) visible.Add(i);
    visible.Sort([this](int a,int b){return Profiles.Items[a].Name.Compare(Profiles.Items[b].Name,ESearchCase::IgnoreCase)<0;});
    return visible;
}
void ADiveHUD::RefreshProfileSelection()
{
    const auto visible=VisibleProfiles();
    const auto* selected=Profiles.Find(ProfileSelection);
    if(!selected||selected->Archived!=bProfileArchive) ProfileSelection=visible.IsEmpty()?FString():Profiles.Items[visible[0]].Id;
    ProfilePage=FMath::Clamp(ProfilePage,0,FMath::Max(0,(visible.Num()-1)/7));
}
FString ADiveHUD::ProfileErrorText() const
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    switch(Profiles.Error)
    {
    case EProfileError::None:return ProfileMessage;
    case EProfileError::Read:case EProfileError::Invalid:return L(TEXT("Profildatei nicht lesbar. Sie bleibt unveraendert; bitte pruefen und neu laden."),TEXT("Profile file is unreadable. It remains unchanged; check it and reload."));
    case EProfileError::Write:return L(TEXT("Speichern fehlgeschlagen. Der bisherige Stand bleibt erhalten."),TEXT("Saving failed. The previous state is preserved."));
    case EProfileError::Conflict:return L(TEXT("Datei wurde anderswo geaendert. Bitte neu laden."),TEXT("The file changed elsewhere. Please reload."));
    case EProfileError::Name:return L(TEXT("Bitte 1–40 Zeichen ohne Zeilenumbrueche eingeben."),TEXT("Use 1–40 characters without line breaks."));
    case EProfileError::Duplicate:return L(TEXT("Dieser Name existiert bereits, eventuell im Archiv."),TEXT("This name already exists, possibly in the archive."));
    case EProfileError::Limit:return L(TEXT("Grenze erreicht: 32 aktive Profile, 128 insgesamt."),TEXT("Limit reached: 32 active profiles, 128 in total."));
    case EProfileError::NoPrevious:return L(TEXT("Kein vorheriger Stand vorhanden."),TEXT("No previous version is available."));
    default:return L(TEXT("Profil nicht gefunden. Bitte neu laden."),TEXT("Profile not found. Please reload."));
    }
}
void ADiveHUD::OpenProfileName(bool Rename)
{
    if(ProfileDialog||!Profiles.bLoaded||!GEngine||!GEngine->GameViewport) return;
    const auto* selected=Profiles.Find(ProfileSelection); if(Rename&&!selected) return;
    bProfileRename=Rename; ProfileEditingId=Rename?ProfileSelection:FString(); ProfileNameError.Empty();
    const TWeakObjectPtr<ADiveHUD> weak(this);
    auto dialog=SNew(SDiveTextDialog).English(Diver->bEnglish)
        .Hint(FText::FromString(Diver->Local(TEXT("1–40 Zeichen. Enter speichert; Escape bricht ab."),TEXT("1–40 characters. Enter saves; Escape cancels."))))
        .Title(FText::FromString(Rename?Diver->Local(TEXT("Profil umbenennen"),TEXT("Rename profile")):Diver->Local(TEXT("Aktuellen Tauchplan speichern"),TEXT("Save current dive plan"))))
        .Initial(FText::FromString(Rename?selected->Name:FString()))
        .Error_Lambda([weak](){return weak.IsValid()?FText::FromString(weak->ProfileNameError):FText::GetEmpty();})
        .OnSubmit_Lambda([weak](const FText& Text,ETextCommit::Type){if(weak.IsValid()) weak->SubmitProfileName(Text.ToString());})
        .OnCancel_Lambda([weak](){if(weak.IsValid()) weak->CloseProfileName();});
    ProfileDialog=dialog; ProfileNameEditor=dialog->Editor;
    GEngine->GameViewport->AddViewportWidgetContent(dialog,100);
    FInputModeUIOnly mode; mode.SetWidgetToFocus(ProfileNameEditor); GetOwningPlayerController()->SetInputMode(mode);
    FSlateApplication::Get().SetKeyboardFocus(ProfileNameEditor,EFocusCause::SetDirectly);
}
void ADiveHUD::CloseProfileName(bool RestoreFocus)
{
    if(ProfileDialog&&GEngine&&GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(ProfileDialog.ToSharedRef());
    ProfileDialog.Reset(); ProfileNameEditor.Reset(); ProfileEditingId.Empty();
    if(RestoreFocus&&Diver) Diver->SetMenu(true);
}
bool ADiveHUD::SubmitProfileName(const FString& Name)
{
    if(!ProfileDialog) return false;
    FString created;
    const bool saved=bProfileRename?Profiles.Rename(ProfileEditingId,Name):Profiles.Add(Name,Diver->PendingConfig,Diver->EntryIndex,created);
    if(!saved) { ProfileNameError=ProfileErrorText(); return false; }
    if(!created.IsEmpty()) { ProfileSelection=created; bProfileArchive=false; }
    CloseProfileName(); RefreshProfileSelection();
    ProfileMessage=Diver->Local(TEXT("Profil gespeichert."),TEXT("Profile saved.")); return true;
}
bool ADiveHUD::ProfileClick(FName Id)
{
    if(ProfileDialog) return true;
    if(Id==TEXT("profile_back")) { CloseProfiles(); return true; }
    if(Id==TEXT("profile_language")) { Diver->bEnglish=!Diver->bEnglish; ProfileMessage.Empty(); Diver->SaveSettings(); return true; }
    if(Id==TEXT("profile_reload")) { OpenProfiles(); return true; }
    if(Id==TEXT("profile_filter")) { bProfileArchive=!bProfileArchive; ProfilePage=0; RefreshProfileSelection(); return true; }
    if(Id==TEXT("profile_prev")) { ProfilePage=FMath::Max(0,ProfilePage-1); return true; }
    if(Id==TEXT("profile_next")) { ++ProfilePage; RefreshProfileSelection(); return true; }
    if(Id.ToString().StartsWith(TEXT("profile_row_")))
    { const int i=FCString::Atoi(*Id.ToString().Mid(12)); if(Profiles.Items.IsValidIndex(i)) ProfileSelection=Profiles.Items[i].Id; return true; }
    if(Id==TEXT("profile_new")) { OpenProfileName(false); return true; }
    if(Id==TEXT("profile_rename")) { OpenProfileName(true); return true; }
    const auto* p=Profiles.Find(ProfileSelection); if(!p) return true;
    if(Id==TEXT("profile_load"))
    {
        if(Diver->bDiveActive||p->Archived) return true;
        Diver->PendingConfig=p->Config; Diver->EntryIndex=p->Entry; Diver->SaveSettings();
        ProfileMessage=Diver->Local(TEXT("Tauchplan geladen. Zurueck im Menue kannst du starten."),TEXT("Dive plan loaded. Return to the menu to begin.")); return true;
    }
    bool saved=false;
    if(Id==TEXT("profile_replace")) saved=Profiles.Replace(ProfileSelection,Diver->PendingConfig,Diver->EntryIndex);
    else if(Id==TEXT("profile_archive")) saved=Profiles.Archive(ProfileSelection,!p->Archived);
    else if(Id==TEXT("profile_undo")) saved=Profiles.Undo(ProfileSelection);
    if(saved) { ProfileMessage=Diver->Local(TEXT("Aenderung gespeichert."),TEXT("Change saved.")); RefreshProfileSelection(); }
    return true;
}
void ADiveHUD::ProfilesMenu()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    Text(L(TEXT("DEINE TAUCHPROFILE"),TEXT("YOUR DIVE PROFILES")),120,65,38,White);
    Text(L(TEXT("Ausruestung, Umwelt und Tauchplan als benannte Konfigurationen speichern."),TEXT("Save equipment, environment and dive plan as named configurations.")),120,120,18,Muted);
    Text(ProfileErrorText(),120,160,17,Profiles.Error==EProfileError::None?Accent:Amber);
    const auto visible=VisibleProfiles(); const auto* selected=Profiles.Find(ProfileSelection);
    Text(bProfileArchive?L(TEXT("ARCHIV"),TEXT("ARCHIVE")):L(TEXT("AKTIVE PROFILE"),TEXT("ACTIVE PROFILES")),120,210,16,Accent);
    for(int row=0;row<7;++row)
    {
        const int n=ProfilePage*7+row; if(!visible.IsValidIndex(n)) break;
        const int index=visible[n]; const auto& p=Profiles.Items[index];
        Button(FName(*FString::Printf(TEXT("profile_row_%d"),index)),ShortName(p.Name),120,247+row*60,400,48,p.Id==ProfileSelection);
    }
    if(visible.IsEmpty()) Text(bProfileArchive?L(TEXT("Das Archiv ist leer."),TEXT("The archive is empty.")):L(TEXT("Noch keine Profile gespeichert."),TEXT("No profiles saved yet.")),120,260,18,Muted);
    Button(TEXT("profile_prev"),TEXT("<"),120,690,60,42);
    Text(FString::Printf(TEXT("%d / %d   (%d)"),ProfilePage+1,FMath::Max(1,(visible.Num()+6)/7),visible.Num()),205,702,17,White);
    Button(TEXT("profile_next"),TEXT(">"),460,690,60,42);
    Button(TEXT("profile_filter"),bProfileArchive?L(TEXT("Aktive Profile anzeigen"),TEXT("Show active profiles")):L(TEXT("Archiv anzeigen"),TEXT("Show archive")),120,750,400,42);
    const auto& plan=selected?selected->Config:Diver->PendingConfig; const auto c=plan.Resolved(); const int entry=selected?selected->Entry:Diver->EntryIndex;
    Rect(560,210,920,420,Panel);
    Text(selected?selected->Name:L(TEXT("Aktueller Tauchplan"),TEXT("Current dive plan")),585,235,22,White);
    const FString modes[]={TEXT("Exploration"),TEXT("Simulation"),L(TEXT("Hardcore (unvollstaendig)"),TEXT("Hardcore (incomplete)"))};
    const FString weather[]={L(TEXT("Heiter"),TEXT("Clear")),L(TEXT("Bedeckt"),TEXT("Overcast")),L(TEXT("Regen"),TEXT("Rain")),L(TEXT("Sturm"),TEXT("Storm"))};
    const FString place=Diver->Environment&&Diver->Environment->Terrain.EntryList(c.StartOnShore).IsValidIndex(entry)?Diver->Environment->Terrain.EntryList(c.StartOnShore)[entry].Name:TEXT("?");
    Text(place+TEXT("  /  ")+modes[static_cast<int>(c.GameMode)]+(c.StartOnShore?L(TEXT("  /  Uferstart"),TEXT("  /  Shore start")):FString()),585,280,21,Accent);
    Text(FString::Printf(TEXT("%.0f L   /   %.1f bar   /   O2 %.0f %%   /   %.1f kg"),c.CylinderLitres,c.StartPressureBar,c.OxygenFraction*100,c.WeightKg),585,325,20,White);
    const FString suit=c.DrySuit?L(TEXT("Trockentauchanzug"),TEXT("Dry suit")):L(TEXT("Nassanzug"),TEXT("Wet suit"));
    const bool autoTrim=c.AutoBuoyancy||c.GameMode==Dive::Mode::Exploration;
    Text(suit+TEXT("   /   ")+(autoTrim?L(TEXT("Automatische Tarierung"),TEXT("Automatic buoyancy")):L(TEXT("Manuelle Tarierung"),TEXT("Manual buoyancy"))),585,365,19,White);
    const FString hour=UTF8_TO_TCHAR(Dive::FormatConfigInput(*Dive::FindConfigInput("hour"),c.Hour).c_str());
    Text(FString::Printf(TEXT("%s   /   %s   /   %.1f m/s   /   %05.1f deg"),*(FString(UTF8_TO_TCHAR(c.Date.Text().c_str()))+TEXT("  ")+hour),*weather[c.Weather],c.WindMetresPerSecond,c.WindDirectionDegrees),585,405,19,White);
    Text(FString::Printf(TEXT("%s %.1f m%s   /   %s %.2f m/s%s"),Diver->bEnglish?TEXT("Visibility"):TEXT("Sicht"),c.VisibilityMetres,c.AutoVisibility?TEXT(" (Auto)"):TEXT(""),Diver->bEnglish?TEXT("Current"):TEXT("Stroemung"),c.CurrentMetresPerSecond,c.CurrentSource==Dive::CurrentSetting::Automatic?TEXT(" (Auto)"):TEXT("")),585,445,19,White);
    if(c.AutoTemperature) Text(FString::Printf(TEXT("%.1f / %.1f C   /   %s"),c.SurfaceTemperatureC,c.BottomTemperatureC,*L(TEXT("Auto: saisonales Tiefenprofil"),TEXT("Auto: seasonal depth profile"))),585,485,19,White);
    else Text(FString::Printf(TEXT("%.1f / %.1f C   /   %s %.1f m (%.1f m)"),c.SurfaceTemperatureC,c.BottomTemperatureC,Diver->bEnglish?TEXT("Thermocline"):TEXT("Sprungschicht"),c.ThermoclineMetres,c.ThermoclineWidthMetres),585,485,19,White);
    Text(FString::Printf(TEXT("Plan: %.1f m   /   %.1f min   /   %s %.3f bar"),c.PlannedDepthMetres,c.PlannedMinutes,Diver->bEnglish?TEXT("Atmosphere"):TEXT("Atmosphaere"),c.SurfacePressureBar),585,525,20,White);
    const auto& torch=Dive::Torch::At(c.TorchModel);
    Text(FString::Printf(TEXT("%s: %s / %.0f lm / %.0f deg / %.0f min"),Diver->bEnglish?TEXT("Torch"):TEXT("Lampe"),UTF8_TO_TCHAR(torch.Name),torch.Lumens,2*torch.OuterDegrees,torch.RuntimeMinutes),585,563,18,White);
    if(selected&&DiveConfigIO::Equal(plan,entry,Diver->PendingConfig,Diver->EntryIndex)) Text(L(TEXT("Entspricht dem aktuellen Tauchplan."),TEXT("Matches the current dive plan.")),585,602,16,Accent);
    if(Diver->bDiveActive) Text(L(TEXT("Laden ist erst nach Ende des laufenden Tauchgangs moeglich."),TEXT("Loading is available after the current dive ends.")),560,645,16,Amber);
    if(selected)
    {
        if(!selected->Archived&&!Diver->bDiveActive) Button(TEXT("profile_load"),L(TEXT("Profil laden"),TEXT("Load profile")),560,680,260,48,true);
        if(!selected->Archived) Button(TEXT("profile_replace"),L(TEXT("Aktuellen Plan hier speichern"),TEXT("Save current plan here")),850,680,340,48);
        if(selected->Previous.IsSet()) Button(TEXT("profile_undo"),L(TEXT("Vorheriger Stand"),TEXT("Previous version")),1220,680,260,48);
        Button(TEXT("profile_rename"),L(TEXT("Umbenennen"),TEXT("Rename")),560,750,260,42);
        Button(TEXT("profile_archive"),selected->Archived?L(TEXT("Aus Archiv wiederherstellen"),TEXT("Restore from archive")):L(TEXT("Archivieren"),TEXT("Archive")),850,750,340,42);
    }
    Button(TEXT("profile_back"),L(TEXT("< Zurueck"),TEXT("< Back")),120,825,240,48,true);
    if(Profiles.bLoaded) Button(TEXT("profile_new"),L(TEXT("Aktuellen Plan als neues Profil speichern"),TEXT("Save current plan as a new profile")),390,825,610,48);
    Button(TEXT("profile_reload"),L(TEXT("Neu laden"),TEXT("Reload")),1030,825,190,48);
    Button(TEXT("profile_language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),1250,825,230,48);
}

bool ADiveHUD::ProfileSmokeTest(int Phase)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))) return false;
    auto shot=[](const TCHAR* Name)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(FString(Name)+TEXT(".png")),true,false); };
    switch(Phase)
    {
    case 0:
        IFileManager::Get().Delete(*(FPaths::ProjectSavedDir()/TEXT("DiveProfilesTest.json")));
        OpenProfiles(); Diver->PendingConfig.Hour=23; Diver->PendingConfig.VisibilityMetres=8;
        Diver->PendingConfig.CylinderLitres=15; Diver->PendingConfig.WeightKg=9; Diver->EntryIndex=1;
        NotifyHitBoxClick(TEXT("profile_new"));
        if(!ProfileNameEditor) return false;
        ProfileNameEditor->SetText(FText::FromString(TEXT("Nacht – Mürg")));
        shot(TEXT("ProfileName")); return true;
    case 1:
        if(!ProfileNameEditor||!SubmitProfileName(ProfileNameEditor->GetText().ToString())) return false;
        shot(TEXT("Profiles_DE"));
        return Profiles.Items.Num()==1&&Profiles.Items[0].Name==TEXT("Nacht – Mürg");
    case 2:
        NotifyHitBoxClick(TEXT("profile_new"));
        if(!ProfileNameEditor) return false;
        ProfileNameEditor->SetText(FText::FromString(TEXT("nacht – mürg")));
        if(SubmitProfileName(ProfileNameEditor->GetText().ToString())||Profiles.Error!=EProfileError::Duplicate) return false;
        shot(TEXT("ProfileDuplicate")); return ProfileDialog.IsValid();
    case 3:
        CloseProfiles();
        if(!bProfilesPage||ProfileDialog||Profiles.Items.Num()!=1) return false;
        NotifyHitBoxClick(TEXT("profile_rename"));
        return SubmitProfileName(TEXT("Nacht am Walensee"));
    case 4:
    {
        Diver->PendingConfig=Dive::Config{}; Diver->EntryIndex=0;
        NotifyHitBoxClick(TEXT("profile_replace"));
        if(!DiveConfigIO::Equal(Profiles.Find(ProfileSelection)->Config,Profiles.Find(ProfileSelection)->Entry,Dive::Config{},0)) return false;
        NotifyHitBoxClick(TEXT("profile_reload"));
        NotifyHitBoxClick(TEXT("profile_undo"));
        return Profiles.Find(ProfileSelection)->Config.CylinderLitres==15&&Profiles.Find(ProfileSelection)->Entry==1;
    }
    case 5:
        NotifyHitBoxClick(TEXT("profile_archive")); NotifyHitBoxClick(TEXT("profile_filter"));
        shot(TEXT("ProfilesArchive"));
        return bProfileArchive&&Profiles.Find(ProfileSelection)&&Profiles.Find(ProfileSelection)->Archived;
    case 6:
    {
        NotifyHitBoxClick(TEXT("profile_archive")); NotifyHitBoxClick(TEXT("profile_filter"));
        NotifyHitBoxClick(TEXT("profile_load"));
        const auto expected=Diver->PendingConfig; const int entry=Diver->EntryIndex;
        Diver->PendingConfig=Dive::Config{}; Diver->EntryIndex=0; Diver->LoadSettings();
        return entry==1&&expected.Hour==23&&expected.CylinderLitres==15&&DiveConfigIO::Equal(expected,entry,Diver->PendingConfig,Diver->EntryIndex);
    }
    case 7:
        OpenControls();
        if(ProfilesOpen()||!ControlsOpen()) return false;
        Diver->ToggleMenu();
        if(ControlsOpen()||!Diver->bMenu) return false;
        OpenProfiles(); Diver->ToggleMenu();
        if(ProfilesOpen()||!Diver->bMenu) return false;
        Diver->PendingConfig=Dive::Config{}; Diver->EntryIndex=0; Diver->StartDive();
        OpenProfiles(); NotifyHitBoxClick(TEXT("profile_load"));
        shot(TEXT("ProfilesActive"));
        return Diver->bDiveActive&&DiveConfigIO::Equal(Diver->PendingConfig,Diver->EntryIndex,Dive::Config{},0)&&
            DiveConfigIO::Equal(Diver->Simulation.Settings,0,Dive::Config{},0);
    case 8:
        NotifyHitBoxClick(TEXT("profile_language")); shot(TEXT("Profiles_EN"));
        return Diver->bEnglish;
    case 9:
        NotifyHitBoxClick(TEXT("profile_language")); NotifyHitBoxClick(TEXT("profile_back"));
        Diver->ResumeDive(); return !bProfilesPage&&!Diver->bMenu;
    case 10:
        return Diver->bDiveActive&&Diver->Simulation.Data.ElapsedSeconds>.2;
    default: return false;
    }
}

#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveAudio.h"
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
const FLinearColor White(.87,.94,.92,1), Muted(.42,.59,.57,1), Accent(.43,.87,.69,1);
const FLinearColor Dark(.012,.032,.032,.94), Panel(.025,.065,.063,.92), Red(1,.35,.25,1), Amber(1,.73,.3,1);
FString Time(double Seconds) { const int t=FMath::Max(0,FMath::FloorToInt(Seconds)); return FString::Printf(TEXT("%02d:%02d"),t/60,t%60); }
}

void ADiveHUD::Text(const FString& Value,float X,float Y,float Size,FLinearColor Color)
{
    if(!InterfaceFont) InterfaceFont=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    const FSlateFontInfo font(InterfaceFont,FMath::Max(8,FMath::RoundToInt(Size*Scale*.75f)),TEXT("Regular"));
    FCanvasTextItem item(FVector2D(OffsetX+X*Scale,OffsetY+Y*Scale),FText::FromString(Value),font,Color);
    Canvas->DrawItem(item);
}
void ADiveHUD::Rect(float X,float Y,float W,float H,FLinearColor Color)
{ DrawRect(Color,OffsetX+X*Scale,OffsetY+Y*Scale,W*Scale,H*Scale); }
void ADiveHUD::Line(float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness)
{ DrawLine(OffsetX+X*Scale,OffsetY+Y*Scale,OffsetX+X2*Scale,OffsetY+Y2*Scale,Color,Thickness*Scale); }

float ADiveHUD::TextWidth(const FString& Value,float Size)
{
    if(!InterfaceFont) InterfaceFont=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    const FSlateFontInfo font(InterfaceFont,FMath::Max(8,FMath::RoundToInt(Size*Scale*.75f)),TEXT("Regular"));
    return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value,font).X/Scale;
}

float ADiveHUD::TextBlock(const FString& Value,float X,float Y,float W,float Size,FLinearColor Color,int32 MaxLines,bool MeasureOnly)
{
    TArray<FString> paragraphs,lines;
    Value.ParseIntoArrayLines(paragraphs,false);
    for(const auto& paragraph:paragraphs)
    {
        TArray<FString> words; paragraph.ParseIntoArray(words,TEXT(" "),true);
        FString line;
        for(const auto& word:words)
        {
            const FString candidate=line.IsEmpty()?word:line+TEXT(" ")+word;
            if(!line.IsEmpty()&&TextWidth(candidate,Size)>W) { lines.Add(line);line=word; }
            else line=candidate;
        }
        lines.Add(line);
    }
    const int32 count=FMath::Min(MaxLines,lines.Num());
    if(MeasureOnly) return count*(Size+5);
    for(int32 i=0;i<count;++i)
    {
        FString line=lines[i];
        const bool more=i==count-1&&lines.Num()>count;
        if(more||TextWidth(line,Size)>W)
        {
            if(bUIAudit&&!bUIOverflow)
            {
                bUIOverflow=true;
                UE_LOG(LogTemp,Error,TEXT("DIVE_UI_TEXT_OVERFLOW x=%.1f y=%.1f width=%.1f text=%s"),X,Y,W,*Value);
            }
            while(!line.IsEmpty()&&TextWidth(line+TEXT("…"),Size)>W)
            {
                // Preserve UTF-16 pairs in player-entered names when shortening.
                const uint32 last=static_cast<uint32>(line[line.Len()-1]);
                const uint32 previous=line.Len()>1?static_cast<uint32>(line[line.Len()-2]):0;
                line.LeftChopInline(last>=0xdc00&&last<=0xdfff&&previous>=0xd800&&previous<=0xdbff?2:1);
            }
            line+=TEXT("…");
        }
        Text(line,X,Y+i*(Size+5),Size,Color);
    }
    return count*(Size+5);
}

void ADiveHUD::Button(FName Id,const FString& Label,float X,float Y,float W,float H,bool Highlight,bool Enabled)
{
    float mx=0,my=0; GetOwningPlayerController()->GetMousePosition(mx,my);
    const bool hover=mx>=OffsetX+X*Scale && mx<=OffsetX+(X+W)*Scale && my>=OffsetY+Y*Scale && my<=OffsetY+(Y+H)*Scale;
    Rect(X,Y,W,H,!Enabled?Dark:Highlight?Accent:(hover?FLinearColor(.07,.15,.13,1):Panel));
    const bool focus=Enabled&&bKeyboardFocus&&KeyboardFocus==Id;
    if(focus)
    {
        Line(X,Y,X+W,Y,White,2);Line(X,Y+H,X+W,Y+H,White,2);
        Line(X,Y,X,Y+H,White,2);Line(X+W,Y,X+W,Y+H,White,2);
    }
    const float size=H<40?15:17;
    TextBlock(Label,X+14,Y+(H-size-5)/2,W-28,size,!Enabled?Muted:Highlight?Dark:White,1);
    if(Enabled)
    {
        ButtonOrder.AddUnique(Id);
        AddHitBox(FVector2D(OffsetX+X*Scale,OffsetY+Y*Scale),FVector2D(W*Scale,H*Scale),Id,true);
    }
}

void ADiveHUD::Field(FName Id,const FString& Label,const FString& Value,float X,float Y,float W)
{
    const bool enabled=!Diver->bDiveActive||Id==TEXT("motion");
    Button(Id,TEXT(""),X,Y,W,78,false,enabled);
    TextBlock(Label,X+14,Y+10,W-30,13,Muted,1);
    TextBlock(Value,X+14,Y+32,W-40,18,enabled?White:Muted,2);
    if(enabled) Text(TEXT("›"),X+W-23,Y+33,19,Accent);
}

bool ADiveHUD::HandleMenuKey(FKey Key,bool Shift)
{
    if(!Diver||!Diver->bMenu||ConfigDialog||ProfileDialog||WaypointDialog||ButtonOrder.IsEmpty()) return false;
    if(Key==EKeys::LeftMouseButton) { bKeyboardFocus=false;return false; }
    if(Key==EKeys::Tab||Key==EKeys::Up||Key==EKeys::Down||Key==EKeys::Left||Key==EKeys::Right)
    {
        const bool backwards=(Key==EKeys::Tab&&Shift)||Key==EKeys::Up||Key==EKeys::Left;
        int32 index=ButtonOrder.Find(KeyboardFocus);
        if(!bKeyboardFocus||index==INDEX_NONE) index=backwards?0:-1;
        index=(index+(backwards?-1:1)+ButtonOrder.Num())%ButtonOrder.Num();
        KeyboardFocus=ButtonOrder[index];bKeyboardFocus=true;return true;
    }
    if((Key==EKeys::Enter||Key==EKeys::SpaceBar)&&bKeyboardFocus&&ButtonOrder.Contains(KeyboardFocus))
    { NotifyHitBoxClick(KeyboardFocus);return true; }
    return false;
}

bool ADiveHUD::BackToDivePlan()
{
    if(!bEnvironmentPage&&!bAudioPage&&!bLoadoutPage) return false;
    bEnvironmentPage=bAudioPage=bLoadoutPage=bAvatarView=false;
    KeyboardFocus=TEXT("plan");return true;
}

void ADiveHUD::DrawHUD()
{
    Super::DrawHUD();
    ButtonOrder.Reset();
    Diver=Cast<ADivePawn>(GetOwningPawn()); if(!Diver || !Canvas || Diver->PhotoPending()) return;
    if(Diver->Equipment&&Diver->Equipment->Avatar)
        Diver->Equipment->Avatar->SetInspection(Diver->bMenu&&bLoadoutPage&&bAvatarView&&!bDiscoveriesPage&&!bEntryMapPage&&!bProfilesPage&&!bControlsPage&&!Diver->bLogbook&&!Diver->bSummary);
    Scale=FMath::Min(Canvas->SizeX/1600.f,Canvas->SizeY/900.f);
    OffsetX=(Canvas->SizeX-1600*Scale)/2; OffsetY=(Canvas->SizeY-900*Scale)/2;
    if(Diver->bMenu)
    {
        DrawRect(FLinearColor(0,.012,.014,.62),0,0,Canvas->SizeX,Canvas->SizeY);
        if(bDiscoveriesPage) Discoveries(); else if(bWaypointPage||bEntryMapPage) Map(true); else if(bProfilesPage) ProfilesMenu(); else if(bControlsPage) ControlsMenu(); else if(Diver->bLogbook) Logbook(); else if(Diver->bSummary) Summary(); else Menu();
    }
    else
    {
        HUDRightBottom=22;
        if(Diver->bShowInstruments||Diver->Simulation.Settings.Rules().PermanentHUD) Instruments();
        if(Diver->bTutorialActive) TutorialOverlay();
        if(Diver->bNoticeEnglish==Diver->bEnglish&&GetWorld()->GetTimeSeconds()<Diver->NoticeUntil)
        {
            if(Diver->bShowComputer)
            {
                const float height=TextBlock(Diver->Notice,44,222,294,16,White,5,true);
                Rect(26,210,330,height+24,Dark);
                TextBlock(Diver->Notice,44,222,294,16,White,5);
            }
            else
            { Rect(450,690,700,64,Dark);TextBlock(Diver->Notice,468,702,664,16,White,2); }
        }
        if(Diver->Simulation.Settings.Rules().AutomaticHints&&Diver->CanFinishDive()&&!Diver->bShowMap)
        {
            const FString hint=TEXT("[ ")+Diver->KeyText(EDiveAction::Finish)+TEXT(" ]  ")+Diver->Local(TEXT("Tauchgang abschliessen"),TEXT("Finish dive"));
            if(Diver->bShowComputer)
            { Rect(26,351,330,56,Dark);TextBlock(hint,44,359,294,16,Accent,2); }
            else { Rect(570,644,460,36,Dark);TextBlock(hint,586,652,428,16,Accent,1); }
        }
        if(Diver->bShowMap) Map();
        else Text(TEXT("+"),795,445,14,FLinearColor(.75,.9,.86,.45));
    }
}

void ADiveHUD::Summary()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    Text(TEXT("D I V E   /   L O G"),120,80,18,Accent);
    Text(Diver->Tutorial.Stage==Dive::Lesson::Done?L(TEXT("Einführung abgeschlossen."),TEXT("Introduction complete.")):L(TEXT("Dein Tauchgang."),TEXT("Your dive.")),120,137,54,White);
    const FString outcome=Diver->Outcome==TEXT("completed")?L(TEXT("Zurück an der Oberfläche"),TEXT("Back at the surface")):
        Diver->Outcome==TEXT("no_breathing_supply")?L(TEXT("Abbruch: keine Atemluft"),TEXT("Ended: no breathing supply")):
        Diver->Outcome==TEXT("out_of_gas")?L(TEXT("Abbruch: Flasche leer"),TEXT("Ended: cylinder empty")):Diver->Outcome;
    Text(outcome,122,215,19,Muted);
    const FString titles[]={L(TEXT("DAUER"),TEXT("DURATION")),L(TEXT("MAXIMALE TIEFE"),TEXT("MAXIMUM DEPTH")),L(TEXT("RESTDRUCK"),TEXT("GAS REMAINING")),L(TEXT("STRECKE"),TEXT("DISTANCE"))};
    const FString values[]={Time(s.ElapsedSeconds),FString::Printf(TEXT("%.1f m"),s.MaxDepthMetres),FString::Printf(TEXT("%.0f bar"),s.CylinderBar),FString::Printf(TEXT("%.0f m"),s.DistanceMetres)};
    for(int i=0;i<4;++i)
    { Rect(120+i*345,290,325,130,Panel); Text(titles[i],140+i*345,310,14,Muted); Text(values[i],137+i*345,347,37,White); }
    Text(L(TEXT("TIEFENPROFIL"),TEXT("DEPTH PROFILE")),120,458,16,Accent);
    Rect(120,500,1360,195,Panel);
    for(int i=0;i<5;++i) Line(120,500+i*48,1480,500+i*48,FLinearColor(.08,.15,.14,1));
    const float range=FMath::Max(5.0,s.MaxDepthMetres);
    for(int i=1;i<Diver->Samples.Num();++i)
    {
        const auto& a=Diver->Samples[i-1].State; const auto& b=Diver->Samples[i].State;
        Line(120+a.ElapsedSeconds/FMath::Max(1.0,s.ElapsedSeconds)*1360,500+a.DepthMetres/range*190,
            120+b.ElapsedSeconds/FMath::Max(1.0,s.ElapsedSeconds)*1360,500+b.DepthMetres/range*190,Accent,2);
    }
    Text(L(TEXT("Telemetrie als JSON: divingsim/Saved/DiveLogs"),TEXT("JSON telemetry: divingsim/Saved/DiveLogs")),120,717,16,Muted);
    Button(TEXT("new"),L(TEXT("Nächsten Tauchgang planen  >"),TEXT("Plan another dive  >")),120,785,540,60,true);
    Button(TEXT("logbook"),L(TEXT("Logbuch / Verläufe"),TEXT("Logbook / profiles")),690,785,490,60);
    Button(TEXT("quit"),L(TEXT("Beenden"),TEXT("Quit")),1215,785,265,60);
}

void ADiveHUD::NotifyHitBoxClick(FName Id)
{
    Super::NotifyHitBoxClick(Id); if(!Diver||ConfigDialog) return;
    if(Id==TEXT("hud_style")) { Diver->HUDStyle=(Diver->HUDStyle+1)%3;Diver->SaveSettings();return; }
    if(Id==TEXT("tutorial_overlay")) { bTutorialCollapsed=!bTutorialCollapsed;return; }
    if(bDiscoveriesPage) { DiscoveryClick(Id);return; }
    if(Id==TEXT("discoveries")) { OpenDiscoveries();return; }
    if(bWaypointPage) { WaypointClick(Id);return; }
    if(bEntryMapPage) { MapClick(Id); return; }
    if(Id==TEXT("profiles")) { OpenProfiles(); return; }
    if(bProfilesPage) { ProfileClick(Id); return; }
    if(Id==TEXT("controls")) { OpenControls(); return; }
    if(Id==TEXT("tutorial")) { if(!Diver->bDiveActive) { bControlsPage=false; Diver->StartTutorial(); } return; }
    if(bControlsPage)
    {
        if(Id==TEXT("controls_back")) { bControlsPage=false; Diver->CapturingAction=INDEX_NONE; }
        else if(Id==TEXT("controls_reset")) { Diver->Controls=FDiveControls{}; Diver->CapturingAction=INDEX_NONE; Diver->ControlMessage=Diver->Local(TEXT("Standardbelegung wiederhergestellt."),TEXT("Default controls restored.")); }
        else if(Id==TEXT("controls_language")) { Diver->bEnglish=!Diver->bEnglish; Diver->ControlMessage.Empty(); }
        else if(Id.ToString().StartsWith(TEXT("bind_"))) { Diver->CapturingAction=FCString::Atoi(*Id.ToString().Mid(5)); Diver->ControlMessage.Empty(); return; }
        else if(Id==TEXT("sensitivity_minus")) Diver->Controls.Sensitivity-=.03;
        else if(Id==TEXT("sensitivity_plus")) Diver->Controls.Sensitivity+=.03;
        else if(Id==TEXT("invert_y")) Diver->Controls.InvertY=!Diver->Controls.InvertY;
        else if(Id==TEXT("fov_minus")) Diver->Controls.FieldOfView-=5;
        else if(Id==TEXT("fov_plus")) Diver->Controls.FieldOfView+=5;
        Diver->ApplyControls(); Diver->SaveSettings(); return;
    }
    if(Id==TEXT("logbook")) { OpenLogbook(Diver->bSummary?Diver->LastLogPath:FString()); return; }
    if(Diver->bLogbook)
    {
        if(Id==TEXT("logphotos")&&LogbookReady()) { OpenDiscoveries(true);return; }
        if(Id==TEXT("logback")) Diver->bLogbook=false;
        else if(Id==TEXT("loglanguage")) { Diver->bEnglish=!Diver->bEnglish; Diver->SaveSettings(); }
        else if(Id==TEXT("logprev")) LogPage=FMath::Max(0,LogPage-1);
        else if(Id==TEXT("lognext")) LogPage=FMath::Min((VisibleLogs().Num()-1)/7,LogPage+1);
        else if(Id==TEXT("logfilter")&&!bIndexLoading&&!bDetailLoading)
        {
            LogFilter=(LogFilter+1)%4; LogPage=0;
            const auto visible=VisibleLogs();
            if(!visible.IsEmpty()) SelectLog(visible[0]);
            else { LogSelection=INDEX_NONE; SelectedLog=FDiveLog{}; }
        }
        else if(Id.ToString().StartsWith(TEXT("logrow_"))) SelectLog(FCString::Atoi(*Id.ToString().Mid(7)));
        else if(Id.ToString().StartsWith(TEXT("logmetric_"))) LogMetricGroup=FMath::Clamp(FCString::Atoi(*Id.ToString().Mid(10)),0,3);
        else if(Id==TEXT("logeventprev")||Id==TEXT("logeventnext"))
        {
            const int direction=Id==TEXT("logeventnext")?1:-1;
            for(int i=LogCursor+direction;SelectedLog.Samples.IsValidIndex(i);i+=direction)
            {
                const uint32 previous=i>0?SelectedLog.Samples[i-1].Warnings:0;
                if(SelectedLog.Samples[i].Warnings&~previous) { LogCursor=i; break; }
            }
        }
        return;
    }
    auto& c=Diver->PendingConfig;
    if(Id==TEXT("entry")) { OpenEntryMap(); return; }
    if(Id==TEXT("plan")) { bEnvironmentPage=bAudioPage=bLoadoutPage=bAvatarView=false; return; }
    if(Id==TEXT("environment")) { bEnvironmentPage=true; bAudioPage=bLoadoutPage=bAvatarView=false; return; }
    if(Id==TEXT("audio")) { bAudioPage=!bAudioPage; bEnvironmentPage=bLoadoutPage=bAvatarView=false; return; }
    if(Id==TEXT("loadout")) { bLoadoutPage=true; bAudioPage=bEnvironmentPage=false; return; }
    if(Id==TEXT("avatar_view")&&bLoadoutPage) { bAvatarView=!bAvatarView;return; }
    if(bLoadoutPage&&bAvatarView&&Diver->Equipment&&Diver->Equipment->Avatar)
    {
        auto* avatar=Diver->Equipment->Avatar.Get();
        if(Id==TEXT("avatar_left")) { avatar->InspectionYaw-=45;return; }
        if(Id==TEXT("avatar_right")) { avatar->InspectionYaw+=45;return; }
        if(Id==TEXT("avatar_zoom")) { avatar->bInspectionClose=!avatar->bInspectionClose;return; }
        if(Id==TEXT("avatar_pose")&&!Diver->bDiveActive) { avatar->bPreviewSwimming=!avatar->bPreviewSwimming;return; }
    }
    if(Id.ToString().StartsWith(TEXT("audio_minus_"))||Id.ToString().StartsWith(TEXT("audio_plus_")))
    {
        const FString name=Id.ToString();
        const int i=FCString::Atoi(*name.Right(1));
        if(i>=0&&i<5)
        {
            Diver->AudioLevels.Values[i]=FMath::Clamp(Diver->AudioLevels.Values[i]+(name.StartsWith(TEXT("audio_plus_"))?.05:-.05),0.,1.);
            Diver->Audio->ApplyLevels(Diver->AudioLevels); Diver->SaveSettings();
        }
        return;
    }
    if(Id==TEXT("start")) { if(Diver->bDiveActive) Diver->ResumeDive(); else Diver->StartDive(); return; }
    if(Id==TEXT("abort")) { Diver->FinishDive(TEXT("aborted")); return; }
    if(Id==TEXT("new")) { Diver->bSummary=false; return; }
    if(Id==TEXT("quit")) { UKismetSystemLibrary::QuitGame(this,GetOwningPlayerController(),EQuitPreference::Quit,false); return; }
    if(Id==TEXT("language")) Diver->bEnglish=!Diver->bEnglish;
    else if(Id==TEXT("motion")) Diver->bCameraMotion=!Diver->bCameraMotion;
    else if(Id==TEXT("quality"))
    {
        Diver->GraphicsQuality=FMath::Fmod(Diver->GraphicsQuality+1,4.f);
        if(auto* settings=GEngine->GetGameUserSettings())
        { settings->SetOverallScalabilityLevel(Diver->GraphicsQuality); settings->ApplyNonResolutionSettings(); settings->SaveSettings(); }
    }
    else if(!Diver->bDiveActive)
    {
        if(OpenConfigInput(Id)) return;
        if(Id==TEXT("mode")) c.GameMode=static_cast<Dive::Mode>((static_cast<int>(c.GameMode)+1)%3);
        else if(Id==TEXT("shore")) c.StartOnShore=!c.StartOnShore;
        else if(Id==TEXT("exploration_trim")&&c.GameMode==Dive::Mode::Exploration) c.ManualExplorationBuoyancy=!c.ManualExplorationBuoyancy;
        else if(Id==TEXT("tank")) c.CylinderLitres=c.CylinderLitres==10?12:c.CylinderLitres==12?15:10;
        else if(Id==TEXT("cylinder_material")) c.CylinderMaterial=1-c.CylinderMaterial;
        else if(Id==TEXT("bcd")) c.BCDModel=(c.BCDModel+1)%Dive::BCDEquipment::Count;
        else if(Id==TEXT("suit")) c.DrySuit=!c.DrySuit;
        else if(Id==TEXT("torch")) c.TorchModel=(c.TorchModel+1)%Dive::Torch::Count;
        else if(Id==TEXT("weather")) c.SelectWeather((c.Weather+1)%Dive::WeatherScenario::Count);
        else if(Id==TEXT("auto_water")) c.AutoTemperature=!c.AutoTemperature;
        else if(Id==TEXT("auto_visibility")) c.AutoVisibility=!c.AutoVisibility;
        else if(Id==TEXT("current_source")) c.CurrentSource=static_cast<Dive::CurrentSetting>((static_cast<int>(c.CurrentSource)+1)%5);
    }
    Diver->SaveSettings();
}

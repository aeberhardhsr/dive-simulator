#include "DiveHUD.h"
#include "DivePawn.h"

namespace
{
const FLinearColor White(.87f,.94f,.92f,1),Muted(.42f,.59f,.57f,1),Accent(.43f,.87f,.69f,1);
const FLinearColor Dark(.012f,.032f,.032f,.94f),Panel(.025f,.065f,.063f,.92f);
}
void ADiveHUD::OpenControls()
{
    Diver=Cast<ADivePawn>(GetOwningPawn()); if(!Diver) return;
    CloseEntryMap(); CloseProfileName(); bProfilesPage=false;
    Diver->SetMenu(true); bControlsPage=true; Diver->ControlMessage.Empty();
}
void ADiveHUD::ControlsMenu()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    Text(L(TEXT("DEINE STEUERUNG"),TEXT("YOUR CONTROLS")),120,70,38,White);
    Text(L(TEXT("Taste anklicken und neue Taste druecken. Belegte Tasten werden getauscht."),TEXT("Click a binding and press a new key. Occupied bindings are swapped.")),120,125,18,Muted);
    Text(Diver->CapturingAction!=INDEX_NONE?L(TEXT("Warte auf Taste ... ESC bricht ab. Linksklick, ESC, F1 und F11 bleiben reserviert."),TEXT("Waiting for a key ... ESC cancels. Left click, ESC, F1 and F11 stay reserved.")):Diver->ControlMessage,120,160,17,Accent);
    for(int i=0;i<FDiveControls::Count;++i)
    {
        const float x=120+(i/9)*720,y=210+(i%9)*48;
        Text(FDiveControls::Label(i,Diver->bEnglish),x,y+11,18,White);
        Button(FName(*FString::Printf(TEXT("bind_%d"),i)),Diver->CapturingAction==i?TEXT("..."):Diver->KeyText(static_cast<EDiveAction>(i)),x+340,y,280,43,Diver->CapturingAction==i);
    }
    Text(L(TEXT("MAUSEMPFINDLICHKEIT"),TEXT("MOUSE SENSITIVITY")),120,672,14,Muted);
    Button(TEXT("sensitivity_minus"),TEXT("-"),120,698,55,42);
    Text(FString::Printf(TEXT("%.2f"),Diver->Controls.Sensitivity),210,708,22,White);
    Button(TEXT("sensitivity_plus"),TEXT("+"),315,698,55,42);
    Text(L(TEXT("VERTIKALE MAUSRICHTUNG"),TEXT("VERTICAL MOUSE AXIS")),420,672,14,Muted);
    Button(TEXT("invert_y"),Diver->Controls.InvertY?L(TEXT("Invertiert"),TEXT("Inverted")):L(TEXT("Normal"),TEXT("Normal")),420,698,300,42);
    Text(L(TEXT("SICHTFELD"),TEXT("FIELD OF VIEW")),790,672,14,Muted);
    Button(TEXT("fov_minus"),TEXT("-"),790,698,55,42);
    Text(FString::Printf(TEXT("%.0f deg"),Diver->Controls.FieldOfView),880,708,22,White);
    Button(TEXT("fov_plus"),TEXT("+"),1010,698,55,42);
    Text(L(TEXT("Maus: umsehen. ESC: Auswahl abbrechen / zurueck. F1: diese Hilfe."),TEXT("Mouse: look around. ESC: cancel binding / back. F1: this help.")),120,765,17,Muted);
    Button(TEXT("controls_back"),L(TEXT("<  Zurueck"),TEXT("<  Back")),120,815,300,48,true);
    Button(TEXT("controls_reset"),L(TEXT("Standards wiederherstellen"),TEXT("Restore defaults")),460,815,355,48);
    if(!Diver->bDiveActive) Button(TEXT("tutorial"),L(TEXT("Einfuehrung starten"),TEXT("Start introduction")),855,815,330,48);
    Button(TEXT("controls_language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),1225,815,235,48);
}
void ADiveHUD::TutorialOverlay()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    auto K=[this](EDiveAction a){return TEXT("[")+Diver->KeyText(a)+TEXT("]");};
    const auto& t=Diver->Tutorial;
    FString title,first,second;
    switch(t.Stage)
    {
    case Dive::Lesson::Look:
        title=L(TEXT("Schau dich um"),TEXT("Look around"));
        first=L(TEXT("Bewege die Maus und suche die Einstiegsboje."),TEXT("Move the mouse and locate the entry buoy."));
        second=L(TEXT("Dies ist eine Einfuehrung in die Spielsteuerung."),TEXT("This introduction teaches the game controls.")); break;
    case Dive::Lesson::Swim:
        title=L(TEXT("Schwimme ein paar Meter"),TEXT("Swim a few metres"));
        first=K(EDiveAction::Forward)+TEXT(" / ")+K(EDiveAction::Backward)+L(TEXT(" vor / zurueck, "),TEXT(" forward / backward, "))+K(EDiveAction::Left)+TEXT(" / ")+K(EDiveAction::Right)+L(TEXT(" seitwaerts"),TEXT(" sideways"));
        second=FString::Printf(TEXT("%.1f / 3.0 m"),t.SwimMetres)+L(TEXT("   |   Du bewegst dich in Blickrichtung."),TEXT("   |   You move in the direction you look.")); break;
    case Dive::Lesson::Buoyancy:
        title=L(TEXT("Probiere das Jacket aus"),TEXT("Try the BCD controls"));
        first=t.InflatedLitres<1.5?K(EDiveAction::Inflate)+L(TEXT(" kurz halten: Jacket fuellen."),TEXT(" hold briefly: inflate the BCD.")):K(EDiveAction::Deflate)+L(TEXT(" kurz halten: Luft wieder ablassen."),TEXT(" hold briefly: release the added air."));
        second=L(TEXT("Beobachte Tiefe und BCD-Liter. Luft veraendert deinen Auftrieb."),TEXT("Watch depth and BCD litres. Air changes your buoyancy.")); break;
    case Dive::Lesson::Computer:
        title=L(TEXT("Lies deinen Tauchcomputer"),TEXT("Read your dive computer"));
        first=t.SawVitals?K(EDiveAction::Computer)+L(TEXT(" senkt deinen Arm wieder."),TEXT(" lowers your wrist again.")):K(EDiveAction::Computer)+L(TEXT(" hebt den Arm. "),TEXT(" raises your wrist. "))+K(EDiveAction::ComputerPage)+L(TEXT(" wechselt zur Vitalwerteseite."),TEXT(" switches to the vitals page."));
        second=L(TEXT("Tiefe, Flaschendruck und Zeit bleiben immer sichtbar."),TEXT("Depth, cylinder pressure and time stay visible.")); break;
    case Dive::Lesson::Lamp:
        title=L(TEXT("Teste die Lampe"),TEXT("Test your torch"));
        first=K(EDiveAction::Lamp)+L(TEXT(" schaltet die Lampe aus und wieder ein."),TEXT(" switches your torch off and back on."));
        second=L(TEXT("Der Lichtkegel folgt deiner Blickrichtung."),TEXT("The beam follows the direction you look.")); break;
    case Dive::Lesson::Map:
        title=L(TEXT("Orientiere dich im See"),TEXT("Find your position"));
        first=K(EDiveAction::Map)+L(TEXT(" oeffnet die Karte. Danach wieder schliessen."),TEXT(" opens the map. Close it again when ready."));
        second=L(TEXT("Gruen zeigt dich, Gelb die Spielstarts. Deine Spur bleibt sichtbar."),TEXT("Green marks you, yellow marks the starts. Your trail stays visible.")); break;
    case Dive::Lesson::Return:
        title=L(TEXT("Kehre zur Einstiegsboje zurueck"),TEXT("Return to the entry buoy"));
        first=K(EDiveAction::Ascend)+L(TEXT(" schwimmt nach oben, "),TEXT(" swims upward, "))+K(EDiveAction::Descend)+L(TEXT(" nach unten."),TEXT(" downward."));
        second=L(TEXT("Bleibe bei der Boje und erreiche weniger als 0,8 m Tiefe."),TEXT("Stay near the buoy and reach less than 0.8 m depth.")); break;
    case Dive::Lesson::Exit:
        title=L(TEXT("Schliesse den Uebungstauchgang ab"),TEXT("Finish the practice dive"));
        first=K(EDiveAction::Finish)+L(TEXT(" beendet den Tauchgang nahe der Boje an der Oberflaeche."),TEXT(" ends the dive at the surface near the buoy."));
        second=L(TEXT("Danach findest du dein Profil im Logbuch."),TEXT("Your dive profile will be available in the logbook.")); break;
    default: return;
    }
    // Keep the map readable while its lesson is active.
    const float y=Diver->bShowMap?20:140;
    Rect(40,y,940,146,Dark); Rect(40,y,4,146,Accent);
    Text(FString::Printf(TEXT("%d / 8   "),static_cast<int>(t.Stage)+1)+title,60,y+17,24,Accent);
    Text(first,60,y+60,18,White); Text(second,60,y+99,16,Muted);
}

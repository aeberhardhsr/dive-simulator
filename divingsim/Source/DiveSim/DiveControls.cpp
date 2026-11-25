#include "DiveControls.h"
#include "Dom/JsonObject.h"

FDiveControls::FDiveControls()
{
    Keys={EKeys::W,EKeys::S,EKeys::A,EKeys::D,EKeys::SpaceBar,EKeys::LeftControl,EKeys::LeftShift,
        EKeys::Q,EKeys::E,EKeys::L,EKeys::H,EKeys::C,EKeys::R,EKeys::M,EKeys::F,EKeys::P};
}
bool FDiveControls::Allowed(FKey Key)
{
    return Key.IsValid()&&Key.IsDigital()&&Key.IsBindableToActions()&&!Key.IsGamepadKey()&&!Key.IsTouch()
        &&Key!=EKeys::AnyKey&&Key!=EKeys::Escape&&Key!=EKeys::F1&&Key!=EKeys::F11&&Key!=EKeys::Tilde
        &&Key!=EKeys::LeftMouseButton&&Key!=EKeys::MouseScrollUp&&Key!=EKeys::MouseScrollDown
        &&Key!=EKeys::LeftAlt&&Key!=EKeys::RightAlt&&Key!=EKeys::LeftCommand&&Key!=EKeys::RightCommand;
}
int FDiveControls::Assign(int Action,FKey NewKey)
{
    if(!Keys.IsValidIndex(Action)||!Allowed(NewKey)) return -2;
    const int other=Find(NewKey);
    if(other==Action) return INDEX_NONE;
    if(other!=INDEX_NONE) Keys[other]=Keys[Action];
    Keys[Action]=NewKey; return other;
}
FString FDiveControls::Id(int Action)
{
    static const TCHAR* ids[]={TEXT("forward"),TEXT("backward"),TEXT("left"),TEXT("right"),TEXT("ascend"),TEXT("descend"),TEXT("sprint"),TEXT("inflate"),TEXT("deflate"),TEXT("lamp"),TEXT("hud"),TEXT("computer"),TEXT("computer_page"),TEXT("map"),TEXT("finish"),TEXT("photo")};
    return Action>=0&&Action<Count?ids[Action]:TEXT("");
}
FString FDiveControls::Label(int Action,bool English)
{
    static const TCHAR* de[]={TEXT("Vorwaerts schwimmen"),TEXT("Rueckwaerts schwimmen"),TEXT("Nach links"),TEXT("Nach rechts"),TEXT("Nach oben"),TEXT("Nach unten"),TEXT("Kraeftiger Flossenschlag"),TEXT("Jacket fuellen"),TEXT("Jacket entleeren"),TEXT("Lampe"),TEXT("Instrumente ein / aus"),TEXT("Handgelenkcomputer"),TEXT("Naechste Computerseite"),TEXT("Karte"),TEXT("Tauchgang abschliessen"),TEXT("Foto aufnehmen")};
    static const TCHAR* en[]={TEXT("Swim forward"),TEXT("Swim backward"),TEXT("Move left"),TEXT("Move right"),TEXT("Move up"),TEXT("Move down"),TEXT("Strong fin kick"),TEXT("Inflate BCD"),TEXT("Deflate BCD"),TEXT("Torch"),TEXT("Toggle instruments"),TEXT("Wrist computer"),TEXT("Next computer page"),TEXT("Map"),TEXT("Finish dive"),TEXT("Take photograph")};
    return Action>=0&&Action<Count?(English?en[Action]:de[Action]):TEXT("");
}
FString FDiveControls::KeyText(EDiveAction Action,bool English) const
{
    const FKey key=Key(Action);
    if(key==EKeys::SpaceBar) return English?TEXT("Space"):TEXT("Leertaste");
    if(key==EKeys::LeftControl) return English?TEXT("L Ctrl"):TEXT("L Strg");
    if(key==EKeys::RightControl) return English?TEXT("R Ctrl"):TEXT("R Strg");
    if(key==EKeys::LeftShift) return TEXT("L Shift");
    if(key==EKeys::RightShift) return TEXT("R Shift");
    if(key==EKeys::RightMouseButton) return English?TEXT("Mouse 2"):TEXT("Maus 2");
    if(key==EKeys::MiddleMouseButton) return English?TEXT("Mouse 3"):TEXT("Maus 3");
    if(key==EKeys::ThumbMouseButton) return English?TEXT("Mouse 4"):TEXT("Maus 4");
    if(key==EKeys::ThumbMouseButton2) return English?TEXT("Mouse 5"):TEXT("Maus 5");
    return key.GetDisplayName().ToString();
}
void FDiveControls::Sanitize()
{
    Sensitivity=FMath::IsFinite(Sensitivity)?FMath::Clamp(Sensitivity,.03,.5):.12;
    FieldOfView=FMath::IsFinite(FieldOfView)?FMath::Clamp(FieldOfView,60.,110.):90;
}
void FDiveControls::Write(const TSharedPtr<FJsonObject>& Root) const
{
    auto bindings=MakeShared<FJsonObject>();
    for(int i=0;i<Count;++i) bindings->SetStringField(Id(i),Keys[i].GetFName().ToString());
    Root->SetObjectField(TEXT("bindings"),bindings);
    Root->SetNumberField(TEXT("mouse_sensitivity"),Sensitivity);
    Root->SetNumberField(TEXT("field_of_view"),FieldOfView);
    Root->SetBoolField(TEXT("invert_y"),InvertY);
}
void FDiveControls::Read(const TSharedPtr<FJsonObject>& Root)
{
    *this=FDiveControls{};
    const TSharedPtr<FJsonObject>* bindings=nullptr;
    if(Root->TryGetObjectField(TEXT("bindings"),bindings)&&bindings&&*bindings)
    {
        TSet<FKey> requested;
        for(int i=0;i<Count;++i)
        {
            FString value;
            if((*bindings)->TryGetStringField(Id(i),value)&&value.Len()<64)
            {
                const FKey key{FName(*value)};
                if(Allowed(key)&&!requested.Contains(key)) { Assign(i,key); requested.Add(key); }
            }
        }
    }
    Root->TryGetNumberField(TEXT("mouse_sensitivity"),Sensitivity);
    Root->TryGetNumberField(TEXT("field_of_view"),FieldOfView);
    Root->TryGetBoolField(TEXT("invert_y"),InvertY); Sanitize();
}

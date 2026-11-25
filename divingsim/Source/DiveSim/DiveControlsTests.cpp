#include "DiveControls.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveBindingPersistence,"DiveSim.Controls.BindingPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveBindingPersistence::RunTest(const FString&)
{
    FDiveControls controls;
    TestEqual(TEXT("Conflicting key reports swapped action"),controls.Assign(0,EKeys::S),1);
    TestEqual(TEXT("Forward now uses S"),controls.Key(EDiveAction::Forward),EKeys::S);
    TestEqual(TEXT("Backward receives old W"),controls.Key(EDiveAction::Backward),EKeys::W);
    controls.Assign(9,EKeys::ThumbMouseButton);
    controls.Sensitivity=.27; controls.FieldOfView=105; controls.InvertY=true;
    auto root=MakeShared<FJsonObject>(); controls.Write(root);
    FString json; FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json));
    TSharedPtr<FJsonObject> decoded; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),decoded);
    FDiveControls restored; restored.Read(decoded);
    TestTrue(TEXT("All bindings survive JSON roundtrip"),restored.Keys==controls.Keys);
    TestEqual(TEXT("Sensitivity survives"),restored.Sensitivity,.27);
    TestEqual(TEXT("FOV survives"),restored.FieldOfView,105.);
    TestTrue(TEXT("Inversion survives"),restored.InvertY);
    TestEqual(TEXT("Localized mouse hint"),restored.KeyText(EDiveAction::Lamp,false),FString(TEXT("Maus 4")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveBindingValidation,"DiveSim.Controls.InvalidAndLegacySettings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveBindingValidation::RunTest(const FString&)
{
    FDiveControls controls;
    for(FKey key:{EKeys::Escape,EKeys::F1,EKeys::F11,EKeys::Tilde,EKeys::LeftMouseButton,EKeys::MouseX,EKeys::Gamepad_LeftX,FKey(FName(TEXT("InventedKey")))})
        TestEqual(TEXT("Reserved or unsupported input is rejected"),controls.Assign(0,key),-2);
    TestEqual(TEXT("Rejected binding leaves original intact"),controls.Key(EDiveAction::Forward),EKeys::W);
    auto root=MakeShared<FJsonObject>(); auto bindings=MakeShared<FJsonObject>();
    bindings->SetStringField(TEXT("forward"),TEXT("I")); bindings->SetStringField(TEXT("backward"),TEXT("I"));
    bindings->SetStringField(TEXT("lamp"),TEXT("Escape")); bindings->SetStringField(TEXT("map"),TEXT("NoSuchKey"));
    root->SetObjectField(TEXT("bindings"),bindings);
    root->SetNumberField(TEXT("mouse_sensitivity"),100); root->SetNumberField(TEXT("field_of_view"),-1);
    controls.Read(root);
    TSet<FKey> unique; for(FKey key:controls.Keys) { TestTrue(TEXT("Loaded key is usable"),FDiveControls::Allowed(key)); unique.Add(key); }
    TestEqual(TEXT("Corrupt duplicate map keeps every action unique"),unique.Num(),FDiveControls::Count);
    TestEqual(TEXT("Sensitivity bounded"),controls.Sensitivity,.5);
    TestEqual(TEXT("FOV bounded"),controls.FieldOfView,60.);
    controls.Read(MakeShared<FJsonObject>());
    TestTrue(TEXT("Legacy settings without controls use defaults"),controls.Keys==FDiveControls{}.Keys);
    return true;
}
#endif

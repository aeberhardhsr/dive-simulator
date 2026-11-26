#include "DivePawn.h"
#include "DiveWorld.h"
#include "Camera/CameraComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"

void ADivePawn::Photograph()
{
    if(bMenu||!bDiveActive||!Environment) return;
    if(PhotoPending()||FScreenshotRequest::IsScreenshotRequested())
    { Notify(Local(TEXT("Die vorherige Aufnahme wird noch gespeichert."),TEXT("The previous capture is still being saved.")));return; }
    if(Photographs.Num()>=DiveDiscoveries::PhotoLimit)
    { Notify(Local(TEXT("Fotoalbum fuer diesen Tauchgang voll (1000)."),TEXT("This dive's photo album is full (1000).")));return; }
    FDivePhotograph photo;photo.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    photo.UTC=FDateTime::UtcNow().ToIso8601();photo.Seconds=Simulation.Data.ElapsedSeconds;photo.Depth=Simulation.Data.DepthMetres;
    photo.Species=Environment->PhotographSpecies(Camera->GetComponentLocation(),Camera->GetForwardVector());
    const FString path=DiveDiscoveries::PhotoPath(photo.Id,bSmokeTest);
    if(!IFileManager::Get().MakeDirectory(*FPaths::GetPath(path),true))
    { Notify(Local(TEXT("Fotoordner konnte nicht angelegt werden."),TEXT("Could not create the photograph directory.")));return; }
    if(!PhotoProcessedHandle.IsValid()) PhotoProcessedHandle=FScreenshotRequest::OnScreenshotRequestProcessed().AddUObject(this,&ADivePawn::PhotoProcessed);
    PendingPhoto=MoveTemp(photo);
    // Canvas and Slate overlays are both hidden. The physical equipment stays visible.
    FScreenshotRequest::RequestScreenshot(path,false,false);
}

void ADivePawn::PhotoProcessed()
{
    if(!PendingPhoto) return;
    const auto photo=PendingPhoto.GetValue();PendingPhoto.Reset();
    const FString path=DiveDiscoveries::PhotoPath(photo.Id,bSmokeTest);
    if(IFileManager::Get().FileSize(*path)<=24)
    { Notify(Local(TEXT("Foto konnte nicht gespeichert werden."),TEXT("The photograph could not be saved.")));return; }
    Photographs.Add(photo);
    // Finalization can happen between the request and the render callback. Append
    // to that same dive, never to a new one (StartDive is guarded while pending).
    const bool saved=SaveLog(bDiveActive?TEXT("in_progress"):Outcome);
    Notify(saved?Local(TEXT("Foto gespeichert | Arten / Gruppen: "),TEXT("Photo saved | species / groups: "))+FString::FromInt(photo.Species.Num()):
        Local(TEXT("Foto gespeichert; Logbuch konnte nicht aktualisiert werden."),TEXT("Photo saved; the logbook could not be updated.")));
    if(bSmokeTest) UE_LOG(LogTemp,Display,TEXT("DIVE_PHOTO_SAVED id=%s species=%d path=%s"),*photo.Id,photo.Species.Num(),*FPaths::ConvertRelativePathToFull(path));
}

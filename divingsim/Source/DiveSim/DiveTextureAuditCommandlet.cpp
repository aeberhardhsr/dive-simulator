#include "DiveTextureAuditCommandlet.h"

#if WITH_EDITOR
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#endif

UDiveTextureAuditCommandlet::UDiveTextureAuditCommandlet()
{
    IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true;
}

int32 UDiveTextureAuditCommandlet::Main(const FString& Params)
{
#if !WITH_EDITOR
    return 1;
#else
    FString output;
    if (!FParse::Value(*Params,TEXT("Output="),output)) return 1;
    TArray<TSharedPtr<FJsonValue>> records;
    const bool requireEncoding=FParse::Param(*Params,TEXT("RequireSourceEncoding"));
    bool passed=true;
    for (const FString& prefix : {FString(TEXT("T_Bed_Sand")), FString(TEXT("T_Bed_Gravel")),
        FString(TEXT("T_Bed_Rock")), FString(TEXT("T_RockScan1_")),
        FString(TEXT("T_RockScan2_")), FString(TEXT("T_RockScan3_"))})
    {
        for (const FString& kind : {FString(TEXT("Color")), FString(TEXT("Normal")), FString(TEXT("ARM"))})
        {
            const FString suffix=prefix.StartsWith(TEXT("T_RockScan")) && kind==TEXT("Normal") ? TEXT("ScanNormal") : kind;
            const FString path=TEXT("/Game/Diving/Textures/")+prefix+suffix;
            UTexture2D* texture=LoadObject<UTexture2D>(nullptr,*path);
            if (!texture) { UE_LOG(LogTemp,Error,TEXT("Missing %s"),*path); return 1; }
            FTextureCompilingManager::Get().FinishCompilation({texture});
            const EPixelFormat format=texture->GetPixelFormat();
            const bool color=kind==TEXT("Color"), normal=kind==TEXT("Normal");
            // This project's DirectX normal sampling explicitly assumes BC5.
            // Colour formats must support hardware sRGB decoding on Windows.
            const bool formatOK=normal ? format==PF_BC5 :
                (format==PF_DXT1 || format==PF_DXT5 || format==PF_BC7 || format==PF_B8G8R8A8 || format==PF_R8G8B8A8);
            const bool encodingOK=texture->SourceColorSettings.EncodingOverride==
                (color ? ETextureSourceEncoding::TSE_sRGB : ETextureSourceEncoding::TSE_Linear);
            const bool valid=formatOK && bool(texture->SRGB)==color && (!requireEncoding || encodingOK) &&
                texture->GetSizeX()==2048 && texture->GetSizeY()==2048 && texture->GetNumMips()==12;
            auto record=MakeShared<FJsonObject>();
            record->SetStringField(TEXT("asset"),path);
            record->SetNumberField(TEXT("source_format"),int32(texture->Source.GetFormat()));
            record->SetNumberField(TEXT("source_encoding"),int32(texture->SourceColorSettings.EncodingOverride));
            if (texture->Source.GetFormat()==TSF_RGBA16)
            {
                const uint16* pixels=reinterpret_cast<const uint16*>(texture->Source.LockMipReadOnly(0));
                const int64 center=(int64(texture->Source.GetSizeY()/2)*texture->Source.GetSizeX()+texture->Source.GetSizeX()/2)*4;
                TArray<TSharedPtr<FJsonValue>> sample;
                for (int c=0;c<3;++c) sample.Add(MakeShared<FJsonValueNumber>(pixels[center+c]/65535.0));
                texture->Source.UnlockMip(0);
                record->SetArrayField(TEXT("source_center_rgb"),sample);
            }
            record->SetStringField(TEXT("platform_format"),GetPixelFormatString(format));
            record->SetBoolField(TEXT("srgb"),texture->SRGB);
            record->SetNumberField(TEXT("width"),texture->GetSizeX());
            record->SetNumberField(TEXT("height"),texture->GetSizeY());
            record->SetNumberField(TEXT("mips"),texture->GetNumMips());
            record->SetBoolField(TEXT("pass"),valid);
            records.Add(MakeShared<FJsonValueObject>(record));
            passed &= valid;
            UE_LOG(LogTemp,Display,TEXT("DIVE_TEXTURE_FORMAT %s source=%d platform=%s srgb=%d mips=%d pass=%d"),
                *path,int32(texture->Source.GetFormat()),GetPixelFormatString(format),texture->SRGB,texture->GetNumMips(),valid);
        }
    }
    auto report=MakeShared<FJsonObject>();
    report->SetBoolField(TEXT("pass"),passed);
    report->SetBoolField(TEXT("require_source_encoding"),requireEncoding);
    report->SetArrayField(TEXT("textures"),records);
    FString json;
    if (!FJsonSerializer::Serialize(report,TJsonWriterFactory<>::Create(&json)) ||
        !FFileHelper::SaveStringToFile(json,*output)) return 1;
    UE_LOG(LogTemp,Display,TEXT("DIVE_TEXTURE_AUDIT_%s count=%d"),passed ? TEXT("PASS") : TEXT("FAIL"),records.Num());
    return passed ? 0 : 1;
#endif
}

#include "Modules/ModuleManager.h"

#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Editor/EditorEngine.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/SoftObjectPath.h"

#include "Editor.h"

namespace
{
    constexpr uint32 BridgePort = 8765;

    TSharedPtr<IHttpRouter> Router;
    FHttpRouteHandle HealthRoute;
    FHttpRouteHandle CommandRoute;

    FString SerializeJson(const TSharedRef<FJsonObject>& JsonObject)
    {
        FString Output;
        const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
        FJsonSerializer::Serialize(JsonObject, Writer);
        return Output;
    }

    TUniquePtr<FHttpServerResponse> JsonResponse(
        const TSharedRef<FJsonObject>& JsonObject)
    {
        return FHttpServerResponse::Create(
            SerializeJson(JsonObject),
            TEXT("application/json; charset=utf-8"));
    }

    TSharedRef<FJsonObject> OkJson(const FString& Message)
    {
        const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
        Json->SetBoolField(TEXT("ok"), true);
        Json->SetStringField(TEXT("message"), Message);
        return Json;
    }

    bool ReadVector(
        const TSharedPtr<FJsonObject>& Json,
        const TCHAR* FieldName,
        FVector& OutVector)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Json->TryGetArrayField(FieldName, Values) || Values == nullptr || Values->Num() < 3)
        {
            return false;
        }

        OutVector.X = (*Values)[0]->AsNumber();
        OutVector.Y = (*Values)[1]->AsNumber();
        OutVector.Z = (*Values)[2]->AsNumber();
        return true;
    }

    bool ReadRotator(
        const TSharedPtr<FJsonObject>& Json,
        const TCHAR* FieldName,
        FRotator& OutRotation)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Json->TryGetArrayField(FieldName, Values) || Values == nullptr || Values->Num() < 3)
        {
            return false;
        }

        OutRotation.Pitch = (*Values)[0]->AsNumber();
        OutRotation.Yaw = (*Values)[1]->AsNumber();
        OutRotation.Roll = (*Values)[2]->AsNumber();
        return true;
    }

    AActor* FindActorByLabel(const FString& Label)
    {
        if (GEditor == nullptr)
        {
            return nullptr;
        }

        UEditorActorSubsystem* ActorSubsystem =
            GEditor->GetEditorSubsystem<UEditorActorSubsystem>();

        if (ActorSubsystem == nullptr)
        {
            return nullptr;
        }

        for (AActor* Actor : ActorSubsystem->GetAllLevelActors())
        {
            if (Actor == nullptr)
            {
                continue;
            }

            if (Actor->GetActorLabel().Equals(Label, ESearchCase::IgnoreCase) ||
                Actor->GetName().Equals(Label, ESearchCase::IgnoreCase))
            {
                return Actor;
            }
        }

        return nullptr;
    }

    FString PrimitiveAssetPath(const FString& Primitive)
    {
        if (Primitive.Equals(TEXT("Cube"), ESearchCase::IgnoreCase))
        {
            return TEXT("/Engine/BasicShapes/Cube.Cube");
        }

        if (Primitive.Equals(TEXT("Sphere"), ESearchCase::IgnoreCase))
        {
            return TEXT("/Engine/BasicShapes/Sphere.Sphere");
        }

        if (Primitive.Equals(TEXT("Cylinder"), ESearchCase::IgnoreCase))
        {
            return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
        }

        if (Primitive.Equals(TEXT("Plane"), ESearchCase::IgnoreCase))
        {
            return TEXT("/Engine/BasicShapes/Plane.Plane");
        }

        return FString();
    }

    TSharedRef<FJsonObject> CreatePrimitive(const TSharedPtr<FJsonObject>& Json)
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();

        if (GEditor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("GEditor is unavailable."));
            return Result;
        }

        UEditorActorSubsystem* ActorSubsystem =
            GEditor->GetEditorSubsystem<UEditorActorSubsystem>();

        if (ActorSubsystem == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Editor Actor Subsystem is unavailable."));
            return Result;
        }

        FString Primitive = TEXT("Cube");
        Json->TryGetStringField(TEXT("primitive"), Primitive);

        const FString AssetPath = PrimitiveAssetPath(Primitive);
        if (AssetPath.IsEmpty())
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(
                TEXT("error"),
                TEXT("Supported primitives: Cube, Sphere, Cylinder, Plane."));
            return Result;
        }

        FVector Location(0.0, 0.0, 100.0);
        FVector Scale(1.0, 1.0, 1.0);
        FRotator Rotation(0.0, 0.0, 0.0);

        ReadVector(Json, TEXT("location"), Location);
        ReadVector(Json, TEXT("scale"), Scale);
        ReadRotator(Json, TEXT("rotation"), Rotation);

        AActor* NewActor = ActorSubsystem->SpawnActorFromClass(
            AStaticMeshActor::StaticClass(),
            Location,
            Rotation,
            false);

        if (NewActor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Failed to spawn the actor."));
            return Result;
        }

        AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(NewActor);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *AssetPath);

        if (MeshActor == nullptr || Mesh == nullptr)
        {
            ActorSubsystem->DestroyActor(NewActor);
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Primitive mesh could not be loaded."));
            return Result;
        }

        MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        MeshActor->SetActorScale3D(Scale);

        FString Label;
        if (!Json->TryGetStringField(TEXT("name"), Label) || Label.TrimStartAndEnd().IsEmpty())
        {
            Label = Primitive + TEXT("_Venky");
        }

        MeshActor->SetActorLabel(Label);
        MeshActor->Modify();
        MeshActor->MarkPackageDirty();

        Result->SetBoolField(TEXT("ok"), true);
        Result->SetStringField(TEXT("action"), TEXT("create_actor"));
        Result->SetStringField(TEXT("primitive"), Primitive);
        Result->SetStringField(TEXT("name"), MeshActor->GetActorLabel());
        Result->SetStringField(TEXT("object_name"), MeshActor->GetName());
        Result->SetStringField(TEXT("class"), MeshActor->GetClass()->GetPathName());

        return Result;
    }

    TSharedRef<FJsonObject> MoveActor(const TSharedPtr<FJsonObject>& Json)
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();

        FString Name;
        if (!Json->TryGetStringField(TEXT("name"), Name) || Name.TrimStartAndEnd().IsEmpty())
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Missing actor name."));
            return Result;
        }

        AActor* Actor = FindActorByLabel(Name);
        if (Actor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Actor not found: ") + Name);
            return Result;
        }

        FVector Location = Actor->GetActorLocation();
        ReadVector(Json, TEXT("location"), Location);

        FVector Scale = Actor->GetActorScale3D();
        ReadVector(Json, TEXT("scale"), Scale);

        FRotator Rotation = Actor->GetActorRotation();
        ReadRotator(Json, TEXT("rotation"), Rotation);

        const FTransform Transform(Rotation, Location, Scale);

        if (GEditor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("GEditor is unavailable."));
            return Result;
        }

        UEditorActorSubsystem* ActorSubsystem =
            GEditor->GetEditorSubsystem<UEditorActorSubsystem>();

        const bool bMoved =
            ActorSubsystem != nullptr &&
            ActorSubsystem->SetActorTransform(Actor, Transform);

        if (!bMoved)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Failed to move actor."));
            return Result;
        }

        Actor->MarkPackageDirty();

        Result->SetBoolField(TEXT("ok"), true);
        Result->SetStringField(TEXT("action"), TEXT("move_actor"));
        Result->SetStringField(TEXT("name"), Actor->GetActorLabel());
        return Result;
    }

    TSharedRef<FJsonObject> DeleteActor(const TSharedPtr<FJsonObject>& Json)
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();

        FString Name;
        if (!Json->TryGetStringField(TEXT("name"), Name) || Name.TrimStartAndEnd().IsEmpty())
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Missing actor name."));
            return Result;
        }

        AActor* Actor = FindActorByLabel(Name);
        if (Actor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Actor not found: ") + Name);
            return Result;
        }

        if (GEditor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("GEditor is unavailable."));
            return Result;
        }

        UEditorActorSubsystem* ActorSubsystem =
            GEditor->GetEditorSubsystem<UEditorActorSubsystem>();

        const bool bDeleted =
            ActorSubsystem != nullptr && ActorSubsystem->DestroyActor(Actor);

        Result->SetBoolField(TEXT("ok"), bDeleted);
        Result->SetStringField(TEXT("action"), TEXT("delete_actor"));
        Result->SetStringField(TEXT("name"), Name);

        if (!bDeleted)
        {
            Result->SetStringField(TEXT("error"), TEXT("Failed to delete actor."));
        }

        return Result;
    }

    TSharedRef<FJsonObject> ListActors()
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        const TArray<TSharedPtr<FJsonValue>> ActorValues;

        TArray<TSharedPtr<FJsonValue>> MutableActorValues;

        if (GEditor != nullptr)
        {
            if (UEditorActorSubsystem* ActorSubsystem =
                    GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
            {
                for (AActor* Actor : ActorSubsystem->GetAllLevelActors())
                {
                    if (Actor == nullptr)
                    {
                        continue;
                    }

                    const TSharedRef<FJsonObject> ActorJson = MakeShared<FJsonObject>();
                    ActorJson->SetStringField(TEXT("name"), Actor->GetActorLabel());
                    ActorJson->SetStringField(TEXT("object_name"), Actor->GetName());
                    ActorJson->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

                    const FVector Location = Actor->GetActorLocation();
                    TArray<TSharedPtr<FJsonValue>> LocationValues;
                    LocationValues.Add(MakeShared<FJsonValueNumber>(Location.X));
                    LocationValues.Add(MakeShared<FJsonValueNumber>(Location.Y));
                    LocationValues.Add(MakeShared<FJsonValueNumber>(Location.Z));
                    ActorJson->SetArrayField(TEXT("location"), LocationValues);

                    MutableActorValues.Add(MakeShared<FJsonValueObject>(ActorJson));
                }
            }
        }

        Result->SetBoolField(TEXT("ok"), true);
        Result->SetArrayField(TEXT("actors"), MutableActorValues);
        Result->SetNumberField(TEXT("count"), MutableActorValues.Num());
        return Result;
    }

    TSharedRef<FJsonObject> PlayEditor()
    {
        const TSharedRef<FJsonObject> Result = OkJson(TEXT("Play in Editor requested."));

        if (GEditor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("GEditor is unavailable."));
            return Result;
        }

        GEditor->PlayMap(nullptr, nullptr, -1, 0, false);
        Result->SetStringField(TEXT("action"), TEXT("play"));
        return Result;
    }

    TSharedRef<FJsonObject> StopEditor()
    {
        const TSharedRef<FJsonObject> Result = OkJson(TEXT("Stop Play in Editor requested."));

        if (GEditor == nullptr)
        {
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("GEditor is unavailable."));
            return Result;
        }

        GEditor->RequestEndPlayMap();
        Result->SetStringField(TEXT("action"), TEXT("stop"));
        return Result;
    }

    TSharedRef<FJsonObject> HandleCommandBody(const FString& Body)
    {
        TSharedPtr<FJsonObject> Json;
        const TSharedRef<TJsonReader<>> Reader =
            TJsonReaderFactory<>::Create(Body);

        if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
        {
            const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Invalid JSON command."));
            return Result;
        }

        FString Action;
        if (!Json->TryGetStringField(TEXT("action"), Action))
        {
            const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), false);
            Result->SetStringField(TEXT("error"), TEXT("Missing action."));
            return Result;
        }

        Action.TrimStartAndEndInline();

        if (Action.Equals(TEXT("health"), ESearchCase::IgnoreCase))
        {
            const TSharedRef<FJsonObject> Result = OkJson(TEXT("Venky Unreal Bridge is connected."));
            Result->SetStringField(TEXT("action"), TEXT("health"));
            Result->SetNumberField(TEXT("port"), BridgePort);
            return Result;
        }

        if (Action.Equals(TEXT("create_actor"), ESearchCase::IgnoreCase) ||
            Action.Equals(TEXT("create_primitive"), ESearchCase::IgnoreCase))
        {
            return CreatePrimitive(Json);
        }

        if (Action.Equals(TEXT("move_actor"), ESearchCase::IgnoreCase))
        {
            return MoveActor(Json);
        }

        if (Action.Equals(TEXT("delete_actor"), ESearchCase::IgnoreCase))
        {
            return DeleteActor(Json);
        }

        if (Action.Equals(TEXT("list_actors"), ESearchCase::IgnoreCase))
        {
            return ListActors();
        }

        if (Action.Equals(TEXT("play"), ESearchCase::IgnoreCase))
        {
            return PlayEditor();
        }

        if (Action.Equals(TEXT("stop"), ESearchCase::IgnoreCase))
        {
            return StopEditor();
        }

        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), false);
        Result->SetStringField(TEXT("error"), TEXT("Unknown action: ") + Action);
        return Result;
    }

    bool HandleHealth(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete)
    {
        const TSharedRef<FJsonObject> Json = OkJson(TEXT("Venky Unreal Bridge is running."));
        Json->SetStringField(TEXT("service"), TEXT("VenkyUnrealBridge"));
        Json->SetStringField(TEXT("editor"), TEXT("Unreal Editor"));
        Json->SetNumberField(TEXT("port"), BridgePort);

        OnComplete(JsonResponse(Json));
        return true;
    }

    bool HandleCommand(
        const FHttpServerRequest& Request,
        const FHttpResultCallback& OnComplete)
    {
        FString Body;
        if (Request.Body.Num() > 0)
        {
            Body = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Request.Body.GetData())));
        }

        const TSharedRef<FJsonObject> Json = HandleCommandBody(Body);
        OnComplete(JsonResponse(Json));
        return true;
    }
}

class FVenkyUnrealBridgeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FHttpServerModule& HttpServer = FHttpServerModule::Get();
        Router = HttpServer.GetHttpRouter(BridgePort, false);

        if (!Router.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("VenkyUnrealBridge: failed to bind port %u."), BridgePort);
            return;
        }

        HealthRoute = Router->BindRoute(
            FHttpPath(TEXT("/health")),
            EHttpServerRequestVerbs::VERB_GET,
            FHttpRequestHandler::CreateStatic(&HandleHealth));

        CommandRoute = Router->BindRoute(
            FHttpPath(TEXT("/command")),
            EHttpServerRequestVerbs::VERB_POST,
            FHttpRequestHandler::CreateStatic(&HandleCommand));

        HttpServer.StartAllListeners();

        UE_LOG(LogTemp, Log, TEXT("VenkyUnrealBridge: http://127.0.0.1:%u"), BridgePort);
    }

    virtual void ShutdownModule() override
    {
        if (Router.IsValid())
        {
            if (HealthRoute.IsValid())
            {
                Router->UnbindRoute(HealthRoute);
                HealthRoute.Reset();
            }

            if (CommandRoute.IsValid())
            {
                Router->UnbindRoute(CommandRoute);
                CommandRoute.Reset();
            }
        }

        if (FHttpServerModule::IsAvailable())
        {
            FHttpServerModule::Get().StopAllListeners();
        }

        Router.Reset();
    }
};

IMPLEMENT_MODULE(FVenkyUnrealBridgeModule, VenkyUnrealBridge)

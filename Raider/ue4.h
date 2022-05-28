#pragma once

#include "native.h"

constexpr auto PI = 3.1415926535897932f;
constexpr auto INV_PI = 0.31830988618f;
constexpr auto HALF_PI = 1.57079632679f;

inline bool bTraveled = false;
inline bool bPlayButton = false;
inline bool bListening = false;

inline UWorld* GetWorld()
{
    return GetEngine()->GameViewport->World;
    // return *(UWorld**)(Offsets::Imagebase + Offsets::GWorldOffset);
}

inline AAthena_PlayerController_C* GetPlayerController(int32 Index = 0)
{
    if (Index > GetWorld()->OwningGameInstance->LocalPlayers.Num())
    {
        std::cout << "WARNING! PlayerController out of range! (" << Index << " out of " << GetWorld()->OwningGameInstance->LocalPlayers.Num() << ")" << '\n';

        return (AAthena_PlayerController_C*)GetWorld()->OwningGameInstance->LocalPlayers[0]->PlayerController;
    }

    return (AAthena_PlayerController_C*)GetWorld()->OwningGameInstance->LocalPlayers[Index]->PlayerController;
}

struct FNetworkObjectInfo
{
    AActor* Actor;

    TWeakObjectPtr<AActor> WeakActor;

    double NextUpdateTime;

    double LastNetReplicateTime;

    float OptimalNetUpdateDelta;

    float LastNetUpdateTime;

    uint32 bPendingNetUpdate : 1;

    uint32 bForceRelevantNextUpdate : 1;

    TSet<TWeakObjectPtr<UNetConnection>> DormantConnections;

    TSet<TWeakObjectPtr<UNetConnection>> RecentlyDormantConnections;
};

class FNetworkObjectList
{
public:
    typedef TSet<TSharedPtr<FNetworkObjectInfo>> FNetworkObjectSet;

    FNetworkObjectSet AllNetworkObjects;
    FNetworkObjectSet ActiveNetworkObjects;
    FNetworkObjectSet ObjectsDormantOnAllConnections;

    TMap<TWeakObjectPtr<UObject>, int32> NumDormantObjectsPerConnection;
};

FORCEINLINE int32& GetReplicationFrame(UNetDriver* Driver)
{
    return *(int32*)(int64(Driver) + 816); // Offsets::Net::ReplicationFrame);
}

FORCEINLINE auto& GetNetworkObjectList(UObject* NetDriver)
{
    return *(*(TSharedPtr<FNetworkObjectList>*)(int64(NetDriver) + 0x508));
}

FORCEINLINE UGameplayStatics* GetGameplayStatics()
{
    return reinterpret_cast<UGameplayStatics*>(UGameplayStatics::StaticClass());
}

FORCEINLINE UKismetSystemLibrary* GetKismetSystem()
{
    return reinterpret_cast<UKismetSystemLibrary*>(UKismetSystemLibrary::StaticClass());
}

FORCEINLINE UKismetStringLibrary* GetKismetString()
{
    return (UKismetStringLibrary*)UKismetStringLibrary::StaticClass();
}

static FORCEINLINE void sinCos(float* ScalarSin, float* ScalarCos, float Value)
{
    float quotient = (INV_PI * 0.5f) * Value;
    if (Value >= 0.0f)
    {
        quotient = (float)((int)(quotient + 0.5f));
    }
    else
    {
        quotient = (float)((int)(quotient - 0.5f));
    }
    float y = Value - (2.0f * PI) * quotient;

    float sign;
    if (y > HALF_PI)
    {
        y = PI - y;
        sign = -1.0f;
    }
    else if (y < -HALF_PI)
    {
        y = -PI - y;
        sign = -1.0f;
    }
    else
    {
        sign = +1.0f;
    }

    float y2 = y * y;

    *ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

    float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
    *ScalarCos = sign * p;
}

static auto RotToQuat(FRotator Rotator)
{
    const float DEG_TO_RAD = PI / (180.f);
    const float DIVIDE_BY_2 = DEG_TO_RAD / 2.f;
    float SP, SY, SR;
    float CP, CY, CR;

    sinCos(&SP, &CP, Rotator.Pitch * DIVIDE_BY_2);
    sinCos(&SY, &CY, Rotator.Yaw * DIVIDE_BY_2);
    sinCos(&SR, &CR, Rotator.Roll * DIVIDE_BY_2);

    FQuat RotationQuat;
    RotationQuat.X = CR * SP * SY - SR * CP * CY;
    RotationQuat.Y = -CR * SP * CY - SR * CP * SY;
    RotationQuat.Z = CR * CP * SY - SR * SP * CY;
    RotationQuat.W = CR * CP * CY + SR * SP * SY;

    return RotationQuat;
}

static auto VecToRot(FVector Vector)
{
    FRotator R;
	
    R.Yaw =  std::atan2(Vector.Y, Vector.X) * (180.f / PI);

	R.Pitch = std::atan2(Vector.Z, std::sqrt(Vector.X * Vector.X + Vector.Y *Vector.Y)) * (180.f / PI);

	// roll can't be found from vector
	R.Roll = 0;

    return R;
}

static AActor* SpawnActorTrans(UClass* StaticClass, FTransform SpawnTransform, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
{
    AActor* FirstActor = GetGameplayStatics()->STATIC_BeginDeferredActorSpawnFromClass(GetWorld(), StaticClass, SpawnTransform, Flags, Owner);

    if (FirstActor)
    {
        AActor* FinalActor = GetGameplayStatics()->STATIC_FinishSpawningActor(FirstActor, SpawnTransform);

        if (FinalActor)
        {
            return FinalActor;
        }
    }

    return nullptr;
}

inline auto GetItemInstances(AFortPlayerController* PC)
{
    return PC->WorldInventory->Inventory.ItemInstances;
}

inline AActor* SpawnActor(UClass* ActorClass, FVector Location = { 0.0f, 0.0f, 0.0f }, FRotator Rotation = { 0, 0, 0 }, AActor* Owner = nullptr)
{
    FTransform SpawnTransform;

    SpawnTransform.Translation = Location;
    SpawnTransform.Scale3D = FVector { 1, 1, 1 };
    SpawnTransform.Rotation = RotToQuat(Rotation);

    return SpawnActorTrans(ActorClass, SpawnTransform, Owner);
}

template <typename RetActorType = AActor>
inline RetActorType* SpawnActor(FVector Location = { 0.0f, 0.0f, 0.0f }, AActor* Owner = nullptr, FQuat Rotation = { 0, 0, 0 })
{
    FTransform SpawnTransform;

    SpawnTransform.Translation = Location;
    SpawnTransform.Scale3D = FVector { 1, 1, 1 };
    SpawnTransform.Rotation = Rotation;

    return (RetActorType*)SpawnActorTrans(RetActorType::StaticClass(), SpawnTransform, Owner);
}

inline void CreateConsole()
{
    GetEngine()->GameViewport->ViewportConsole = (UConsole*)GetGameplayStatics()->STATIC_SpawnObject(UConsole::StaticClass(), GetEngine()->GameViewport);
}

inline auto CreateCheatManager(APlayerController* Controller, bool bFortCheatManager = false)
{
    if (!Controller->CheatManager)
    {
        if (bFortCheatManager)
            Controller->CheatManager = (UCheatManager*)GetGameplayStatics()->STATIC_SpawnObject(UCheatManager::StaticClass(), Controller);
        else
            Controller->CheatManager = (UCheatManager*)GetGameplayStatics()->STATIC_SpawnObject(UFortCheatManager::StaticClass(), Controller);
    }

    return Controller->CheatManager;
}

inline bool IsMatchingGuid(FGuid A, FGuid B)
{
    return A.A == B.A && A.B == B.B && A.C == B.C && A.D == B.D;
}

bool CanBuild(FVector& Location)
{
    static auto GameState = reinterpret_cast<AAthena_GameState_C*>(GetWorld()->GameState);

    FBuildingGridActorFilter filter { true, true, true, true};
    FBuildingNeighboringActorInfo OutActors;
    GameState->StructuralSupportSystem->K2_GetBuildingActorsInGridCell(Location, filter, &OutActors);
    auto Amount = OutActors.NeighboringCenterCellInfos.Num() + OutActors.NeighboringFloorInfos.Num() + OutActors.NeighboringWallInfos.Num();
    if (Amount == 0)
        return true;
    return false;
}

bool IsCurrentlyDisconnecting(UNetConnection* Connection)
{
    return false;
}

void Spectate(UNetConnection* SpectatingConnection, AFortPlayerStateAthena* StateToSpectate)
{
    auto PawnToSpectate = StateToSpectate->GetCurrentPawn();
    auto DeadPC = (AFortPlayerControllerAthena*)SpectatingConnection->PlayerController;
    auto DeadPlayerState = (AFortPlayerStateAthena*)DeadPC->PlayerState;

    if (!IsCurrentlyDisconnecting(SpectatingConnection) && SpectatingConnection && StateToSpectate && DeadPlayerState && DeadPC && PawnToSpectate)
    {
        DeadPC->PlayerToSpectateOnDeath = PawnToSpectate;
        DeadPC->SpectateOnDeath();

        DeadPlayerState->SpectatingTarget = StateToSpectate;
        DeadPlayerState->bIsSpectator = true;
        DeadPlayerState->OnRep_SpectatingTarget();

        auto SpectatorPC = SpawnActor<ABP_SpectatorPC_C>(PawnToSpectate->K2_GetActorLocation());
        SpectatorPC->SetNewCameraType(ESpectatorCameraType::DroneAttach, true);
        SpectatorPC->CurrentCameraType = ESpectatorCameraType::DroneAttach;
        SpectatorPC->ResetCamera();
        SpectatingConnection->PlayerController = SpectatorPC;
        
        // auto SpectatorPawn = SpawnActor<ABP_SpectatorPawn_C>(PawnToSpectate->K2_GetActorLocation(), PawnToSpectate);

        /* SpectatorPC->SpectatorPawn = SpectatorPawn;
        SpectatorPC->Pawn = SpectatorPawn;
        SpectatorPC->AcknowledgedPawn = SpectatorPawn;
        SpectatorPawn->Owner = SpectatorPC;
        SpectatorPawn->OnRep_Owner();
        SpectatorPC->OnRep_Pawn();
        SpectatorPC->Possess(SpectatorPawn); */

        if (DeadPC->QuickBars)
            DeadPC->QuickBars->K2_DestroyActor();
    }
}

inline void UpdateInventory(AFortPlayerController* PlayerController, int Dirty = 0, bool bRemovedItem = false)
{
    PlayerController->WorldInventory->bRequiresLocalUpdate = true;
    PlayerController->WorldInventory->HandleInventoryLocalUpdate();
    PlayerController->HandleWorldInventoryLocalUpdate();
    PlayerController->ForceUpdateQuickbar(EFortQuickBars::Primary);
    PlayerController->QuickBars->ForceNetUpdate();

    if (bRemovedItem)
        PlayerController->WorldInventory->Inventory.MarkArrayDirty();

    if (Dirty != 0 && PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num() >= Dirty)
        PlayerController->WorldInventory->Inventory.MarkItemDirty(PlayerController->WorldInventory->Inventory.ReplicatedEntries[Dirty]);
}

inline auto AddItem(AFortPlayerController* PC, UFortWorldItemDefinition* Def, int Slot, EFortQuickBars Bars = EFortQuickBars::Primary, int Count = 1, int* Idx = nullptr)
{
    if (!PC || !Def)
        return FFortItemEntry();

    if (Def->IsA(UFortWeaponItemDefinition::StaticClass()))
        Count = 1;

    auto QuickBarSlots = PC->QuickBars->PrimaryQuickBar.Slots;

    auto TempItemInstance = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 1);
    TempItemInstance->SetOwningControllerForTemporaryItem(PC);

    TempItemInstance->ItemEntry.Count = Count;
    TempItemInstance->OwnerInventory = PC->WorldInventory;

    auto& ItemEntry = TempItemInstance->ItemEntry;

    auto _Idx = PC->WorldInventory->Inventory.ReplicatedEntries.Add(ItemEntry);
	
    if (Idx)
        *Idx = _Idx;
	
    PC->WorldInventory->Inventory.ItemInstances.Add((UFortWorldItem*)TempItemInstance);
    PC->QuickBars->ServerAddItemInternal(ItemEntry.ItemGuid, Bars, Slot);

    return ItemEntry;
}

inline auto AddItemWithUpdate(AFortPlayerController* PC, UFortWorldItemDefinition* Def, int Slot, EFortQuickBars Bars = EFortQuickBars::Primary, int Count = 1)
{
	int Idx = 0;

    auto ItemEntry = AddItem(PC, Def, Slot, Bars, Count, &Idx);

    UpdateInventory(PC, Idx);

    return ItemEntry;
}

inline auto RemoveItem(AFortPlayerControllerAthena* PC, EFortQuickBars QuickBars, int Slot) // IMPORTANT TO FIX THIS
{
    if (Slot == 0 || !PC || PC->IsInAircraft())
        return;

    // UpdateInventory(PC, 0, true);

    auto pcQuickBars = PC->QuickBars;
    // pcQuickBars->PrimaryQuickBar.Slots[Slot].Items.FreeArray();
    // pcQuickBars->EmptySlot(QuickBars, Slot);
    auto& QuickBarSlot = pcQuickBars->PrimaryQuickBar.Slots[Slot];
    pcQuickBars->PrimaryQuickBar.Slots.RemoveAt(Slot);

    auto Inventory = PC->WorldInventory->Inventory;

    if (Inventory.ReplicatedEntries.Num() >= Slot)
    {
        Inventory.ReplicatedEntries.RemoveAt(Slot);
        std::cout << "Removed from ReplicatedEntries!\n";
    }

    if (Inventory.ItemInstances.Num() >= Slot)
    {
        Inventory.ItemInstances.RemoveAt(Slot);
        std::cout << "Removed from ItemInstances!\n";
    }

    // PC->RemoveItemFromQuickBars(PC->QuickBars->PrimaryQuickBar.Slots[Slot].Items[0]);

    UpdateInventory(PC, 0, true);
}

inline AFortWeapon* EquipWeaponDefinition(APawn* dPawn, UFortWeaponItemDefinition* Definition, FGuid& Guid)
{
    // auto weaponClass = Definition->GetWeaponActorClass();
    // if (weaponClass)
    auto Pawn = (APlayerPawn_Athena_C*)dPawn;
    if (Pawn && Definition)
    {
        // auto Weapon = (AFortWeapon*)SpawnActorTrans(weaponClass, {}, Pawn);
        auto Weapon = Pawn->EquipWeaponDefinition(Definition, Guid);

        if (Weapon)
        {
            /* Weapon->ItemEntryGuid = Guid;
            Weapon->WeaponData = Definition; */

            Weapon->ItemEntryGuid = Guid;
            Weapon->OnRep_ReplicatedWeaponData();
            Weapon->ClientGivenTo(Pawn);
            Pawn->ClientInternalEquipWeapon(Weapon);
            Pawn->OnRep_CurrentWeapon();
        }

        return Weapon;
    }

    return nullptr;
}

inline void EquipInventoryItem(AFortPlayerControllerAthena* PC, FGuid& Guid)
{
    if (!PC || PC->IsInAircraft())
        return;

    auto ItemInstances = PC->WorldInventory->Inventory.ItemInstances;

    for (int i = 0; i < ItemInstances.Num(); i++)
    {
        auto CurrentItemInstance = ItemInstances[i];

        if (!CurrentItemInstance)
            continue;

        auto Def = (UFortWeaponItemDefinition*)CurrentItemInstance->GetItemDefinitionBP();

        if (IsMatchingGuid(CurrentItemInstance->GetItemGuid(), Guid) && Def)
        {
            EquipWeaponDefinition((APlayerPawn_Athena_C*)PC->Pawn, Def, Guid);
        }
    }
}

inline void DumpObjects()
{
    std::ofstream objects("ObjectsDump.txt");

    if (objects)
    {
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            auto Object = UObject::GObjects->GetByIndex(i);

            if (!Object)
                continue;

            objects << '[' + std::to_string(Object->InternalIndex) + "] " + Object->GetFullName() << '\n';
        }
    }

    objects.close();

    std::cout << "Finished dumping objects!\n";
}

AFortOnlineBeaconHost* HostBeacon = nullptr;

void Listen()
{
    printf("[UWorld::Listen]\n");

    HostBeacon = SpawnActor<AFortOnlineBeaconHost>();
    HostBeacon->ListenPort = 7777;
    auto bInitBeacon = Native::OnlineBeaconHost::InitHost(HostBeacon);
    CheckNullFatal(bInitBeacon, "Failed to initialize the Beacon!");

    HostBeacon->NetDriverName = FName(282); // REGISTER_NAME(282,GameNetDriver)
    HostBeacon->NetDriver->NetDriverName = FName(282); // REGISTER_NAME(282,GameNetDriver)
    HostBeacon->NetDriver->World = GetWorld();

    GetWorld()->NetDriver = HostBeacon->NetDriver;
    GetWorld()->LevelCollections[0].NetDriver = HostBeacon->NetDriver;
    GetWorld()->LevelCollections[1].NetDriver = HostBeacon->NetDriver;

    Native::OnlineBeacon::PauseBeaconRequests(HostBeacon, false);

    auto GameState = (AAthena_GameState_C*)GetWorld()->GameState;

    // GameState->SpectatorClass = ABP_SpectatorPawn_C::StaticClass();
    // sGameState->OnRep_SpectatorClass();
	
    ((AAthena_GameMode_C*)GetWorld()->AuthorityGameMode)->GameSession->MaxPlayers = 100;
}

static void SummonPickup(AFortPlayerPawn* Pawn, auto ItemDef, int Count, FVector Location)
{
    auto FortPickup = SpawnActor<AFortPickupAthena>(Location, Pawn);

    FortPickup->bReplicates = true; // should be autmoatic but eh

    FortPickup->PrimaryPickupItemEntry.Count = Count;
    FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;

    FortPickup->OnRep_PrimaryPickupItemEntry();
    FortPickup->TossPickup(Location, Pawn, 6, true);
}

static void SummonPickupFromChest(auto ItemDef, int Count, FVector Location)
{
    auto FortPickup = SpawnActor<AFortPickupAthena>(Location);

    FortPickup->bReplicates = true; // should be autmoatic but eh
    FortPickup->bTossedFromContainer = true;

    FortPickup->PrimaryPickupItemEntry.Count = Count;
    FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;

    FortPickup->OnRep_PrimaryPickupItemEntry();
    FortPickup->OnRep_TossedFromContainer();
}

static void HandlePickup(AFortPlayerPawn* Pawn, void* params, bool bEquip = false)
{
    if (!Pawn || !params)
        return;

    auto Params = (AFortPlayerPawn_ServerHandlePickup_Params*)params;

    auto ItemInstances = ((AFortPlayerController*)Pawn->Controller)->WorldInventory->Inventory.ItemInstances;

    if (Params->Pickup)
    {
        if (bEquip && !Params->Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortWeaponItemDefinition::StaticClass()))
            bEquip = false;

        auto WorldItemDefinition = (UFortWorldItemDefinition*)Params->Pickup->PrimaryPickupItemEntry.ItemDefinition;
        auto Controller = (AFortPlayerControllerAthena*)Pawn->Controller;
        auto QuickBarSlots = Controller->QuickBars->PrimaryQuickBar.Slots;

        for (int i = 0; i < QuickBarSlots.Num(); i++)
        {
            if (!QuickBarSlots[i].Items.Data) // Checks if the slot is empty
            {
                if (i >= 6)
                {
                    auto QuickBars = Controller->QuickBars;

                    auto FocusedSlot = QuickBars->PrimaryQuickBar.CurrentFocusedSlot;

                    if (FocusedSlot == 0)
                        continue;

                    i = FocusedSlot;

                    FGuid& FocusedGuid = QuickBarSlots[FocusedSlot].Items[0];

                    for (int j = 0; i < ItemInstances.Num(); j++)
                    {
                        auto ItemInstance = ItemInstances[j];

                        if (!ItemInstance)
                            continue;

                        auto Def = ItemInstance->ItemEntry.ItemDefinition;
                        auto Guid = ItemInstance->ItemEntry.ItemGuid;

                        if (IsMatchingGuid(FocusedGuid, Guid))
                        {
                            SummonPickup((APlayerPawn_Athena_C*)Pawn, Def, 1, Pawn->K2_GetActorLocation());
                            break;
                        }
                    }

                    RemoveItem(Controller, EFortQuickBars::Primary, FocusedSlot);
                }

                auto entry = AddItemWithUpdate((AFortPlayerController*)Pawn->Controller, WorldItemDefinition, i, EFortQuickBars::Primary, 1); // Params->Pickup->PrimaryPickupItemEntry.Count);
                Params->Pickup->K2_DestroyActor();

                if (bEquip)
                    EquipWeaponDefinition((APlayerPawn_Athena_C*)Pawn, (UFortWeaponItemDefinition*)WorldItemDefinition, entry.ItemGuid);

                UpdateInventory((AFortPlayerController*)Pawn->Controller);

                break;
            }
        }
    }
}

static void InitInventory(AFortPlayerController* PlayerController, bool bSpawnInventory = true)
{
    PlayerController->QuickBars = SpawnActor<AFortQuickBars>({ -280, 400, 3000 }, PlayerController);
    auto QuickBars = PlayerController->QuickBars;
    PlayerController->OnRep_QuickBar();

    static auto Wall = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Wall.BuildingItemData_Wall");
    static auto Stair = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Stair_W.BuildingItemData_Stair_W");
    static auto Cone = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_RoofS.BuildingItemData_RoofS");
    static auto Floor = UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Floor.BuildingItemData_Floor");
    static auto Wood = UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition WoodItemData.WoodItemData");
    static auto Stone = UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition StoneItemData.StoneItemData");
    static auto Metal = UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition MetalItemData.MetalItemData");
    static auto Medium = UObject::FindObject<UFortResourceItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium");
    static auto Light = UObject::FindObject<UFortResourceItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight");
    static auto Heavy = UObject::FindObject<UFortResourceItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy");
    static auto EditTool = UObject::FindObject<UFortEditToolItemDefinition>("FortEditToolItemDefinition EditTool.EditTool");

    // we should probably only update once

    AddItem(PlayerController, Wall, 0, EFortQuickBars::Secondary, 1);
    AddItem(PlayerController, Floor, 1, EFortQuickBars::Secondary, 1);
    AddItem(PlayerController, Stair, 2, EFortQuickBars::Secondary, 1);
    AddItem(PlayerController, Cone, 3, EFortQuickBars::Secondary, 1);

    AddItem(PlayerController, Wood, 0, EFortQuickBars::Secondary, 999);
    AddItem(PlayerController, Stone, 0, EFortQuickBars::Secondary, 999);
    AddItem(PlayerController, Metal, 0, EFortQuickBars::Secondary, 999);
    AddItem(PlayerController, Medium, 0, EFortQuickBars::Secondary, 999);
    AddItem(PlayerController, Light, 0, EFortQuickBars::Secondary, 999);
    AddItem(PlayerController, Heavy, 0, EFortQuickBars::Secondary, 999);

    AddItemWithUpdate(PlayerController, EditTool, 0, EFortQuickBars::Primary, 1);

    QuickBars->ServerActivateSlotInternal(EFortQuickBars::Primary, 0, 0, true);
}

template <typename Class>
static FFortItemEntry FindItemInInventory(AFortPlayerControllerAthena* PC, bool& bFound)
{
    auto ItemInstances = PC->WorldInventory->Inventory.ItemInstances;

    for (int i = 0; i < ItemInstances.Num(); i++)
    {
        auto ItemInstance = ItemInstances[i];

        if (!ItemInstance)
            continue;

        auto Def = ItemInstance->ItemEntry.ItemDefinition;

        if (Def && Def->IsA(Class::StaticClass()))
        {
            bFound = true;
            return ItemInstance->ItemEntry;
        }
    }

    bFound = false;
    return FFortItemEntry();
}

FGameplayAbilitySpec* UAbilitySystemComponent_FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
{
    auto Specs = AbilitySystemComponent->ActivatableAbilities.Items;

    for (int i = 0; i < Specs.Num(); i++)
    {
        auto& Spec = Specs[i];

        if (Spec.Handle.Handle == Handle.Handle)
        {
            return &Spec;
        }
    }

    return nullptr;
}

void UAbilitySystemComponent_ConsumeAllReplicatedData(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey AbilityOriginalPredictionKey)
{
    /*
    FGameplayAbilitySpecHandleAndPredictionKey toFind { AbilityHandle, AbilityOriginalPredictionKey.Current };

    auto MapPairsData = AbilitySystemComponent->AbilityTargetDataMap;
    */
}

auto TryActivateAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle AbilityToActivate, bool InputPressed, FPredictionKey* PredictionKey, FGameplayEventData* TriggerEventData)
{
    auto Spec = UAbilitySystemComponent_FindAbilitySpecFromHandle(AbilitySystemComponent, AbilityToActivate);

    if (!Spec)
    {
        printf("InternalServerTryActiveAbility. Rejecting ClientActivation of ability with invalid SpecHandle!\n");
        AbilitySystemComponent->ClientActivateAbilityFailed(AbilityToActivate, PredictionKey->Current);
        return;
    }

    // UAbilitySystemComponent_ConsumeAllReplicatedData(AbilitySystemComponent, AbilityToActivate, *PredictionKey);

    UGameplayAbility* InstancedAbility = nullptr;
    Spec->InputPressed = true;

    if (Native::AbilitySystemComponent::InternalTryActivateAbility(AbilitySystemComponent, AbilityToActivate, *PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
    {
        // TryActivateAbility handles notifying the client of success
    }
    else
    {
        printf("InternalServerTryActiveAbility. Rejecting ClientActivation of %s. InternalTryActivateAbility failed\n", Spec->Ability->GetName().c_str());
        AbilitySystemComponent->ClientActivateAbilityFailed(AbilityToActivate, PredictionKey->Current);
        Spec->InputPressed = false;
        return;
    }

    Native::AbilitySystemComponent::MarkAbilitySpecDirty(AbilitySystemComponent, *Spec);
}

static auto GrantGameplayAbility(APlayerPawn_Athena_C* TargetPawn, UClass* GameplayAbilityClass)
{
    auto AbilitySystemComponent = TargetPawn->AbilitySystemComponent;

    if (!AbilitySystemComponent)
        return;

    auto GenerateNewSpec = [&]() -> FGameplayAbilitySpec
    {
        FGameplayAbilitySpecHandle Handle { rand() };

        FGameplayAbilitySpec Spec { -1, -1, -1, Handle, (UGameplayAbility*)GameplayAbilityClass->CreateDefaultObject(), 1, -1, nullptr, 0, false, false, false };

        return Spec;
    };

    auto Spec = GenerateNewSpec();
    auto Handle = Native::AbilitySystemComponent::GiveAbility(AbilitySystemComponent, &Spec.Handle, Spec);
}

static void HandleInventoryDrop(AFortPlayerPawn* Pawn, void* params)
{
    auto Params = (AFortPlayerController_ServerAttemptInventoryDrop_Params*)params;

    auto ItemInstances = GetItemInstances((AFortPlayerControllerAthena*)Pawn->Controller);
    auto Controller = (AFortPlayerControllerAthena*)Pawn->Controller;
    auto QuickBars = Controller->QuickBars;

    auto PrimaryQuickBarSlots = QuickBars->PrimaryQuickBar.Slots;
    auto SecondaryQuickBarSlots = QuickBars->SecondaryQuickBar.Slots;

    for (int i = 1; i < PrimaryQuickBarSlots.Num(); i++)
    {
        if (PrimaryQuickBarSlots[i].Items.Data)
        {
            if (IsMatchingGuid(PrimaryQuickBarSlots[i].Items[0], Params->ItemGuid))
            {
                RemoveItem(Controller, EFortQuickBars::Primary, i);
                // break;
            }
        }
    }

    for (int i = 0; i < SecondaryQuickBarSlots.Num(); i++)
    {
        if (SecondaryQuickBarSlots[i].Items.Data)
        {
            if (IsMatchingGuid(SecondaryQuickBarSlots[i].Items[0], Params->ItemGuid))
            {
                RemoveItem(Controller, EFortQuickBars::Secondary, i);
                // break;
            }
        }
    }

    for (int i = 1; i < ItemInstances.Num(); i++)
    {
        auto ItemInstance = ItemInstances[i];

        if (!ItemInstance)
            continue;

        auto Guid = ItemInstance->GetItemGuid();

        if (IsMatchingGuid(Guid, Params->ItemGuid))
        {
            auto def = ItemInstance->ItemEntry.ItemDefinition;

            if (def)
            {
                std::cout << "Matching Guid for " << def->GetFullName() << '\n';
                SummonPickup(Pawn, def, 1, Pawn->K2_GetActorLocation());
                // break;
            }
        }
    }

    /* for (int i = ItemInstances.Num(); i > 0; i--) // equip the item before until its valid
    {
        auto ItemInstance = ItemInstances[i];

        if (!ItemInstance)
            continue;

        auto Def = ItemInstance->ItemEntry.ItemDefinition;

        if (Def) // && Def->IsA(UFortWeaponItemDefinition::StaticClass()))
        {
            QuickBars->PrimaryQuickBar.CurrentFocusedSlot = i;
            // EquipWeaponDefinition((APlayerPawn_Athena_C*)Controller->Pawn, (UFortWeaponItemDefinition*)Def, ItemInstance->ItemEntry.ItemGuid, ItemInstance->ItemEntry.Count);
            QuickBars->ServerActivateSlotInternal(EFortQuickBars::Primary, 0, 0, true);
            break;
        }
    } */
}

static bool KickPlayer(AFortPlayerControllerAthena* PC, FString Message)
{
    FText text = reinterpret_cast<UKismetTextLibrary*>(UKismetTextLibrary::StaticClass())->STATIC_Conv_StringToText(Message);
    return Native::OnlineSession::KickPlayer(GetWorld()->AuthorityGameMode->GameSession, PC, text);
}

FTransform GetPlayerStart(AFortPlayerControllerAthena* PC)
{
    TArray<AActor*> OutActors;

    static auto GameplayStatics = (UGameplayStatics*)UGameplayStatics::StaticClass()->CreateDefaultObject();
    GameplayStatics->STATIC_GetAllActorsOfClass(GetWorld(), AFortPlayerStartWarmup::StaticClass(), &OutActors);

    auto ActorsNum = OutActors.Num();

    auto SpawnTransform = FTransform();
    SpawnTransform.Scale3D = FVector(1, 1, 1);
    SpawnTransform.Rotation = FQuat();
    SpawnTransform.Translation = FVector { 1250, 1818, 3284 }; // Next to salty

    auto GamePhase = ((AAthena_GameState_C*)GetWorld()->GameState)->GamePhase;

    if (ActorsNum != 0
        && (GamePhase == EAthenaGamePhase::Setup || GamePhase == EAthenaGamePhase::Warmup))
    {
        auto ActorToUseNum = rand() % ActorsNum;
        auto ActorToUse = (OutActors)[ActorToUseNum];

        while (!ActorToUse)
        {
            ActorToUseNum = rand() % ActorsNum;
            ActorToUse = (OutActors)[ActorToUseNum];
        }

        auto Location = ActorToUse->K2_GetActorLocation();
        SpawnTransform.Translation = ActorToUse->K2_GetActorLocation();

        PC->WarmupPlayerStart = (AFortPlayerStartWarmup*)ActorToUse;
    }

    return SpawnTransform;

    // return (GetWorld()->AuthorityGameMode->FindPlayerStart(PC, IncomingName))->K2_GetActorLocation();
}

inline UKismetMathLibrary* GetMath()
{
    return (UKismetMathLibrary*)UKismetMathLibrary::StaticClass();
}

static void InitPawn(AFortPlayerControllerAthena* PlayerController, FVector Loc = FVector { 1250, 1818, 3284 }, FQuat Rotation = FQuat())
{
    if (PlayerController->Pawn)
        PlayerController->Pawn->K2_DestroyActor();

    auto SpawnTransform = FTransform();
    SpawnTransform.Scale3D = FVector(1, 1, 1);
    SpawnTransform.Rotation = Rotation;
    SpawnTransform.Translation = Loc;

    // SpawnTransform = GetPlayerStart(PlayerController);

    auto Pawn = (APlayerPawn_Athena_C*)SpawnActorTrans(APlayerPawn_Athena_C::StaticClass(), SpawnTransform, PlayerController);

    PlayerController->Pawn = Pawn;
    PlayerController->AcknowledgedPawn = Pawn;
    Pawn->Owner = PlayerController;
    Pawn->OnRep_Owner();
    PlayerController->OnRep_Pawn();
    PlayerController->Possess(Pawn);

    Pawn->SetMaxHealth(100);
    Pawn->SetMaxShield(100);

    Pawn->bReplicateMovement = true;
    Pawn->OnRep_ReplicateMovement();

    static auto FortRegisteredPlayerInfo = UObject::FindObject<UFortRegisteredPlayerInfo>("FortRegisteredPlayerInfo Transient.FortEngine_0_1.FortGameInstance_0_1.FortRegisteredPlayerInfo_0_1");

    auto Hero = FortRegisteredPlayerInfo->AthenaMenuHeroDef;

    PlayerController->StrongMyHero = Hero;

    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

    PlayerState->HeroType = Hero->GetHeroTypeBP();
    PlayerState->OnRep_HeroType();

    for (auto i = 0; i < Hero->CharacterParts.Num(); i++)
    {
        auto Part = Hero->CharacterParts[i];

        if (!Part)
            continue;

        PlayerState->CharacterParts[i] = Part;
    }

    PlayerState->OnRep_CharacterParts();

    PlayerController->OnRep_QuickBar();
    PlayerController->QuickBars->OnRep_PrimaryQuickBar();
    PlayerController->QuickBars->OnRep_SecondaryQuickBar();
}

void ChangeItem(AFortPlayerControllerAthena* PC, UFortItemDefinition* Old, UFortItemDefinition* New, int Slot, bool bEquip = false) // we can find the slot too
{
    RemoveItem(PC, EFortQuickBars::Primary, Slot);
    auto NewEntry = AddItemWithUpdate(PC, (UFortWorldItemDefinition*)New, Slot);

    if (bEquip)
        EquipWeaponDefinition(PC->Pawn, (UFortWeaponItemDefinition*)New, NewEntry.ItemGuid);
}

void ClientMessage(AFortPlayerControllerAthena* PC, FString Message) // Send a message to the user's console.
{
    PC->ClientMessage(Message, FName(-1), 10000);
}

auto toWStr(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

inline UFortWeaponRangedItemDefinition* FindWID(const std::string& WID)
{
    auto Def = UObject::FindObject<UFortWeaponRangedItemDefinition>("FortWeaponRangedItemDefinition " + WID + '.' + WID);

    if (!Def)
    {
        Def = UObject::FindObject<UFortWeaponRangedItemDefinition>("WID_Harvest_" + WID + "_Athena_C_T01" + ".WID_Harvest_" + WID + "_Athena_C_T01");
        if (!Def)
            Def = UObject::FindObject<UFortWeaponRangedItemDefinition>(WID + "." + WID);
    }

    return Def;
}

void EquipLoadout(AFortPlayerController* Controller, std::vector<UFortWeaponRangedItemDefinition*> WIDS)
{
    FFortItemEntry pickaxeEntry;
	
    for (int i = 0; i < WIDS.size(); i++)
    {
        // if (i >= 6)
            // break;
        
        auto Def = WIDS[i];
		
		if (Def)
        {
            auto Entry = AddItemWithUpdate(Controller, Def, i);
            if (i == 0)
                pickaxeEntry = Entry;
        }
    }

	EquipWeaponDefinition(Controller->Pawn, (UFortWeaponMeleeItemDefinition*)pickaxeEntry.ItemDefinition, pickaxeEntry.ItemGuid);
}

inline auto ApplyAbilities(APawn* _Pawn)
{
    auto Pawn = (APlayerPawn_Athena_C*)_Pawn;
    static auto SprintAbility = UObject::FindObject<UClass>("Class FortniteGame.FortGameplayAbility_Sprint");
    static auto ReloadAbility = UObject::FindObject<UClass>("Class FortniteGame.FortGameplayAbility_Reload");
    static auto RangedWeaponAbility = UObject::FindObject<UClass>("Class FortniteGame.FortGameplayAbility_RangedWeapon");
    static auto JumpAbility = UObject::FindObject<UClass>("Class FortniteGame.FortGameplayAbility_Jump");
    static auto DeathAbility = UObject::FindObject<UClass>("BlueprintGeneratedClass GA_DefaultPlayer_Death.GA_DefaultPlayer_Death_C");
    static auto InteractUseAbility = UObject::FindObject<UClass>("BlueprintGeneratedClass GA_DefaultPlayer_InteractUse.GA_DefaultPlayer_InteractUse_C");
    static auto InteractSearchAbility = UObject::FindObject<UClass>("BlueprintGeneratedClass GA_DefaultPlayer_InteractSearch.GA_DefaultPlayer_InteractSearch_C");

    GrantGameplayAbility(Pawn, SprintAbility);
    GrantGameplayAbility(Pawn, ReloadAbility);
    GrantGameplayAbility(Pawn, RangedWeaponAbility);
    GrantGameplayAbility(Pawn, JumpAbility);
    GrantGameplayAbility(Pawn, DeathAbility);
    GrantGameplayAbility(Pawn, InteractUseAbility);
    GrantGameplayAbility(Pawn, InteractSearchAbility);
}

// ==================================================================
// NOTE(vak): Entity definition and handling
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

typedef u32 entity_id;
#define NilEntityID (0)

typedef enum
{
    EntityProp_Render,
    EntityProp_Lifetime,

    EntityProp_COUNT = 64,
} entity_prop;

typedef struct
{
    entity_id EntityID;
} entity_iter;

local entity_id     MakeEntity              (void);
local void          DeleteEntity            (entity_id EntityID);

local entity_iter   IterateEntities         (void);
local void          NextEntity              (entity_iter* Iter);

local void          SetEntityProp           (entity_id EntityID, entity_prop Prop, b32 Enabled);
local void          SetEntityP              (entity_id EntityID, v2 P);
local void          SetEntityDP             (entity_id EntityID, v2 DP);
local void          SetEntityForce          (entity_id EntityID, v2 Force);
local void          AddEntityForce          (entity_id EntityID, v2 Force);
local void          SetEntitySize           (entity_id EntityID, v2 Size);
local void          SetEntityColor          (entity_id EntityID, v4 Color);
local void          SetEntityLifetime       (entity_id EntityID, f32 Seconds);

local b32           GetEntityProp           (entity_id EntityID, entity_prop Prop);
local v2            GetEntityP              (entity_id EntityID);
local v2            GetEntityDP             (entity_id EntityID);
local v2            GetEntityDDP            (entity_id EntityID);
local v2            GetEntitySize           (entity_id EntityID);
local v4            GetEntityColor          (entity_id EntityID);
local f32           GetEntityLifetime       (entity_id EntityID);
local f32           GetEntityLifeRemaining  (entity_id EntityID);

local v2            GetEntityPredictedP     (entity_id EntityID, f32 DeltaTime);
local void          UpdateEntity            (entity_id EntityID, f32 DeltaTime);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

typedef struct
{
    // NOTE(vak): Properties

    u64 PropFlags;

    // NOTE(vak): Physics

    v2 P;
    v2 DP;
    v2 DDP;

    // NOTE(vak): Rendering

    v2 Size;
    v4 InverseTint;

    // NOTE(vak): Timing

    time CreationTime;
    f32 Lifetime;
} entity;

typedef struct
{
    u64     Level1;
    u64     Level0[64];
    entity  Entities[4096];
} entity_chunk;

local entity_chunk EntityChunk = {0};

local void MarkEntitySlotUsed(u32 Index)
{
    u32 Index0 = Index % 64;
    u32 Index1 = Index / 64;

    EntityChunk.Level0[Index1] |= ((u64)1 << Index0);

    b32 Filled = (EntityChunk.Level0[Index1] == U64Max);

    EntityChunk.Level1 |= ((u64)Filled << Index1);
}

local void MarkEntitySlotFree(u32 Index)
{
    u32 Index0 = Index % 64;
    u32 Index1 = Index / 64;

    EntityChunk.Level0[Index1] &= ~((u64)1 << Index0);

    b32 Filled = (EntityChunk.Level0[Index1] == U64Max);

    EntityChunk.Level1 &= ~((u64)1      << Index1);
    EntityChunk.Level1 |=  ((u64)Filled << Index1);
}

local entity* GetEntity(entity_id EntityID)
{
    entity* Entity = EntityChunk.Entities + (EntityID - 1);
    return (Entity);
}

local entity_id MakeEntity(void)
{
    if (EntityChunk.Level1 == U64Max)
        return (0);

    u32 Index1 = CountTrailingZeroes64(~EntityChunk.Level1);
    u32 Index0 = CountTrailingZeroes64(~EntityChunk.Level0[Index1]);
    u32 Index  = Index1*64 + Index0;

    entity_id Result = 1 + Index;

    MarkEntitySlotUsed(Index);

    entity* Entity = GetEntity(Result);

    ZeroStruct(Entity);
    Entity->CreationTime = GetWallClock();

    return (Result);
}

local void DeleteEntity(entity_id EntityID)
{
    ZeroStruct(GetEntity(EntityID));
    MarkEntitySlotFree(EntityID - 1);
}

local entity_iter IterateEntities(void)
{
    entity_iter Iter = {0};

    u32 Index = 0;
    for (;;)
    {
        u32 BitIndex  = Index % 64;
        u32 MaskIndex = Index / 64;

        u64 Mask = EntityChunk.Level0[MaskIndex] >> BitIndex;
        u32 MaxCount = (64 - BitIndex);

        u32 Count = CountTrailingZeroes64(Mask);

        Index += Count;
        if (Count < MaxCount)
            break;
    }

    if (Index >= ArrayCount(EntityChunk.Entities))
        Iter.EntityID = 0;
    else
        Iter.EntityID = 1 + Index;

    return (Iter);
}

local void NextEntity(entity_iter* Iter)
{
    if (Iter->EntityID == 0)
        return;

    u32 Index = Iter->EntityID;

    if (Index < ArrayCount(EntityChunk.Entities))
    {
        for (;;)
        {
            u32 BitIndex  = Index % 64;
            u32 MaskIndex = Index / 64;

            u64 Mask = EntityChunk.Level0[MaskIndex] >> BitIndex;
            u32 MaxCount = (64 - BitIndex);

            u32 Count = CountTrailingZeroes64(Mask);

            Index += Count;
            if (Count < MaxCount)
                break;
        }
    }

    if (Index >= ArrayCount(EntityChunk.Entities))
        Iter->EntityID = 0;
    else
        Iter->EntityID = 1 + Index;
}

local void SetEntityProp(entity_id EntityID, entity_prop Prop, b32 Enabled)
{
    entity* Entity = GetEntity(EntityID);

    if (Enabled)
        Entity->PropFlags |= ((u64)1 << Prop);
    else
        Entity->PropFlags &= ~((u64)1 << Prop);
}

local void SetEntityP(entity_id EntityID, v2 P)
{
    entity* Entity = GetEntity(EntityID);
    Entity->P = P;
}

local void SetEntityDP(entity_id EntityID, v2 DP)
{
    entity* Entity = GetEntity(EntityID);
    Entity->DP = DP;
}

local void SetEntityForce(entity_id EntityID, v2 Force)
{
    entity* Entity = GetEntity(EntityID);
    Entity->DDP = Force;
}

local void AddEntityForce(entity_id EntityID, v2 Force)
{
    entity* Entity = GetEntity(EntityID);
    Entity->DDP = V2Add(Entity->DDP, Force);
}

local void SetEntitySize(entity_id EntityID, v2 Size)
{
    entity* Entity = GetEntity(EntityID);
    Entity->Size = Size;
}

local void SetEntityColor(entity_id EntityID, v4 Color)
{
    entity* Entity = GetEntity(EntityID);

    v4 One = V4(1, 1, 1, 1);

    Entity->InverseTint = V4Sub(One, Color);
}

local void SetEntityLifetime(entity_id EntityID, f32 Seconds)
{
    entity* Entity = GetEntity(EntityID);
    Entity->Lifetime = Seconds;
}

local b32 GetEntityProp(entity_id EntityID, entity_prop Prop)
{
    entity* Entity = GetEntity(EntityID);
    b32 Result = (Entity->PropFlags >> Prop) & 1;
    return (Result);
}

local v2 GetEntityP(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);

    v2 Result = Entity->P;
    return (Result);
}

local v2 GetEntityDP(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);

    v2 Result = Entity->DP;
    return (Result);
}

local v2 GetEntityDDP(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);

    v2 Result = Entity->DDP;
    return (Result);
}

local v2 GetEntitySize(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);

    v2 Result = Entity->Size;
    return (Result);
}

local v4 GetEntityColor(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);

    v4 One = V4(1, 1, 1, 1);
    v4 Result = V4Sub(One, Entity->InverseTint);

    return (Result);
}

local f32 GetEntityLifetime(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);
    f32 Result = Entity->Lifetime;
    return (Result);
}

local f32 GetEntityLifeRemaining(entity_id EntityID)
{
    entity* Entity = GetEntity(EntityID);

    f32 Elapsed = GetSecondsElapsed(Entity->CreationTime, GetWallClock());
    f32 Result = Maximum(0, Entity->Lifetime - Elapsed);

    return (Result);
}

local v2 GetEntityPredictedP(entity_id EntityID, f32 DeltaTime)
{
    entity* Entity = GetEntity(EntityID);

    v2 ChangeInP = V2Add(
        V2MulScalar(Entity->DP, DeltaTime),
        V2MulScalar(Entity->DDP, 0.5f * Square(DeltaTime))
    );

    v2 Result = V2Add(Entity->P, ChangeInP);
    return (Result);
}

local void UpdateEntity(entity_id EntityID, f32 DeltaTime)
{
    entity* Entity = GetEntity(EntityID);

    if (GetEntityProp(EntityID, EntityProp_Lifetime))
    {
        if (GetEntityLifeRemaining(EntityID) <= 0.0f)
        {
            DeleteEntity(EntityID);
            return;
        }
    }

    v2 ChangeInP = V2Add(
        V2MulScalar(Entity->DP, DeltaTime),
        V2MulScalar(Entity->DDP, 0.5f * Square(DeltaTime))
    );

    Entity->P = V2Add(Entity->P, ChangeInP);
    Entity->DP = V2DivScalar(ChangeInP, DeltaTime);
}


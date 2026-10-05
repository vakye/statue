
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
    EntityProp_Render = 0,
    EntityProp_COUNT = 64,
} entity_prop;

typedef struct
{
    entity_id EntityID;
} entity_iter;

local entity_id     MakeEntity          (void);

local entity_iter   IterateEntities     (void);
local void          NextEntity          (entity_iter* Iter);

local void          SetEntityProp       (entity_id EntityID, entity_prop Prop, b32 Enabled);
local void          SetEntityP          (entity_id EntityID, v2 P);
local void          SetEntitySize       (entity_id EntityID, v2 Size);
local void          SetEntityColor      (entity_id EntityID, v4 Color);
local void          SetEntityForce      (entity_id EntityID, v2 Force);
local void          AddEntityForce      (entity_id EntityID, v2 Force);

local b32           GetEntityProp       (entity_id EntityID, entity_prop Prop);
local v2            GetEntityP          (entity_id EntityID);
local v2            GetEntityDP         (entity_id EntityID);
local v2            GetEntityDDP        (entity_id EntityID);
local v2            GetEntitySize       (entity_id EntityID);
local v4            GetEntityColor      (entity_id EntityID);

local v2            GetEntityPredictedP (entity_id EntityID, f32 DeltaTime);
local void          SimulateEntity      (entity_id EntityID, f32 DeltaTime);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

typedef struct
{
    // NOTE(vak): Properties

    u64 PropFlags;

    // NOTE(vak): Physics

    v2 P;
    v2 LastP;
    v2 DP;
    v2 DDP;

    // NOTE(vak): Rendering

    v2 Size;
    v4 InverseTint;
} entity;

local entity Entities[4096] = {0};
local u32 EntityCount = 0;

local entity_id MakeEntity(void)
{
    if (EntityCount == ArrayCount(Entities))
    {
        Println(StdErr, Str("error: too many entities"));
        Exit(1);
    }

    entity_id Result = 1 + EntityCount++;
    return (Result);
}

local entity* GetEntity(entity_id EntityID)
{
    if ((EntityID == 0) || (EntityID > EntityCount))
    {
        Println(StdErr, Str("error: invalid entity_id in GetEntity()"));
        Exit(1);
    }

    entity* Entity = Entities + (EntityID - 1);
    return (Entity);
}

local entity_iter IterateEntities(void)
{
    entity_iter Iter =
    {
        .EntityID = Minimum(1, EntityCount),
    };

    return (Iter);
}

local void NextEntity(entity_iter* Iter)
{
    Iter->EntityID++;

    if (Iter->EntityID > EntityCount)
        Iter->EntityID = 0;
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
    Entity->LastP = P;
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

    v2 Result = V2Sub(Entity->P, Entity->LastP);
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

local v2 GetEntityPredictedP(entity_id EntityID, f32 DeltaTime)
{
    entity* Entity = GetEntity(EntityID);

    v2 ChangeInP = V2Add(
        V2Sub(Entity->P, Entity->LastP),
        V2MulScalar(Entity->DDP, 0.5f * Square(DeltaTime))
    );

    v2 Result = V2Add(Entity->P, ChangeInP);
    return (Result);
}

local void SimulateEntity(entity_id EntityID, f32 DeltaTime)
{
    entity* Entity = GetEntity(EntityID);

    v2 ChangeInP = V2Add(
        V2Sub(Entity->P, Entity->LastP),
        V2MulScalar(Entity->DDP, 0.5f * Square(DeltaTime))
    );

    Entity->LastP = Entity->P;
    Entity->P = V2Add(Entity->P, ChangeInP);

    Entity->DP = V2DivScalar(V2Sub(Entity->P, Entity->LastP), DeltaTime);
}


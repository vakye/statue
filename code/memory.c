
// ==================================================================
// NOTE(vak): Memory management
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

typedef u32 arena_id;

local arena_id  MakeArena       (usize MinCommited, usize MinReserved);
local void      ResetArena      (arena_id ArenaID);
local void*     GetArenaBase    (arena_id ArenaID);
local usize     GetArenaUsed    (arena_id ArenaID);
local void*     GetArenaAllocAt (arena_id ArenaID);
local void*     PushArenaSize   (arena_id ArenaID, usize Size);

#define PushArena(ArenaID, Type) (Type*)PushArenaSize(ArenaID, sizeof(Type))
#define PushArenaArray(ArenaID, Type, Count) (Type*)PushArenaSize(ArenaID, sizeof(Type) * (Count))

typedef struct
{
    arena_id    ArenaID;
    usize       RestoreUsed;
} temporary_memory;

local temporary_memory BeginTemporaryMemory(arena_id ArenaID);
local void EndTemporaryMemory(temporary_memory TemporaryMemory);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

#define ArenaGranuleSize KB(256)

typedef struct
{
    void* Base;
    usize Used;
    usize Commited;
    usize Reserved;
} arena;

local arena Arenas[64] = {0};
local u32 ArenaCount = 0;

local arena* GetArena(arena_id ArenaID)
{
    arena* Arena = 0;

    if ((ArenaID > 0) && (ArenaID <= ArenaCount))
    {
        Arena = Arenas + ArenaID - 1;
    }

    return (Arena);
}

local arena_id MakeArena(usize MinCommited, usize MinReserved)
{
    if (ArenaCount == ArrayCount(Arenas))
    {
        Println(StdErr, Str("error: too many arenas"));
        Exit(1);
    }

    arena_id ArenaID = 1 + ArenaCount++;
    arena* Arena = GetArena(ArenaID);

    Arena->Reserved = AlignUp(MinReserved, ArenaGranuleSize);
    Arena->Commited = AlignUp(MinCommited, ArenaGranuleSize);

    Arena->Base = ReserveMemory(Arena->Reserved);

    if (Arena->Commited)
        CommitMemory(Arena->Base, Arena->Commited);

    return (ArenaID);
}

local void ResetArena(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    if (!Arena) return;

    Arena->Used = 0;
}

local void* GetArenaBase(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    if (!Arena) return (0);

    return (Arena->Base);
}

local usize GetArenaUsed(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    if (!Arena) return (0);

    return (Arena->Used);
}

local void* GetArenaAllocAt(arena_id ArenaID)
{
    arena* Arena = GetArena(ArenaID);
    if (!Arena) return (0);

    return ((u8*)Arena->Base + Arena->Used);
}

local void* PushArenaSize(arena_id ArenaID, usize Size)
{
    arena* Arena = GetArena(ArenaID);
    if (!Arena) return (0);

    if (Arena->Used + Size > Arena->Commited)
    {
        usize ExpandSize = (Arena->Used + Size) - (Arena->Commited);
        usize CommitSize = AlignUp(ExpandSize, ArenaGranuleSize);
        void* CommitAt   = (u8*)Arena->Base + Arena->Commited;

        if (Arena->Commited + CommitSize > Arena->Reserved)
        {
            Println(StdErr, Str("error: arena ran out of memory"));
            Exit(1);
        }

        CommitMemory(CommitAt, CommitSize);

        Arena->Commited += CommitSize;
    }

    void* Result = (u8*)Arena->Base + Arena->Used;
    Arena->Used += Size;

    return (Result);
}

local temporary_memory BeginTemporaryMemory(arena_id ArenaID)
{
    temporary_memory Result =
    {
        .ArenaID = ArenaID,
        .RestoreUsed = GetArenaUsed(ArenaID),
    };

    return (Result);
}

local void EndTemporaryMemory(temporary_memory TemporaryMemory)
{
    arena* Arena = GetArena(TemporaryMemory.ArenaID);
    if (!Arena) return;

    if (Arena->Used < TemporaryMemory.RestoreUsed)
    {
        Println(StdErr, Str("error: temporary memory is out of date"));
        Exit(1);
    }

    Arena->Used = TemporaryMemory.RestoreUsed;
}


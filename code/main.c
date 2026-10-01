
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

typedef u32 tile_id;

typedef struct
{
    u32     TileCountX;
    u32     TileCountY;
    u32*    Tiles;

    v2      Position; // NOTE(vak): Top-left corner of tile map
    v2      TileSize;
} tile_map;

local tile_map LoadTileMap(arena_id ArenaID, string TileMapString, u32 TileCountX, u32 TileCountY)
{
    if (TileMapString.Size != TileCountX*TileCountY)
    {
        Println(StdErr, Str("error: invalid tile map data string"));
        Exit(1);
    }

    tile_map Result =
    {
        .TileCountX = TileCountX,
        .TileCountY = TileCountY,
        .Tiles      = PushArenaArray(ArenaID, u32, TileCountX*TileCountY),
    };

    for (u32 TileIndex = 0; TileIndex < TileMapString.Size; TileIndex++)
    {
        char Character = TileMapString.Data[TileIndex];

        if (Character == '.')
            Result.Tiles[TileIndex] = 0;
        else if (Character == '#')
            Result.Tiles[TileIndex] = 1;
    }

    return (Result);
}

local void SetTileMapPosition(tile_map* TileMap, v2 Position)
{
    TileMap->Position = Position;
}

local void SetTileSize(tile_map* TileMap, v2 TileSize)
{
    TileMap->TileSize = TileSize;
}

local v2 GetTileMapSize(tile_map* TileMap)
{
    v2 TileCount = V2(
        (f32)TileMap->TileCountX,
        (f32)TileMap->TileCountY
    );

    v2 Result = V2Mul(TileCount, TileMap->TileSize);

    return (Result);
}

local tile_id GetTouchedTileID(tile_map* TileMap, v2 Position)
{

    v2 Coordinate = V2Div(V2Sub(Position, TileMap->Position), TileMap->TileSize);

    ssize SnappedX = (ssize)Coordinate.X;
    ssize SnappedY = (ssize)Coordinate.Y;

    if ((SnappedX < 0) || (SnappedX >= TileMap->TileCountX))
        return (0);

    if ((SnappedY < 0) || (SnappedY >= TileMap->TileCountY))
        return (0);

    ssize TileIndex = SnappedY * (ssize)TileMap->TileCountX + SnappedX;

    tile_id Result = 1 + TileIndex;

    return (Result);
}

local void SetTileValue(tile_map* TileMap, tile_id TileID, u32 Value)
{
    u32 TileCount = TileMap->TileCountX * TileMap->TileCountY;

    if ((TileID == 0) || (TileID > TileCount))
        return;

    u32 TileIndex = TileID - 1;

    TileMap->Tiles[TileIndex] = Value;
}

local rect2 GetTileRect(tile_map* TileMap, tile_id TileID)
{
    u32 TileCount = TileMap->TileCountX * TileMap->TileCountY;

    if ((TileID == 0) || (TileID > TileCount))
        return R2MinMax(V2Zero(), V2Zero());

    u32 TileIndex = TileID - 1;

    u32 TileX = TileIndex % TileMap->TileCountX;
    u32 TileY = TileIndex / TileMap->TileCountX;

    v2 TileCoordinate = V2((f32)TileX, (f32)TileY);

    v2 Min = V2Add(TileMap->Position, V2Mul(TileCoordinate, TileMap->TileSize));
    v2 Max = V2Add(Min, TileMap->TileSize);

    rect2 Result = R2MinMax(Min, Max);
    return (Result);
}

local void DrawTileMap(tile_map* TileMap)
{
    for (u32 TileY = 0; TileY < TileMap->TileCountY; TileY++)
    {
        for (u32 TileX = 0; TileX < TileMap->TileCountX; TileX++)
        {
            u32 TileIndex = TileY * TileMap->TileCountX + TileX;
            tile_id TileID = 1 + TileIndex;
            u32 TileValue = TileMap->Tiles[TileIndex];

            rect2 TileRect = GetTileRect(TileMap, TileID);

            if (TileValue)
            {
                RenderRect(
                    TileRect,
                    V4(0.8f, 0.8f, 0.8f, 1.0f)
                );
            }
        }
    }
}

local void Main(void)
{
    SetupWindow();
    SetupRenderer();

    arena_id PermanentArenaID = MakeArena(KB(64), GB(64));

    // NOTE(vak): 16x16 Tilemap

    string SmileyTileMapString = Str(
        "################"
        "#..............#"
        "#.###......###.#"
        "#.###......###.#"
        "#.###......###.#"
        "#..............#"
        "#..............#"
        "#..............#"
        "#..............#"
        "#..#........#..#"
        "#...##....##...#"
        "#.....####.....#"
        "#..............#"
        "#..............#"
        "#..............#"
        "################"
    );

    tile_map TileMap = LoadTileMap(
        PermanentArenaID,
        SmileyTileMapString,
        16, 16
    );

    SetTileSize(&TileMap, V2(32.0f, 32.0f));

    while (!IsWindowClosed())
    {
        PollEvents();

        v2 WindowSize = V2(
            (f32)GetWindowSizeX(),
            (f32)GetWindowSizeY()
        );

        v2 TileMapSize = GetTileMapSize(&TileMap);
        v2 TileMapP = V2ScalarMul(0.5f, V2Sub(WindowSize, TileMapSize));

        SetTileMapPosition(&TileMap, TileMapP);

        v2 MouseP = InputGetMouseP();
        tile_id MouseTouchingTileID = GetTouchedTileID(&TileMap, MouseP);

        if (InputIsDown(InputButton_MouseLeft))
            SetTileValue(&TileMap, MouseTouchingTileID, 1);

        if (InputIsDown(InputButton_MouseRight))
            SetTileValue(&TileMap, MouseTouchingTileID, 0);

        // NOTE(vak): Rendering stuff
        {
            SetClearColor(V4(0.07f, 0.08f, 0.1f, 1.0f));
            BeginRendering();

            DrawTileMap(&TileMap);

            if (MouseTouchingTileID)
            {
                rect2 TileRect = GetTileRect(&TileMap, MouseTouchingTileID);

                RenderRect(
                    TileRect,
                    V4(1.0f, 1.0f, 1.0f, 0.5f)
                );
            }

            EndRendering();
        }

        PresentWindow();
    }
}


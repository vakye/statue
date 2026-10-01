
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

local void Main(void)
{
    SetupWindow();
    SetupRenderer();

    // NOTE(vak): 16x16

    string TileMapString = Str(
        "################"
        "#.....#........#"
        "###.####..######"
        "#.........#....#"
        "#.........#....#"
        "#..............#"
        "######....######"
        "#....#....#....#"
        "#..............#"
        "#...########...#"
        "#...#..........#"
        "#...#..........#"
        "#####..#########"
        "#...#..........#"
        "#..............#"
        "#########..#####"
    );

    u32 TileCountX = 16;
    u32 TileCountY = 16;

    u32 TileMap[16 * 16] = {0};

    v2 TileSize = V2(32.0f, 32.0f);

    v2 TileMapSize = V2Mul(TileSize, V2((f32)TileCountX, (f32)TileCountY));

    for (u32 TileIndex = 0; TileIndex < TileMapString.Size; TileIndex++)
    {
        char Character = TileMapString.Data[TileIndex];

        if (Character == '.')
            TileMap[TileIndex] = 0;
        else if (Character == '#')
            TileMap[TileIndex] = 1;
    }

    while (!IsWindowClosed())
    {
        PollEvents();

        v2 WindowSize = V2(
            (f32)GetWindowSizeX(),
            (f32)GetWindowSizeY()
        );

        SetClearColor(V4(0.07f, 0.08f, 0.1f, 1.0f));
        BeginRendering();

        v2 TileMapP = V2ScalarMul(0.5f, V2Sub(WindowSize, TileMapSize));

        for (u32 TileY = 0; TileY < TileCountY; TileY++)
        {
            for (u32 TileX = 0; TileX < TileCountX; TileX++)
            {
                u32 TileValue = TileMap[TileY * TileCountX + TileX];

                v2 TileCoordinate = V2((f32)TileX, (f32)TileY);

                v2 Min = V2Add(TileMapP, V2Mul(TileCoordinate, TileSize));
                v2 Max = V2Add(Min, TileSize);

                if (TileValue)
                {
                    RenderRect(
                        R2MinMax(Min, Max),
                        V4(1.0f, 1.0f, 1.0f, 1.0f)
                    );
                }
            }
        }

        EndRendering();

        PresentWindow();
    }
}


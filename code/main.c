
// ==================================================================
// NOTE(vak): Main()
// ==================================================================

#pragma once

local void Main(void)
{
    SetupWindow();
    SetupRenderer();
    ToggleFullscreen();

    while (!IsWindowClosed())
    {
        PollEvents();

        if (InputIsPressed(InputButton_KeyF11))
            ToggleFullscreen();

        // NOTE(vak): Render
        {
            SetClearColor(V4(1.0f, 1.0f, 1.0f, 1.0f));
            BeginRendering();

            EndRendering();
        }

        PresentWindow();
    }
}


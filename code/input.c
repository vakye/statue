
// ==================================================================
// NOTE(vak): Input system that facilitates event communication
// between the underlying platform-dependent input code and the rest
// of the codebase.
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

// NOTE(vak): All coordinates (like mouse coordinates, ...) are
// assumed to be window coordinates with origin located at top-left.

typedef enum
{
    InputButton_Nil = 0,

    InputButton_MouseLeft,
    InputButton_MouseRight,
    InputButton_MouseMiddle,

    InputButton_KeyW,
    InputButton_KeyA,
    InputButton_KeyS,
    InputButton_KeyD,

    InputButton_KeyLeft,
    InputButton_KeyRight,
    InputButton_KeyUp,
    InputButton_KeyDown,

    InputButton_KeyF11,

    InputButton_COUNT,
} input_button;

// NOTE(vak): For everybody else

local b32 InputIsDown       (input_button Button);
local b32 InputIsUp         (input_button Button);
local b32 InputIsPressed    (input_button Button);
local b32 InputIsReleased   (input_button Button);
local v2  InputGetMouseP    (void);

// NOTE(vak): For platform

local void InputPrepareForFrame (void);
local void InputReportButton    (input_button Button, b32 IsDown);
local void InputReportMouseP    (v2 MouseP);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

typedef struct
{
    v2 MouseP; // NOTE(vak): This frame

    bit_mask IsDownMasks[GetBitMaskCount(InputButton_COUNT)]; // NOTE(vak): This frame
    bit_mask WasDownMasks[GetBitMaskCount(InputButton_COUNT)]; // NOTE(vak): Last frame
} input_state;

typedef struct
{
    b32 IsDown;
    b32 WasDown;
} input_button_state;

local input_state Input = {0};

local input_button_state InputGetButtonState(input_button Button)
{
    input_button_state Result = {0};

    Result.IsDown = BitMaskGet(
        Input.IsDownMasks, InputButton_COUNT, Button
    );

    Result.WasDown = BitMaskGet(
        Input.WasDownMasks, InputButton_COUNT, Button
    );

    return (Result);
}

local b32 InputIsDown(input_button Button)
{
    input_button_state State = InputGetButtonState(Button);

    return (State.IsDown);
}

local b32 InputIsUp(input_button Button)
{
    input_button_state State = InputGetButtonState(Button);

    return (!State.IsDown);
}

local b32 InputIsPressed(input_button Button)
{
    input_button_state State = InputGetButtonState(Button);

    return (State.IsDown) && (!State.WasDown);
}

local b32 InputIsReleased(input_button Button)
{
    input_button_state State = InputGetButtonState(Button);

    return (!State.IsDown) && (State.WasDown);
}

local v2 InputGetMouseP(void)
{
    return (Input.MouseP);
}

local void InputPrepareForFrame(void)
{
    for (usize Button = 0; Button < InputButton_COUNT; Button++)
    {
        usize IsDownBit = BitMaskGet(
            Input.IsDownMasks,
            InputButton_COUNT,
            Button
        );

        BitMaskSet(
            Input.WasDownMasks,
            InputButton_COUNT,
            Button,
            IsDownBit
        );
    }
}

local void InputReportButton(input_button Button, b32 IsDown)
{
    BitMaskSet(
        Input.IsDownMasks,
        InputButton_COUNT,
        Button,
        IsDown
    );
}

local void InputReportMouseP(v2 MouseP)
{
    Input.MouseP = MouseP;
}


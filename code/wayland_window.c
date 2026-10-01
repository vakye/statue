
// ==================================================================
// NOTE(vak): Wayland protocol window + input implementation
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

local void WaylandSetupWindow       (void);
local void WaylandToggleFullscreen  (void);
local u32  WaylandGetWindowSizeX    (void);
local u32  WaylandGetWindowSizeY    (void);
local b32  WaylandIsWindowClosed    (void);
local void WaylandPollEvents        (void);
local void WaylandPresentWindow     (void);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

#include <wayland-client.h>
#include "wayland_xdg.c"

typedef struct
{
    // NOTE(vak): Size reported by xdg_toplevel_resize
    u32 TopLevelSizeX;
    u32 TopLevelSizeY;

    // NOTE(vak): Acknowledged size when we send xdg_surface_ack_configure.
    // This should be used as the actual window SizeX and SizeY since we
    // sent an acknowledgement for these to the server.
    u32 SizeX;
    u32 SizeY;

    b32 IsFullscreen;
    b32 HasClosed;

    struct wl_display*      Display;
    struct wl_registry*     Registry;
    struct wl_compositor*   Compositor;
    struct xdg_wm_base*     XdgWmBase;
    struct wl_surface*      Surface;
    struct xdg_surface*     XdgSurface;
    struct xdg_toplevel*    XdgTopLevel;

    struct wl_seat*         Seat;
    struct wl_pointer*      Pointer;
    struct wl_keyboard*     Keyboard;

    // TODO(vak): Figure out a better way to do this...
    // Maybe buffer up events and then wait for frame() ???

    u32                     LastPointerMotionTime;
} wayland_state;

local wayland_state Wayland = {0};

local void WaylandFatalError(string Message)
{
    Print(StdErr, Str("wayland: error: "));
    Println(StdErr, Message);
    Exit(1);
}

local void WaylandRegistryGlobalEvent(
    void*               Data,
    struct wl_registry* Registry,
    u32                 Name,
    const char*         InterfaceData,
    u32                 Version
)
{
    Unused(Data);
    Unused(Registry);

    string Interface = CString(InterfaceData);

    if (StringEquals(Interface, CString(wl_compositor_interface.name)))
    {
        Wayland.Compositor = wl_registry_bind(Wayland.Registry, Name, &wl_compositor_interface, Version);

        if (!Wayland.Compositor)
            WaylandFatalError(Str("failed to bind wl_compositor"));
    }
    else if (StringEquals(Interface, CString(xdg_wm_base_interface.name)))
    {
        Wayland.XdgWmBase = wl_registry_bind(Wayland.Registry, Name, &xdg_wm_base_interface, Version);

        if (!Wayland.XdgWmBase)
            WaylandFatalError(Str("failed to bind xdg_wm_base"));
    }
    else if (StringEquals(Interface, CString(wl_seat_interface.name)))
    {
        Wayland.Seat = wl_registry_bind(Wayland.Registry, Name, &wl_seat_interface, Version);

        if (!Wayland.Seat)
            WaylandFatalError(Str("failed to bind wl_seat"));
    }
}

local struct wl_registry_listener WaylandRegistryListener =
{
    .global = WaylandRegistryGlobalEvent,
};

local void WaylandXdgWmBasePingEvent(
    void*               Data,
    struct xdg_wm_base* XdgWmBase,
    u32                 Serial
)
{
    Unused(Data);
    Unused(XdgWmBase);

    xdg_wm_base_pong(Wayland.XdgWmBase, Serial);
}

local struct xdg_wm_base_listener WaylandXdgWmBaseListener =
{
    .ping = WaylandXdgWmBasePingEvent,
};

local void WaylandXdgSurfaceConfigureEvent(
    void*               Data,
    struct xdg_surface* XdgSurface,
    u32                 Serial
)
{
    Unused(Data);
    Unused(XdgSurface);

    Wayland.SizeX = Wayland.TopLevelSizeX;
    Wayland.SizeY = Wayland.TopLevelSizeY;

    xdg_surface_ack_configure(Wayland.XdgSurface, Serial);
}

local struct xdg_surface_listener WaylandXdgSurfaceListener =
{
    .configure = WaylandXdgSurfaceConfigureEvent,
};

local void WaylandXdgTopLevelConfigureEvent(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel,
    s32                     Width,
    s32                     Height,
    struct wl_array*        States
)
{
    Unused(Data);
    Unused(XdgTopLevel);
    Unused(States);

    Wayland.TopLevelSizeX = Maximum(0, Width);
    Wayland.TopLevelSizeY = Maximum(0, Height);
}

local void WaylandXdgTopLevelCloseEvent(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel
)
{
    Unused(Data);
    Unused(XdgTopLevel);

    Wayland.HasClosed = true;
}

local void WaylandXdgTopLevelConfigureBoundsEvent(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel,
    s32                     Width,
    s32                     Height
)
{
    Unused(Data);
    Unused(XdgTopLevel);
    Unused(Width);
    Unused(Height);
}

local void WaylandXdgTopLevelWmCapabilitiesEvent(
    void*                   Data,
    struct xdg_toplevel*    XdgTopLevel,
    struct wl_array*        Capabilities
)
{
    Unused(Data);
    Unused(XdgTopLevel);
    Unused(Capabilities);
}

local struct xdg_toplevel_listener WaylandXdgTopLevelListener =
{
    .configure          = WaylandXdgTopLevelConfigureEvent,
    .close              = WaylandXdgTopLevelCloseEvent,
    .configure_bounds   = WaylandXdgTopLevelConfigureBoundsEvent,
    .wm_capabilities    = WaylandXdgTopLevelWmCapabilitiesEvent,
};

local void WaylandPointerEnterEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Serial,
    struct wl_surface*  Surface,
    wl_fixed_t          SurfaceX,
    wl_fixed_t          SurfaceY
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Serial);

    if (Surface != Wayland.Surface)
        return;

    v2 MouseP = V2(
        (f32) wl_fixed_to_double(SurfaceX),
        (f32) wl_fixed_to_double(SurfaceY)
    );

    InputReportMouseP(MouseP);
}

local void WaylandPointerLeaveEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Serial,
    struct wl_surface*  Surface
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Serial);
    Unused(Surface);
}

local void WaylandPointerMotionEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Time,
    wl_fixed_t          SurfaceX,
    wl_fixed_t          SurfaceY
)
{
    Unused(Data);
    Unused(Pointer);

    if (Time > Wayland.LastPointerMotionTime)
    {
        v2 MouseP = V2(
            (f32) wl_fixed_to_double(SurfaceX),
            (f32) wl_fixed_to_double(SurfaceY)
        );

        InputReportMouseP(MouseP);

        Wayland.LastPointerMotionTime = Time;
    }
}

local void WaylandPointerButtonEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Serial,
    u32                 Time,
    u32                 EvdevButtonCode,
    u32                 State
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Serial);
    Unused(Time);

    b32 IsDown = (State == WL_POINTER_BUTTON_STATE_PRESSED);

    input_button Button = InputButton_Nil;

    switch (EvdevButtonCode)
    {
        default: Button = InputButton_Nil; break;

        // NOTE(vak): Values taken from linux/input-event-codes.h
        //      BTN_LEFT    = 0x110
        //      BTN_RIGHT   = 0x111
        //      BTN_MIDDLE  = 0x112

        case 0x110: Button = InputButton_MouseLeft;      break;
        case 0x111: Button = InputButton_MouseRight;     break;
        case 0x112: Button = InputButton_MouseMiddle;    break;
    }

    if (Button != InputButton_Nil)
        InputReportButton(Button, IsDown);
}

local void WaylandPointerFrameEvent(
    void*               Data,
    struct wl_pointer*  Pointer
)
{
    Unused(Data);
    Unused(Pointer);
}

local void WaylandPointerAxisEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Time,
    u32                 Axis,
    wl_fixed_t          Value
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Time);
    Unused(Axis);
    Unused(Value);
}

local void WaylandPointerAxisSourceEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 AxisSource
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(AxisSource);
}

local void WaylandPointerAxisStopEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Time,
    u32                 Axis
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Time);
    Unused(Axis);
}

local void WaylandPointerAxisDiscreteEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Axis,
    s32                 Discrete
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Axis);
    Unused(Discrete);
}

local void WaylandPointerAxisValue120Event(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Axis,
    s32                 Value120
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Axis);
    Unused(Value120);
}

local void WaylandPointerAxisRelativeDirectionEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    u32                 Axis,
    u32                 Direction
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(Axis);
    Unused(Direction);
}

local void WaylandPointerWarpEvent(
    void*               Data,
    struct wl_pointer*  Pointer,
    wl_fixed_t          SurfaceX,
    wl_fixed_t          SurfaceY
)
{
    Unused(Data);
    Unused(Pointer);
    Unused(SurfaceX);
    Unused(SurfaceY);
}

local struct wl_pointer_listener WaylandPointerListener =
{
    .enter                      = WaylandPointerEnterEvent,
    .leave                      = WaylandPointerLeaveEvent,
    .motion                     = WaylandPointerMotionEvent,
    .button                     = WaylandPointerButtonEvent,
    .frame                      = WaylandPointerFrameEvent,
    .axis                       = WaylandPointerAxisEvent,
    .axis_source                = WaylandPointerAxisSourceEvent,
    .axis_stop                  = WaylandPointerAxisStopEvent,
    .axis_discrete              = WaylandPointerAxisDiscreteEvent,
    .axis_value120              = WaylandPointerAxisValue120Event,
    .axis_relative_direction    = WaylandPointerAxisRelativeDirectionEvent,
    .warp                       = WaylandPointerWarpEvent,
};

local void WaylandSeatCapabilitiesEvent(
    void*           Data,
    struct wl_seat* Seat,
    u32             Capabilities
)
{
    Unused(Data);
    Unused(Seat);

    if (Wayland.Pointer) wl_pointer_release(Wayland.Pointer);
    if (Wayland.Keyboard) wl_keyboard_release(Wayland.Keyboard);

    if (Capabilities & WL_SEAT_CAPABILITY_POINTER)
        Wayland.Pointer = wl_seat_get_pointer(Wayland.Seat);

    if (Capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
        Wayland.Keyboard = wl_seat_get_keyboard(Wayland.Seat);

    if (Wayland.Pointer)
        wl_pointer_add_listener(Wayland.Pointer, &WaylandPointerListener, 0);
}

local void WaylandSeatNameEvent(
    void*           Data,
    struct wl_seat* Seat,
    const char*     Name
)
{
    Unused(Data);
    Unused(Seat);
    Unused(Name);
}

local struct wl_seat_listener WaylandSeatListener =
{
    .capabilities   = WaylandSeatCapabilitiesEvent,
    .name           = WaylandSeatNameEvent,
};

local void WaylandSetupWindow(void)
{
    Wayland.Display = wl_display_connect(0);
    if (!Wayland.Display)
        WaylandFatalError(Str("failed to connect to wl_display"));

    Wayland.Registry = wl_display_get_registry(Wayland.Display);
    if (!Wayland.Registry)
        WaylandFatalError(Str("failed to get wl_registry"));

    wl_registry_add_listener(Wayland.Registry, &WaylandRegistryListener, 0);
    wl_display_roundtrip(Wayland.Display);

    if (!Wayland.Compositor)
        WaylandFatalError(Str("no wl_compositor"));

    if (!Wayland.XdgWmBase)
        WaylandFatalError(Str("no xdg_wm_base"));

    xdg_wm_base_add_listener(Wayland.XdgWmBase, &WaylandXdgWmBaseListener, 0);

    if (Wayland.Seat)
        wl_seat_add_listener(Wayland.Seat, &WaylandSeatListener, 0);

    Wayland.Surface = wl_compositor_create_surface(Wayland.Compositor);
    if (!Wayland.Surface)
        WaylandFatalError(Str("failed to create wl_surface"));

    Wayland.XdgSurface = xdg_wm_base_get_xdg_surface(Wayland.XdgWmBase, Wayland.Surface);
    if (!Wayland.XdgSurface)
        WaylandFatalError(Str("failed to get xdg_surface"));

    xdg_surface_add_listener(Wayland.XdgSurface, &WaylandXdgSurfaceListener, 0);

    Wayland.XdgTopLevel = xdg_surface_get_toplevel(Wayland.XdgSurface);
    if (!Wayland.XdgTopLevel)
        WaylandFatalError(Str("failed to get xdg_toplevel"));

    xdg_toplevel_add_listener(Wayland.XdgTopLevel, &WaylandXdgTopLevelListener, 0);

    xdg_toplevel_set_title(Wayland.XdgTopLevel, "Statue");
    xdg_toplevel_set_app_id(Wayland.XdgTopLevel, "Statue");

    wl_surface_commit(Wayland.Surface);
    wl_display_roundtrip(Wayland.Display);
    wl_surface_commit(Wayland.Surface);
}

local void WaylandToggleFullscreen(void)
{
    if (!Wayland.IsFullscreen)
        xdg_toplevel_set_fullscreen(Wayland.XdgTopLevel, 0);
    else
        xdg_toplevel_unset_fullscreen(Wayland.XdgTopLevel);

    Wayland.IsFullscreen = !Wayland.IsFullscreen;
}

local u32 WaylandGetWindowSizeX(void)
{
    return (Wayland.SizeX);
}

local u32 WaylandGetWindowSizeY(void)
{
    return (Wayland.SizeY);
}

local b32 WaylandIsWindowClosed(void)
{
    return (Wayland.HasClosed);
}

local void WaylandPollEvents(void)
{
    wl_display_roundtrip(Wayland.Display);
}

local void WaylandPresentWindow(void)
{
    wl_surface_commit(Wayland.Surface);
}


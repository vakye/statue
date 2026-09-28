
// ==================================================================
// NOTE(vak): Wayland protocol window + input implementation
// ==================================================================

// ==================================================================
// NOTE(vak): Required system call wrappers:
//      + socket()
//      + connect()
//      + recv()
//      + send()
// ==================================================================

#pragma once

// ==================================================================
// NOTE(vak): Interface
// ==================================================================

local void WaylandSetupWindow       (void);
local u32  WaylandGetWindowSizeX    (void);
local u32  WaylandGetWindowSizeY    (void);
local b32  WaylandIsWindowClosed    (void);
local void WaylandPollEvents        (void);
local void WaylandPresentWindow     (void);

// ==================================================================
// NOTE(vak): Internal Interface
// ==================================================================

local void WaylandFatalError (string Message);
local u32  WaylandPushID     (void);

typedef struct // NOTE(vak): 256 bytes
{
    u32 ObjectID;
    u16 OpCode;
    u16 Size;
    u8  Data[248];
} wayland_request;

local void WaylandBeginRequest              (wayland_request* Request, u32 ObjectID, u16 OpCode);
local void WaylandPushU32                   (wayland_request* Request, u32 Value);
local void WaylandPushString                (wayland_request* Request, string Value);
local void WaylandEndRequest                (wayland_request* Request);

typedef struct
{
    u8*     Buffer;
    usize   Size;
    usize   ConsumeAt;
} wayland_receiver;

local void      WaylandReceive              (wayland_receiver* Receiver);
local u32       WaylandConsumeU32           (wayland_receiver* Receiver);
local u16       WaylandConsumeU16           (wayland_receiver* Receiver);
local string    WaylandConsumeString        (wayland_receiver* Receiver);
local void      WaylandSkipBytes            (wayland_receiver* Receiver, usize Size);

local void      WaylandDisplayConnect       (void);
local void      WaylandDisplayGetRegistry   (void);
local void      WaylandSetupSurface         (void);
local void      WaylandCommitSurface        (void);
local void      WaylandDisplayRoundtrip     (void);

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

typedef struct
{
    // NOTE(vak): Size reported by xdg_toplevel_resize
    s32 TopLevelSizeX;
    s32 TopLevelSizeY;

    // NOTE(vak): Acknowledged size when we send xdg_surface_ack_configure.
    // This should be used as the actual window SizeX and SizeY since we
    // sent an acknowledgement for these to the server.
    s32 SizeX;
    s32 SizeY;

    b32 HasClosed;
    b32 ReceivedDone;

    u32 ObjectCount;
    s32 DisplayFD;

    u32 SyncCallbackID;
    u32 RegistryID;
    u32 CompositorID;
    u32 XdgWmBaseID;
    u32 SurfaceID;
    u32 XdgSurfaceID;
    u32 XdgTopLevelID;
} wayland_state;

local wayland_state Wayland = {0};

local void WaylandSetupWindow(void)
{
    WaylandDisplayConnect();
    WaylandDisplayGetRegistry();
    WaylandDisplayRoundtrip();

    if (Wayland.CompositorID == 0) WaylandFatalError(Str("no wl_compositor"));
    if (Wayland.XdgWmBaseID  == 0) WaylandFatalError(Str("no xdg_wm_base"));

    WaylandSetupSurface();

    WaylandCommitSurface();
    WaylandDisplayRoundtrip();
    WaylandCommitSurface();
}

local u32 WaylandGetWindowSizeX(void)
{
    return Maximum(0, Wayland.SizeX);
}

local u32 WaylandGetWindowSizeY(void)
{
    return Maximum(0, Wayland.SizeY);
}

local b32 WaylandIsWindowClosed(void)
{
    return (Wayland.HasClosed);
}

local void WaylandPollEvents(void)
{
    WaylandDisplayRoundtrip();
}

local void WaylandPresentWindow(void)
{
    WaylandCommitSurface();
}

// ==================================================================
// NOTE(vak): Internal Implementation
// ==================================================================

enum
{
    WaylandObjectID_Display                     = 1,
    WaylandOpCode_DisplayErrorEvent             = 0,
    WaylandOpCode_DisplaySync                   = 0,
    WaylandOpCode_DisplayGetRegistry            = 1,

    WaylandOpCode_CallbackDoneEvent             = 0,

    WaylandOpCode_RegistryBind                  = 0,
    WaylandOpCode_RegistryGlobalEvent           = 0,

    WaylandOpCode_CompositorCreateSurface       = 0,

    WaylandOpCode_SurfaceCommit                 = 6,

    WaylandOpCode_XdgWmBaseGetXdgSurface        = 2,
    WaylandOpCode_XdgWmBasePingEvent            = 0,
    WaylandOpCode_XdgWmBasePong                 = 3,

    WaylandOpCode_XdgSurfaceGetTopLevel         = 1,
    WaylandOpCode_XdgSurfaceAckConfigure        = 4,
    WaylandOpCode_XdgSurfaceConfigureEvent      = 0,

    WaylandOpCode_XdgTopLevelSetTitle           = 2,
    WaylandOpCode_XdgTopLevelSetAppID           = 3,
    WaylandOpCode_XdgTopLevelConfigureEvent     = 0,
    WaylandOpCode_XdgTopLevelCloseEvent         = 1,
};

local void WaylandFatalError(string Message)
{
    Print(StdErr, Str("[wayland]: error: "));
    Println(StdErr, Message);
    Exit(1);
}

local u32 WaylandPushID(void)
{
    // NOTE(vak): ID #1 always correspond to
    // the display, so we start allocating IDs
    // starting from #2.

    Wayland.ObjectCount++;

    u32 Result = 1 + Wayland.ObjectCount;

    return (Result);
}

local void WaylandBeginRequest(wayland_request* Request, u32 ObjectID, u16 OpCode)
{
    ZeroStruct(Request);

    Request->ObjectID   = ObjectID;
    Request->OpCode     = OpCode;
    Request->Size       = 8;
}

local void WaylandPushU32(wayland_request* Request, u32 Value)
{
    if (Request->Size < 8)
        WaylandFatalError(Str("wayland_request not initialized properly"));

    if (Request->Size + 4 > sizeof(*Request))
        WaylandFatalError(Str("wayland_request not big enough"));

    *(u32*)(Request->Data + Request->Size - 8) = Value;
    Request->Size += 4;
}

local void WaylandPushString(wayland_request* Request, string Value)
{
    if (Request->Size < 8)
        WaylandFatalError(Str("wayland_request not initialized properly"));

    usize SizeWithNull = Value.Size + 1;

    WaylandPushU32(Request, SizeWithNull);

    usize AlignedSize = AlignUp(SizeWithNull, 4);
    usize Padding = AlignedSize - SizeWithNull;

    if (Request->Size + AlignedSize > sizeof(*Request))
        WaylandFatalError(Str("wayland_request not big enough"));

    MemoryCopy(
        Request->Data + Request->Size - 8,
        Value.Data,
        Value.Size
    );

    Request->Size += Value.Size;

    Request->Data[Request->Size - 8] = '\0';
    Request->Size++;

    for (usize Index = 0; Index < Padding; Index++)
    {
        Request->Data[Request->Size - 8] = 0;
        Request->Size++;
    }
}

local void WaylandEndRequest(wayland_request* Request)
{
    ssize SendResult = send(
        Wayland.DisplayFD,
        Request,
        Request->Size,
        0
    );

    if (SendResult != (ssize)Request->Size)
        WaylandFatalError(Str("failed to send wayland request"));
}

local void WaylandReceive(wayland_receiver* Receiver)
{
    ZeroStruct(Receiver);

    u8 Buffer[KB(256)] = {0};

    ssize RecvResult = recv(
        Wayland.DisplayFD,
        Buffer,
        sizeof(Buffer),
        0
    );

    if (RecvResult >= 0)
    {
        Receiver->Buffer    = Buffer;
        Receiver->Size      = RecvResult;
        Receiver->ConsumeAt = 0;
    }
    else
    {
        WaylandFatalError(Str("failed to recv from wayland display"));
    }
}

local u32 WaylandConsumeU32(wayland_receiver* Receiver)
{
    if (Receiver->ConsumeAt + 4 > Receiver->Size)
        WaylandFatalError(Str("invalid wayland packet from server"));

    u32 Result = *(u32*)(Receiver->Buffer + Receiver->ConsumeAt);
    Receiver->ConsumeAt += 4;

    return (Result);
}

local u16 WaylandConsumeU16(wayland_receiver* Receiver)
{
    if (Receiver->ConsumeAt + 2 > Receiver->Size)
        WaylandFatalError(Str("invalid wayland packet from server"));

    u32 Result = *(u16*)(Receiver->Buffer + Receiver->ConsumeAt);
    Receiver->ConsumeAt += 2;

    return (Result);
}

local string WaylandConsumeString(wayland_receiver* Receiver)
{
    u32 SizeWithNull = WaylandConsumeU32(Receiver);
    u32 PaddedSize = AlignUp(SizeWithNull, 4);

    if (Receiver->ConsumeAt + PaddedSize > Receiver->Size)
        WaylandFatalError(Str("invalid wayland packet from server"));

    string Result = NilString;

    if (SizeWithNull > 0)
    {
        Result = StrData(
            (char*)(Receiver->Buffer + Receiver->ConsumeAt),
            SizeWithNull - 1
        );
    }

    Receiver->ConsumeAt += PaddedSize;

    return (Result);
}

local void WaylandSkipBytes(wayland_receiver* Receiver, usize Size)
{
    if (Receiver->ConsumeAt + Size > Receiver->Size)
        WaylandFatalError(Str("invalid wayland packet from server"));

    Receiver->ConsumeAt += Size;
}

local void WaylandDisplayConnect(void)
{
    // NOTE(vak): Retrieve $XDG_RUNTIME_DIR and $WAYLAND_DISPLAY

    string XdgRuntimeDir = GetEnv(Str("XDG_RUNTIME_DIR"));

    if (IsNilString(XdgRuntimeDir))
        WaylandFatalError(Str("unable to get XDG_RUNTIME_DIR"));

    string WaylandDisplay = GetEnv(Str("WAYLAND_DISPLAY"));

    if (IsNilString(WaylandDisplay))
        WaylandDisplay = Str("wayland-0");

    // NOTE(vak): Construct an address to the wayland display:
    //      Address.sun_path = '$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY'

    struct sockaddr_un Address =
    {
        .sun_family = AF_UNIX,
    };

    if (XdgRuntimeDir.Size + WaylandDisplay.Size + 2 > sizeof(Address.sun_path))
        WaylandFatalError(Str("path to wayland display is too long"));

    {
        usize Count = 0;

        MemoryCopy(
            Address.sun_path + Count,
            XdgRuntimeDir.Data,
            XdgRuntimeDir.Size
        );

        Count += XdgRuntimeDir.Size;

        Address.sun_path[Count++] = '/';

        MemoryCopy(
            Address.sun_path + Count,
            WaylandDisplay.Data,
            WaylandDisplay.Size
        );

        Count += WaylandDisplay.Size;

        Address.sun_path[Count] = '\0';
    }

    // NOTE(vak): Connect to wayland display

    Wayland.DisplayFD = socket(AF_UNIX, SOCK_STREAM, 0);
    if (Wayland.DisplayFD < 0)
        WaylandFatalError(Str("unable to make wayland display socket"));

    s32 ConnectResult = connect(
        Wayland.DisplayFD,
        (const struct sockaddr*)&Address,
        sizeof(Address)
    );

    if (ConnectResult < 0)
        WaylandFatalError(Str("failed to connect to wayland display"));
}

local void WaylandDisplayGetRegistry(void)
{
    wayland_request Request = {0};

    WaylandBeginRequest(
        &Request,
        WaylandObjectID_Display,
        WaylandOpCode_DisplayGetRegistry
    );

    Wayland.RegistryID = WaylandPushID();
    WaylandPushU32(&Request, Wayland.RegistryID);

    WaylandEndRequest(&Request);
}

local void WaylandSetupSurface(void)
{
    wayland_request Request = {0};

    // NOTE(vak): wl_surface
    {
        WaylandBeginRequest(
            &Request,
            Wayland.CompositorID,
            WaylandOpCode_CompositorCreateSurface
        );

        Wayland.SurfaceID = WaylandPushID();
        WaylandPushU32(&Request, Wayland.SurfaceID);

        WaylandEndRequest(&Request);
    }

    // NOTE(vak): xdg_surface
    {
        WaylandBeginRequest(
            &Request,
            Wayland.XdgWmBaseID,
            WaylandOpCode_XdgWmBaseGetXdgSurface
        );

        Wayland.XdgSurfaceID = WaylandPushID();
        WaylandPushU32(&Request, Wayland.XdgSurfaceID);
        WaylandPushU32(&Request, Wayland.SurfaceID);

        WaylandEndRequest(&Request);
    }

    // NOTE(vak): xdg_toplevel
    {
        WaylandBeginRequest(
            &Request,
            Wayland.XdgSurfaceID,
            WaylandOpCode_XdgSurfaceGetTopLevel
        );

        Wayland.XdgTopLevelID = WaylandPushID();
        WaylandPushU32(&Request, Wayland.XdgTopLevelID);

        WaylandEndRequest(&Request);
    }

    // NOTE(vak): xdg_toplevel_set_title("Statue")
    {
        WaylandBeginRequest(
            &Request,
            Wayland.XdgTopLevelID,
            WaylandOpCode_XdgTopLevelSetTitle
        );

        WaylandPushString(&Request, Str("Statue"));

        WaylandEndRequest(&Request);
    }

    // NOTE(vak): xdg_toplevel_set_app_id("Statue")
    {
        WaylandBeginRequest(
            &Request,
            Wayland.XdgTopLevelID,
            WaylandOpCode_XdgTopLevelSetAppID
        );

        WaylandPushString(&Request, Str("Statue"));

        WaylandEndRequest(&Request);
    }
}

local void WaylandCommitSurface(void)
{
    wayland_request Request = {0};

    WaylandBeginRequest(
        &Request,
        Wayland.SurfaceID,
        WaylandOpCode_SurfaceCommit
    );

    WaylandEndRequest(&Request);
}

local void WaylandRegistryBind(u32 Name, string Interface, u32 Version, u32 NewID)
{
    wayland_request Request = {0};

    WaylandBeginRequest(
        &Request,
        Wayland.RegistryID,
        WaylandOpCode_RegistryBind
    );

    WaylandPushU32      (&Request, Name);
    WaylandPushString   (&Request, Interface);
    WaylandPushU32      (&Request, Version);
    WaylandPushU32      (&Request, NewID);

    WaylandEndRequest(&Request);
}

local void WaylandHandleMessages(void)
{
    wayland_receiver Receiver = {0};

    WaylandReceive(&Receiver);

    while (Receiver.ConsumeAt < Receiver.Size)
    {
        u32 ObjectID = WaylandConsumeU32(&Receiver);
        u16 OpCode   = WaylandConsumeU16(&Receiver);
        u16 Size     = WaylandConsumeU16(&Receiver);

        if ((Size < 8) || (Size % 4 != 0))
            WaylandFatalError(Str("received wayland packet with invalid size from server"));

        if ((ObjectID == WaylandObjectID_Display) &&
            (OpCode == WaylandOpCode_DisplayErrorEvent))
        {
            u32     TargetObjectID  = WaylandConsumeU32(&Receiver);
            u32     Code            = WaylandConsumeU32(&Receiver);
            string  ErrorMessage    = WaylandConsumeString(&Receiver);

            Unused(TargetObjectID);
            Unused(Code);

            WaylandFatalError(ErrorMessage);            
        }
        else if ((ObjectID == Wayland.SyncCallbackID) &&
                 (OpCode == WaylandOpCode_CallbackDoneEvent))
        {
            u32 CallbackData = WaylandConsumeU32(&Receiver);
            Unused(CallbackData);

            Wayland.ReceivedDone = true;
        }
        else if ((ObjectID == Wayland.RegistryID) &&
                 (OpCode == WaylandOpCode_RegistryGlobalEvent))
        {
            u32     Name        = WaylandConsumeU32(&Receiver);
            string  Interface   = WaylandConsumeString(&Receiver);
            u32     Version     = WaylandConsumeU32(&Receiver);

            if (StringEquals(Interface, Str("wl_compositor")))
            {
                Wayland.CompositorID = WaylandPushID();
                WaylandRegistryBind(Name, Interface, Version, Wayland.CompositorID);
            }
            else if (StringEquals(Interface, Str("xdg_wm_base")))
            {
                Wayland.XdgWmBaseID = WaylandPushID();
                WaylandRegistryBind(Name, Interface, Version, Wayland.XdgWmBaseID);
            }
        }
        else if ((ObjectID == Wayland.XdgWmBaseID) &&
                 (OpCode == WaylandOpCode_XdgWmBasePingEvent))
        {
            u32 Serial = WaylandConsumeU32(&Receiver);

            wayland_request Request = {0};

            WaylandBeginRequest(
                &Request,
                Wayland.XdgWmBaseID,
                WaylandOpCode_XdgWmBasePong
            );

            WaylandPushU32(&Request, Serial);

            WaylandEndRequest(&Request);
        }
        else if ((ObjectID == Wayland.XdgSurfaceID) &&
                 (OpCode == WaylandOpCode_XdgSurfaceConfigureEvent))
        {
            Wayland.SizeX = Wayland.TopLevelSizeX;
            Wayland.SizeY = Wayland.TopLevelSizeY;

            u32 Serial = WaylandConsumeU32(&Receiver);

            wayland_request Request = {0};

            WaylandBeginRequest(
                &Request,
                Wayland.XdgSurfaceID,
                WaylandOpCode_XdgSurfaceAckConfigure
            );

            WaylandPushU32(&Request, Serial);

            WaylandEndRequest(&Request);
        }
        else if ((ObjectID == Wayland.XdgTopLevelID) &&
                 (OpCode == WaylandOpCode_XdgTopLevelConfigureEvent))
        {
            s32 SizeX       = (s32)WaylandConsumeU32(&Receiver);
            s32 SizeY       = (s32)WaylandConsumeU32(&Receiver);
            u32 StateBytes  = WaylandConsumeU32(&Receiver);

            WaylandSkipBytes(&Receiver, StateBytes);

            Wayland.TopLevelSizeX = Maximum(0, SizeX);
            Wayland.TopLevelSizeY = Maximum(0, SizeY);
        }
        else if ((ObjectID == Wayland.XdgTopLevelID) &&
                 (OpCode == WaylandOpCode_XdgTopLevelCloseEvent))
        {
            Wayland.HasClosed = true;
        }
        else
        {
            usize SizeRemaining = Size - 8;
            WaylandSkipBytes(&Receiver, SizeRemaining);
        }
    }
}

local void WaylandDisplaySync(void)
{
}

local void WaylandDisplayRoundtrip(void)
{
    Wayland.ReceivedDone = false;

    // NOTE(vak): wl_display_sync()
    {
        wayland_request Request = {0};

        WaylandBeginRequest(
            &Request,
            WaylandObjectID_Display,
            WaylandOpCode_DisplaySync
        );

        if (Wayland.SyncCallbackID == 0)
            Wayland.SyncCallbackID = WaylandPushID();

        WaylandPushU32(&Request, Wayland.SyncCallbackID);

        WaylandEndRequest(&Request);
    }

    while (!Wayland.ReceivedDone)
        WaylandHandleMessages();
}


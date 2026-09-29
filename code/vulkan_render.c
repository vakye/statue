
// ==================================================================
// NOTE(vak): Vulkan implementation of render.c
// ==================================================================

#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

local PFN_vkGetInstanceProcAddr         vkGetInstanceProcAddr = 0;
local PFN_vkCreateInstance              vkCreateInstance = 0;
local PFN_vkEnumerateInstanceVersion    vkEnumerateInstanceVersion = 0;

// ==================================================================
// NOTE(vak): All instance functions that are loaded
// ==================================================================

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    #include <vulkan/vulkan_wayland.h>

    #define VulkanWaylandFunctions(X) \
        X(vkCreateWaylandSurfaceKHR)
#else
    #error VK_USE_PLATFORM_*_KHR not defined for Vulkan renderer
#endif

#if !defined(VulkanWaylandFunctions)
    #define VulkanWaylandFunctions(X)
#endif

#define VulkanAllFunctions(X) \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceProperties) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkGetPhysicalDeviceSurfaceSupportKHR) \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    \
    X(vkCreateDevice) \
    X(vkGetDeviceQueue) \
    \
    X(vkCreateCommandPool) \
    X(vkAllocateCommandBuffers) \
    \
    X(vkCreateSemaphore) \
    \
    X(vkCreateImageView) \
    X(vkDestroyImageView) \
    \
    X(vkCreateSwapchainKHR) \
    X(vkDestroySwapchainKHR) \
    X(vkGetSwapchainImagesKHR) \
    X(vkAcquireNextImageKHR) \
    \
    X(vkResetCommandBuffer) \
    X(vkBeginCommandBuffer) \
    X(vkEndCommandBuffer) \
    \
    X(vkCmdPipelineBarrier) \
    X(vkCmdBeginRendering) \
    X(vkCmdEndRendering) \
    \
    X(vkQueueSubmit) \
    X(vkQueuePresentKHR) \
    X(vkDeviceWaitIdle) \
    \
    VulkanWaylandFunctions(X)

#define VulkanDeclareFunction(Name) local PFN_##Name Name = 0;
    VulkanAllFunctions(VulkanDeclareFunction)
#undef VulkanDeclareFunction

// ==================================================================
// NOTE(vak): Implementation
// ==================================================================

typedef struct
{
    u32                 VersionOfAPI;
    VkInstance          Instance;
    VkSurfaceKHR        Surface;
    VkPhysicalDevice    PhysicalDevice;
    u32                 QueueFamilyIndex;
    VkDevice            Device;
    VkQueue             Queue;
    VkCommandPool       CommandPool;
    VkCommandBuffer     CommandBuffer;
    VkSemaphore         AcquireSemaphore;
    VkSemaphore         SubmitSemaphore;
    VkSurfaceFormatKHR  SwapchainFormat;
    VkPresentModeKHR    PresentMode;

    VkExtent2D          SwapchainExtent;
    VkSwapchainKHR      Swapchain;
    u32                 SwapchainImageCount;

    // TODO(vak): Arena allocation

    VkImage             SwapchainImages[16];
    VkImageView         SwapchainImageViews[16];

    v4                  ClearColor;
    u32                 ImageIndex;
} vulkan_state;

local vulkan_state Vulkan = {0};

local void VulkanFatalError(string Message)
{
    Print(StdErr, Str("vulkan: error: "));
    Println(StdErr, Message);
    Exit(1);
}

#define VulkanCheck(VulkanCall) \
    if ((VulkanCall) != VK_SUCCESS) \
        VulkanFatalError(Str("'" #VulkanCall "' failed"));

local void SetupRenderer(void)
{
    // NOTE(vak): Load non-instance functions
    {
        vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetVulkanLoader();

        if (!vkGetInstanceProcAddr)
            VulkanFatalError(Str("failed to load vkGetInstanceProcAddr"));

        vkCreateInstance = (PFN_vkCreateInstance)
            vkGetInstanceProcAddr(0, "vkCreateInstance");

        if (!vkCreateInstance)
            VulkanFatalError(Str("failed to load vkCreateInstance"));

        vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)
            vkGetInstanceProcAddr(0, "vkEnumerateInstanceVersion");

        if (!vkEnumerateInstanceVersion)
            VulkanFatalError(Str("vkEnumerateInstanceVersion not available. Vulkan 1.0 is not supported"));
    }

    // NOTE(vak): API version
    {
        Vulkan.VersionOfAPI = VK_API_VERSION_1_4;

        u32 InstanceVersion = 0;
        VulkanCheck(vkEnumerateInstanceVersion(&InstanceVersion));

        if (InstanceVersion < Vulkan.VersionOfAPI)
            VulkanFatalError(Str("only vulkan 1.4 or above is supported"));
    }

    // NOTE(vak): Instance
    {
        const char* Layers[] =
        {
            "VK_LAYER_KHRONOS_validation",
        };

        const char* Extensions[] =
        {
            "VK_KHR_surface",

            #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
                "VK_KHR_wayland_surface",
            #else
                #error Missing Vulkan surface extension
            #endif
        };

        VkInstanceCreateInfo InstanceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo)
            {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = "Statue",
                .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                .pEngineName = "Statue",
                .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                .apiVersion = Vulkan.VersionOfAPI,
            },
            .enabledLayerCount = ArrayCount(Layers),
            .ppEnabledLayerNames = Layers,
            .enabledExtensionCount = ArrayCount(Extensions),
            .ppEnabledExtensionNames = Extensions,
        };

        VulkanCheck(vkCreateInstance(&InstanceInfo, 0, &Vulkan.Instance));
    }

    // NOTE(vak): Load instance functions
    {
        #define VulkanLoadFunction(Name) \
            Name = (PFN_##Name)vkGetInstanceProcAddr(Vulkan.Instance, #Name); \
            if (!Name) \
                VulkanFatalError(Str("failed to load " #Name));

        VulkanAllFunctions(VulkanLoadFunction)

        #undef VulkanLoadFunction
    }

    // NOTE(vak): Surface
    {
        #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
            VkWaylandSurfaceCreateInfoKHR SurfaceInfo =
            {
                .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
                .display = Wayland.Display,
                .surface = Wayland.Surface,
            };

            VulkanCheck(vkCreateWaylandSurfaceKHR(Vulkan.Instance, &SurfaceInfo, 0, &Vulkan.Surface));
        #else
            #error Unimplemented Vulkan surface creation code
        #endif
    }

    // NOTE(vak): Physical device
    {
        // TODO(vak): Arena allocation

        VkPhysicalDevice PhysicalDevices[64] = {0};
        u32 PhysicalDeviceCount = ArrayCount(PhysicalDevices);

        VulkanCheck(vkEnumeratePhysicalDevices(
            Vulkan.Instance,
            &PhysicalDeviceCount,
            PhysicalDevices
        ));

        VkPhysicalDevice Preferred  = {0};
        VkPhysicalDevice Fallback   = {0};

        for (u32 Index = 0; Index < PhysicalDeviceCount; Index++)
        {
            VkPhysicalDevice PhysicalDevice = PhysicalDevices[Index];

            VkPhysicalDeviceProperties Properties = {0};
            vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);

            if (Properties.apiVersion < Vulkan.VersionOfAPI)
                continue;

            if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                if (!Preferred) Preferred = PhysicalDevice;
            }
            else
            {
                if (!Fallback) Fallback = PhysicalDevice;
            }
        }

        Vulkan.PhysicalDevice = (Preferred) ? (Preferred) : (Fallback);

        if (!Vulkan.PhysicalDevice)
            VulkanFatalError(Str("no suitable physical device available"));
    }

    // NOTE(vak): Queue family index
    {
        // TODO(vak): Arena allocation

        VkQueueFamilyProperties QueueFamiliesProperties[64] = {0};
        u32 QueueFamilyCount = ArrayCount(QueueFamiliesProperties);

        vkGetPhysicalDeviceQueueFamilyProperties(
            Vulkan.PhysicalDevice,
            &QueueFamilyCount,
            QueueFamiliesProperties
        );

        Vulkan.QueueFamilyIndex = U32Max;

        for (u32 Index = 0; Index < QueueFamilyCount; Index++)
        {
            VkQueueFamilyProperties* Properties = QueueFamiliesProperties + Index;

            VkBool32 SurfaceSupported = VK_FALSE;

            VulkanCheck(vkGetPhysicalDeviceSurfaceSupportKHR(
                Vulkan.PhysicalDevice,
                Index,
                Vulkan.Surface,
                &SurfaceSupported
            ));

            if (!SurfaceSupported)
                continue;

            VkQueueFlags RequiredFlags =
                VK_QUEUE_GRAPHICS_BIT |
                VK_QUEUE_TRANSFER_BIT |
                VK_QUEUE_COMPUTE_BIT;

            if ((Properties->queueFlags & RequiredFlags) == RequiredFlags)
            {
                Vulkan.QueueFamilyIndex = Index;
                break;
            }
        }

        if (Vulkan.QueueFamilyIndex == U32Max)
            VulkanFatalError(Str("failed to select suitable queue family"));
    }

    // NOTE(vak): Device
    {
        const char* Extensions[] =
        {
            "VK_KHR_swapchain",
        };

        VkPhysicalDeviceVulkan13Features Vulkan13Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .dynamicRendering = true,
        };

        VkDeviceCreateInfo DeviceInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &Vulkan13Features,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &(VkDeviceQueueCreateInfo)
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = Vulkan.QueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = (f32[1]){1.0f},
            },
            .enabledExtensionCount = ArrayCount(Extensions),
            .ppEnabledExtensionNames = Extensions,
        };

        VulkanCheck(vkCreateDevice(Vulkan.PhysicalDevice, &DeviceInfo, 0, &Vulkan.Device));
    }

    // NOTE(vak): Queue
    {
        vkGetDeviceQueue(Vulkan.Device, Vulkan.QueueFamilyIndex, 0, &Vulkan.Queue);
    }

    // NOTE(vak): Command pool
    {
        VkCommandPoolCreateInfo CommandPoolInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = Vulkan.QueueFamilyIndex,
        };

        VulkanCheck(vkCreateCommandPool(Vulkan.Device, &CommandPoolInfo, 0, &Vulkan.CommandPool));
    }

    // NOTE(vak): Command buffer
    {
        VkCommandBufferAllocateInfo AllocateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = Vulkan.CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VulkanCheck(vkAllocateCommandBuffers(Vulkan.Device, &AllocateInfo, &Vulkan.CommandBuffer));
    }

    // NOTE(vak): Semaphore
    {
        VkSemaphoreCreateInfo SemaphoreInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VulkanCheck(vkCreateSemaphore(Vulkan.Device, &SemaphoreInfo, 0, &Vulkan.AcquireSemaphore));
        VulkanCheck(vkCreateSemaphore(Vulkan.Device, &SemaphoreInfo, 0, &Vulkan.SubmitSemaphore));
    }

    // NOTE(vak): Swapchain format
    {
        // TODO(vak): Arena allocation

        VkSurfaceFormatKHR SurfaceFormats[512] = {0};
        u32 SurfaceFormatCount = ArrayCount(SurfaceFormats);

        VulkanCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            Vulkan.PhysicalDevice,
            Vulkan.Surface,
            &SurfaceFormatCount,
            SurfaceFormats
        ));

        Vulkan.SwapchainFormat.format = VK_FORMAT_UNDEFINED;

        for (u32 Index = 0; Index < SurfaceFormatCount; Index++)
        {
            VkSurfaceFormatKHR SurfaceFormat = SurfaceFormats[Index];

            if (SurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                Vulkan.SwapchainFormat = SurfaceFormat;
                break;
            }
            else if (SurfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
            {
                Vulkan.SwapchainFormat = SurfaceFormat;
                break;
            }
        }

        if (!Vulkan.SwapchainFormat.format)
            VulkanFatalError(Str("failed to select suitable image format for swapchain"));
    }

    // NOTE(vak): Present mode
    {
        // TODO(vak): Arena allocation

        VkPresentModeKHR PresentModes[64] = {0};
        u32 PresentModeCount = ArrayCount(PresentModes);

        VulkanCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
            Vulkan.PhysicalDevice,
            Vulkan.Surface,
            &PresentModeCount,
            PresentModes
        ));

        Vulkan.PresentMode = VK_PRESENT_MODE_FIFO_KHR;

        for (u32 Index = 0; Index < PresentModeCount; Index++)
        {
            VkPresentModeKHR PresentMode = PresentModes[Index];

            if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                Vulkan.PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }
}

local void SetClearColor(v4 Color)
{
    Vulkan.ClearColor = Color;
}

local void BeginRendering(void)
{
    // NOTE(vak): Swapchain resize
    {
        u32 WindowSizeX = GetWindowSizeX();
        u32 WindowSizeY = GetWindowSizeY();

        if ((WindowSizeX != Vulkan.SwapchainExtent.width) ||
            (WindowSizeY != Vulkan.SwapchainExtent.height))
        {
            VulkanCheck(vkDeviceWaitIdle(Vulkan.Device));

            for (u32 Index = 0; Index < Vulkan.SwapchainImageCount; Index++)
                if (Vulkan.SwapchainImageViews[Index])
                    vkDestroyImageView(Vulkan.Device, Vulkan.SwapchainImageViews[Index], 0);

            if (Vulkan.Swapchain)
                vkDestroySwapchainKHR(Vulkan.Device, Vulkan.Swapchain, 0);

            Vulkan.SwapchainExtent.width  = WindowSizeX;
            Vulkan.SwapchainExtent.height = WindowSizeY;

            VkSurfaceCapabilitiesKHR SurfaceCapabilities = {0};
            VulkanCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                Vulkan.PhysicalDevice,
                Vulkan.Surface,
                &SurfaceCapabilities
            ));

            u32 DesiredImageCount = 3;

            u32 MinImageCount = Clamp(
                SurfaceCapabilities.minImageCount,
                DesiredImageCount,
                SurfaceCapabilities.maxImageCount
            );

            VkSwapchainCreateInfoKHR SwapchainInfo =
            {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = Vulkan.Surface,
                .minImageCount = MinImageCount,
                .imageFormat = Vulkan.SwapchainFormat.format,
                .imageColorSpace = Vulkan.SwapchainFormat.colorSpace,
                .imageExtent = Vulkan.SwapchainExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .preTransform = SurfaceCapabilities.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = Vulkan.PresentMode,
                .clipped = true,
            };

            VulkanCheck(vkCreateSwapchainKHR(Vulkan.Device, &SwapchainInfo, 0, &Vulkan.Swapchain));

            Vulkan.SwapchainImageCount = ArrayCount(Vulkan.SwapchainImages);

            VulkanCheck(vkGetSwapchainImagesKHR(
                Vulkan.Device,
                Vulkan.Swapchain,
                &Vulkan.SwapchainImageCount,
                Vulkan.SwapchainImages
            ));

            for (u32 Index = 0; Index < Vulkan.SwapchainImageCount; Index++)
            {
                VkImageViewCreateInfo ImageViewInfo =
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = Vulkan.SwapchainImages[Index],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = Vulkan.SwapchainFormat.format,
                    .components =
                    {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .layerCount = 1,
                    },
                };

                VulkanCheck(vkCreateImageView(Vulkan.Device, &ImageViewInfo, 0, &Vulkan.SwapchainImageViews[Index]));
            }

            VulkanCheck(vkDeviceWaitIdle(Vulkan.Device));
        }
    }

    // NOTE(vak): Acquire image
    {
        VulkanCheck(vkAcquireNextImageKHR(
            Vulkan.Device,
            Vulkan.Swapchain,
            U64Max,
            Vulkan.AcquireSemaphore,
            0,
            &Vulkan.ImageIndex
        ));
    }
}

local void EndRendering(void)
{
    // NOTE(vak): Begin command buffer
    {
        VkCommandBufferBeginInfo BeginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };

        VulkanCheck(vkResetCommandBuffer(Vulkan.CommandBuffer, 0));
        VulkanCheck(vkBeginCommandBuffer(Vulkan.CommandBuffer, &BeginInfo));
    }

    // NOTE(vak): Render barrier
    {
        VkImageMemoryBarrier RenderBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Vulkan.SwapchainImages[Vulkan.ImageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            Vulkan.CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0, 0, 0,
            1, &RenderBarrier
        );
    }

    // NOTE(vak): Begin rendering
    {
        VkRenderingInfo RenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea =
            {
                .offset = {.x = 0, .y = 0},
                .extent = Vulkan.SwapchainExtent,
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkRenderingAttachmentInfo)
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = Vulkan.SwapchainImageViews[Vulkan.ImageIndex],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue =
                {
                    .color = {.float32 = {
                        Vulkan.ClearColor.R,
                        Vulkan.ClearColor.G,
                        Vulkan.ClearColor.B,
                        Vulkan.ClearColor.A,
                    }},
                },
            },
        };

        vkCmdBeginRendering(Vulkan.CommandBuffer, &RenderingInfo);
    }

    // NOTE(vak): End rendering
    {
        vkCmdEndRendering(Vulkan.CommandBuffer);
    }

    // NOTE(vak): Present barrier
    {
        VkImageMemoryBarrier PresentBarrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Vulkan.SwapchainImages[Vulkan.ImageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            Vulkan.CommandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, 0, 0, 0,
            1, &PresentBarrier
        );
    }

    // NOTE(vak): End command buffer
    {
        VulkanCheck(vkEndCommandBuffer(Vulkan.CommandBuffer));
    }

    // NOTE(vak): Submit
    {
        VkPipelineStageFlags WaitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo SubmitInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &Vulkan.AcquireSemaphore,
            .pWaitDstStageMask = &WaitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &Vulkan.CommandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &Vulkan.SubmitSemaphore,
        };

        VulkanCheck(vkQueueSubmit(Vulkan.Queue, 1, &SubmitInfo, 0));
    }

    // NOTE(vak): Present
    {
        VkPresentInfoKHR PresentInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &Vulkan.SubmitSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &Vulkan.Swapchain,
            .pImageIndices = &Vulkan.ImageIndex,
        };

        VulkanCheck(vkQueuePresentKHR(Vulkan.Queue, &PresentInfo));
    }

    // NOTE(vak): Wait until device finished
    // TODO(vak): Better synchronization to support frames in flight?
    {
        VulkanCheck(vkDeviceWaitIdle(Vulkan.Device));
    }
}


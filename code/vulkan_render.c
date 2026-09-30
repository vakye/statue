
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
    X(vkGetPhysicalDeviceMemoryProperties) \
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
    X(vkCreateShaderModule) \
    X(vkDestroyShaderModule) \
    \
    X(vkCreateDescriptorSetLayout) \
    X(vkCreatePipelineLayout) \
    X(vkCreateGraphicsPipelines) \
    \
    X(vkAllocateMemory) \
    X(vkMapMemory) \
    \
    X(vkCreateBuffer) \
    X(vkGetBufferMemoryRequirements) \
    X(vkBindBufferMemory) \
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
    X(vkCmdBindPipeline) \
    X(vkCmdSetViewport) \
    X(vkCmdSetScissor) \
    X(vkCmdPushDescriptorSet) \
    X(vkCmdDraw) \
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
    VkBuffer        Buffer;
    VkDeviceMemory  Memory;
    usize           Size;
    void*           Mapping;
} vulkan_buffer;

typedef struct
{
    arena_id                    ArenaID;

    u32                         VersionOfAPI;
    VkInstance                  Instance;
    VkSurfaceKHR                Surface;
    VkPhysicalDevice            PhysicalDevice;
    u32                         QueueFamilyIndex;
    VkDevice                    Device;
    VkQueue                     Queue;
    VkCommandPool               CommandPool;
    VkCommandBuffer             CommandBuffer;
    VkSemaphore                 AcquireSemaphore;
    VkSemaphore                 SubmitSemaphore;
    VkSurfaceFormatKHR          SwapchainFormat;
    VkPresentModeKHR            PresentMode;

    VkDescriptorSetLayout       SetLayout;
    VkPipelineLayout            PipelineLayout;
    VkPipeline                  Pipeline;

    vulkan_buffer               VertexBuffer;

    VkExtent2D                  SwapchainExtent;
    VkSwapchainKHR              Swapchain;
    u32                         SwapchainImageCount;
    VkImage*                    SwapchainImages;
    VkImageView*                SwapchainImageViews;

    v4                          ClearColor;
    u32                         ImageIndex;
    u32                         VertexCount;
} vulkan_state;

typedef struct
{
    v2 Position;
    v2 TexCoord;
    v4 Color;
} vulkan_vertex;

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

local u32 VulkanSelectMemoryType(
    VkMemoryPropertyFlags   PropertyFlags,
    u32                     MemoryTypeBits
)
{
    u32 Result = U32Max;

    VkPhysicalDeviceMemoryProperties MemoryProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(Vulkan.PhysicalDevice, &MemoryProperties);

    for (u32 Index = 0; Index < MemoryProperties.memoryTypeCount; Index++)
    {
        if ((MemoryTypeBits & (1 << Index)) == 0)
            continue;

        VkMemoryType* MemoryType = MemoryProperties.memoryTypes + Index;

        if ((MemoryType->propertyFlags & PropertyFlags) == PropertyFlags)
        {
            Result = Index;
            break;
        }
    }

    if (Result == U32Max)
        VulkanFatalError(Str("unable to select a suitable memory type"));

    return (Result);
}

local void VulkanCreateBuffer(
    vulkan_buffer*          Buffer,
    usize                   Size,
    VkBufferUsageFlags      UsageFlags,
    VkMemoryPropertyFlags   MemoryPropertyFlags,
    b32                     Mapped)
{
    ZeroStruct(Buffer);

    Buffer->Size = Size;

    VkBufferCreateInfo BufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Buffer->Size,
        .usage = UsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VulkanCheck(vkCreateBuffer(Vulkan.Device, &BufferInfo, 0, &Buffer->Buffer));

    VkMemoryRequirements MemoryRequirements = {0};
    vkGetBufferMemoryRequirements(Vulkan.Device, Buffer->Buffer, &MemoryRequirements);

    VkMemoryAllocateInfo AllocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = VulkanSelectMemoryType(
            MemoryPropertyFlags,
            MemoryRequirements.memoryTypeBits
        ),
    };

    VulkanCheck(vkAllocateMemory(Vulkan.Device, &AllocateInfo, 0, &Buffer->Memory));
    VulkanCheck(vkBindBufferMemory(Vulkan.Device, Buffer->Buffer, Buffer->Memory, 0));

    if (Mapped)
    {
        VulkanCheck(vkMapMemory(
            Vulkan.Device,
            Buffer->Memory,
            0,
            Buffer->Size,
            0,
            &Buffer->Mapping
        ));
    }
}

local void SetupRenderer(void)
{
    // NOTE(vak): Arena
    {
        Vulkan.ArenaID = MakeArena(KB(64), GB(4));
    }

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
        temporary_memory EnumerateMemory = BeginTemporaryMemory(Vulkan.ArenaID);

        u32 PhysicalDeviceCount = 0;

        VulkanCheck(vkEnumeratePhysicalDevices(
            Vulkan.Instance,
            &PhysicalDeviceCount,
            0
        ));

        VkPhysicalDevice* PhysicalDevices = PushArenaArray(
            EnumerateMemory.ArenaID,
            VkPhysicalDevice,
            PhysicalDeviceCount
        );

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

        EndTemporaryMemory(EnumerateMemory);
    }

    // NOTE(vak): Queue family index
    {
        temporary_memory EnumerateMemory = BeginTemporaryMemory(Vulkan.ArenaID);

        u32 QueueFamilyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(
            Vulkan.PhysicalDevice,
            &QueueFamilyCount,
            0
        );

        VkQueueFamilyProperties* QueueFamiliesProperties = PushArenaArray(
            EnumerateMemory.ArenaID,
            VkQueueFamilyProperties,
            QueueFamilyCount
        );

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

        EndTemporaryMemory(EnumerateMemory);
    }

    // NOTE(vak): Device
    {
        const char* Extensions[] =
        {
            "VK_KHR_swapchain",
        };

        VkPhysicalDeviceVulkan14Features Vulkan14Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
            .pushDescriptor = true,
        };

        VkPhysicalDeviceVulkan13Features Vulkan13Features =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &Vulkan14Features,
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
        temporary_memory EnumerateMemory = BeginTemporaryMemory(Vulkan.ArenaID);

        u32 SurfaceFormatCount = 0;

        VulkanCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            Vulkan.PhysicalDevice,
            Vulkan.Surface,
            &SurfaceFormatCount,
            0
        ));

        VkSurfaceFormatKHR* SurfaceFormats = PushArenaArray(
            EnumerateMemory.ArenaID,
            VkSurfaceFormatKHR,
            SurfaceFormatCount
        );

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

        EndTemporaryMemory(EnumerateMemory);
    }

    // NOTE(vak): Present mode
    {
        temporary_memory EnumerateMemory = BeginTemporaryMemory(Vulkan.ArenaID);

        u32 PresentModeCount = 0;

        VulkanCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
            Vulkan.PhysicalDevice,
            Vulkan.Surface,
            &PresentModeCount,
            0
        ));

        VkPresentModeKHR* PresentModes = PushArenaArray(
            EnumerateMemory.ArenaID,
            VkPresentModeKHR,
            PresentModeCount
        );

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

        EndTemporaryMemory(EnumerateMemory);
    }

    // NOTE(vak): Descriptor set layout
    {
        VkDescriptorSetLayoutBinding Bindings[] =
        {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            },
        };

        VkDescriptorSetLayoutCreateInfo SetLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
            .bindingCount = ArrayCount(Bindings),
            .pBindings = Bindings,
        };

        VulkanCheck(vkCreateDescriptorSetLayout(Vulkan.Device, &SetLayoutInfo, 0, &Vulkan.SetLayout));
    }

    // NOTE(vak): Pipeline layout
    {
        VkPipelineLayoutCreateInfo PipelineLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &Vulkan.SetLayout,
        };

        VulkanCheck(vkCreatePipelineLayout(Vulkan.Device, &PipelineLayoutInfo, 0, &Vulkan.PipelineLayout));
    }

    // NOTE(vak): Pipeline
    {
        persist u32 VertexCode[] =
        {
            #include "shaders/basic.vert.h"
        };

        persist u32 FragmentCode[] =
        {
            #include "shaders/basic.frag.h"
        };

        VkShaderModuleCreateInfo VertexModuleInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = sizeof(VertexCode),
            .pCode = VertexCode,
        };

        VkShaderModuleCreateInfo FragmentModuleInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = sizeof(FragmentCode),
            .pCode = FragmentCode,
        };

        VkShaderModule VertexModule = {0};
        VkShaderModule FragmentModule = {0};

        VulkanCheck(vkCreateShaderModule(Vulkan.Device, &VertexModuleInfo, 0, &VertexModule));
        VulkanCheck(vkCreateShaderModule(Vulkan.Device, &FragmentModuleInfo, 0, &FragmentModule));

        VkPipelineShaderStageCreateInfo StageInfos[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = VertexModule,
                .pName = "main",
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = FragmentModule,
                .pName = "main",
            },
        };

        VkPipelineVertexInputStateCreateInfo VertexInputStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        };

        VkPipelineTessellationStateCreateInfo TessellationStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        };

        VkPipelineViewportStateCreateInfo ViewportStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &(VkViewport){0},
            .scissorCount = 1,
            .pScissors = &(VkRect2D){0},
        };

        VkPipelineRasterizationStateCreateInfo RasterizationStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo MultisampleStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };

        VkPipelineDepthStencilStateCreateInfo DepthStencilStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        };

        VkPipelineColorBlendStateCreateInfo ColorBlendStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &(VkPipelineColorBlendAttachmentState)
            {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT |
                    VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT,
            },
        };

        VkDynamicState DynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo DynamicStateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = ArrayCount(DynamicStates),
            .pDynamicStates = DynamicStates,
        };

        VkPipelineRenderingCreateInfo RenderingInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &Vulkan.SwapchainFormat.format,
        };

        VkGraphicsPipelineCreateInfo PipelineInfo =
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &RenderingInfo,
            .stageCount = ArrayCount(StageInfos),
            .pStages = StageInfos,
            .pVertexInputState = &VertexInputStateInfo,
            .pInputAssemblyState = &InputAssemblyStateInfo,
            .pTessellationState = &TessellationStateInfo,
            .pViewportState = &ViewportStateInfo,
            .pRasterizationState = &RasterizationStateInfo,
            .pMultisampleState = &MultisampleStateInfo,
            .pDepthStencilState = &DepthStencilStateInfo,
            .pColorBlendState = &ColorBlendStateInfo,
            .pDynamicState = &DynamicStateInfo,
            .layout = Vulkan.PipelineLayout,
        };

        VulkanCheck(vkCreateGraphicsPipelines(Vulkan.Device, 0, 1, &PipelineInfo, 0, &Vulkan.Pipeline));

        vkDestroyShaderModule(Vulkan.Device, FragmentModule, 0);
        vkDestroyShaderModule(Vulkan.Device, VertexModule, 0);
    }

    // NOTE(vak): Vertex buffer
    {
        usize MaxRectPerDraw = 16384;
        usize MaxVerticesPerDraw = MaxRectPerDraw * 6;
        usize VertexBufferSize = MaxVerticesPerDraw * sizeof(vulkan_vertex);

        VulkanCreateBuffer(
            &Vulkan.VertexBuffer,
            VertexBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_TRUE
        );
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

            VkSurfaceCapabilitiesKHR SurfaceCapabilities = {0};
            VulkanCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                Vulkan.PhysicalDevice,
                Vulkan.Surface,
                &SurfaceCapabilities
            ));

            if (!Vulkan.SwapchainImages)
            {
                Vulkan.SwapchainImages = PushArenaArray(
                    Vulkan.ArenaID,
                    VkImage,
                    SurfaceCapabilities.maxImageCount
                );

                Vulkan.SwapchainImageViews = PushArenaArray(
                    Vulkan.ArenaID,
                    VkImageView,
                    SurfaceCapabilities.maxImageCount
                );

                ZeroArray(Vulkan.SwapchainImages, SurfaceCapabilities.maxImageCount);
                ZeroArray(Vulkan.SwapchainImageViews, SurfaceCapabilities.maxImageCount);
            }

            for (u32 Index = 0; Index < Vulkan.SwapchainImageCount; Index++)
                if (Vulkan.SwapchainImageViews[Index])
                    vkDestroyImageView(Vulkan.Device, Vulkan.SwapchainImageViews[Index], 0);

            if (Vulkan.Swapchain)
                vkDestroySwapchainKHR(Vulkan.Device, Vulkan.Swapchain, 0);

            Vulkan.SwapchainExtent.width  = WindowSizeX;
            Vulkan.SwapchainExtent.height = WindowSizeY;

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

            Vulkan.SwapchainImageCount = SurfaceCapabilities.maxImageCount;

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

    // NOTE(vak): Reset
    {
        Vulkan.VertexCount = 0;
    }
}

local void RenderRect(rect2 Rect, v4 Color)
{
    usize MaxVertexCount = Vulkan.VertexBuffer.Size / sizeof(vulkan_vertex);

    // TODO(vak): Support multiple draws in one frame, so we don't
    // have to panic when the vertex buffer runs out of space.

    if (Vulkan.VertexCount + 6 > MaxVertexCount)
        VulkanFatalError(Str("vertex buffer out of space"));

    vulkan_vertex* V = (vulkan_vertex*)Vulkan.VertexBuffer.Mapping + Vulkan.VertexCount;

    v2 Min = Rect.Min;
    v2 Max = Rect.Max;

    V[0] = (vulkan_vertex){V2(Min.X, Min.Y), V2(0.0f, 0.0f), Color};
    V[1] = (vulkan_vertex){V2(Max.X, Min.Y), V2(1.0f, 0.0f), Color};
    V[2] = (vulkan_vertex){V2(Max.X, Max.Y), V2(1.0f, 1.0f), Color};

    V[3] = (vulkan_vertex){V2(Max.X, Max.Y), V2(1.0f, 1.0f), Color};
    V[4] = (vulkan_vertex){V2(Min.X, Max.Y), V2(0.0f, 1.0f), Color};
    V[5] = (vulkan_vertex){V2(Min.X, Min.Y), V2(0.0f, 0.0f), Color};

    Vulkan.VertexCount += 6;
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

    // NOTE(vak): Dispatch draw
    if (Vulkan.VertexCount)
    {
        vkCmdBindPipeline(Vulkan.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Vulkan.Pipeline);

        VkViewport Viewport =
        {
            .x = 0.0f,
            .y = (f32)Vulkan.SwapchainExtent.height,
            .width = (f32)Vulkan.SwapchainExtent.width,
            .height = -(f32)Vulkan.SwapchainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D Scissor =
        {
            .offset = {.x = 0, .y = 0},
            .extent = Vulkan.SwapchainExtent,
        };

        vkCmdSetViewport(Vulkan.CommandBuffer, 0, 1, &Viewport);
        vkCmdSetScissor(Vulkan.CommandBuffer, 0, 1, &Scissor);

        VkWriteDescriptorSet DescriptorWrites[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = 0,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &(VkDescriptorBufferInfo)
                {
                    .buffer = Vulkan.VertexBuffer.Buffer,
                    .offset = 0,
                    .range = Vulkan.VertexBuffer.Size,
                },
            },
        };

        vkCmdPushDescriptorSet(
            Vulkan.CommandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            Vulkan.PipelineLayout,
            0,
            ArrayCount(DescriptorWrites),
            DescriptorWrites
        );

        vkCmdDraw(Vulkan.CommandBuffer, Vulkan.VertexCount, 1, 0, 0);
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


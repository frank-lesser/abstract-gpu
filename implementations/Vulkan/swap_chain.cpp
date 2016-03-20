#include "swap_chain.hpp"
#include "texture.hpp"
#include "texture_format.hpp"
#include "framebuffer.hpp"

_agpu_swap_chain::_agpu_swap_chain(agpu_device *device)
    : device(device)
{
    surface = nullptr;
    graphicsQueue = nullptr;
    presentationQueue = nullptr;
    handle = nullptr;
}

void _agpu_swap_chain::lostReferences()
{
    if (graphicsQueue)
        graphicsQueue->release();
    if (presentationQueue)
        presentationQueue->release();

    for (auto fb : framebuffers)
    {
        if(fb)
            fb->release();
    }
    if (handle)
        vkDestroySwapchainKHR(device->device, handle, nullptr);
    if(surface)
        vkDestroySurfaceKHR(device->vulkanInstance, surface, nullptr);
}

_agpu_swap_chain *_agpu_swap_chain::create(agpu_device *device, agpu_command_queue* graphicsCommandQueue, agpu_swap_chain_create_info *createInfo)
{
    VkSurfaceKHR surface;
    if (!graphicsCommandQueue || !createInfo)
        return nullptr;

#ifdef _WIN32
    if (!createInfo->window)
        return nullptr;
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
    memset(&surfaceCreateInfo, 0, sizeof(surfaceCreateInfo));
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    surfaceCreateInfo.hwnd = (HWND)createInfo->window;

    auto error = vkCreateWin32SurfaceKHR(device->vulkanInstance, &surfaceCreateInfo, nullptr, &surface);
#endif
    if (error)
    {
        printError("Failed to create the swap chain surface\n");
        return false;
    }

    agpu_command_queue *presentationQueue = graphicsCommandQueue;
    if (!graphicsCommandQueue->supportsPresentingSurface(surface))
    {
        // TODO: Find a presentation queue.
        vkDestroySurfaceKHR(device->vulkanInstance, surface, nullptr);
        printError("Surface presentation in different queue is not yet supported.\n");
        return nullptr;
    }

    uint32_t formatCount = 0;
    error = device->fpGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, surface, &formatCount, nullptr);
    if (error)
    {
        vkDestroySurfaceKHR(device->vulkanInstance, surface, nullptr);
        return nullptr;
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    error = device->fpGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, surface, &formatCount, &surfaceFormats[0]);
    if (error)
    {
        vkDestroySurfaceKHR(device->vulkanInstance, surface, nullptr);
        return nullptr;
    }

    // Create the swap chain object.
    auto swapChain = new agpu_swap_chain(device);
    swapChain->surface = surface;
    swapChain->graphicsQueue = graphicsCommandQueue;
    swapChain->presentationQueue = presentationQueue;
    graphicsCommandQueue->retain();
    presentationQueue->retain();

    // Set the format.
    if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        swapChain->format = mapTextureFormat(createInfo->colorbuffer_format);
        if (swapChain->format == VK_FORMAT_UNDEFINED)
            swapChain->format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        swapChain->format = surfaceFormats[0].format;
    }
    swapChain->colorSpace = surfaceFormats[0].colorSpace;

    // Initialize the rest of the swap chain.
    if (!swapChain->initialize(createInfo))
    {
        swapChain->release();
        return nullptr;
    }

    return swapChain;
}

bool _agpu_swap_chain::initialize(agpu_swap_chain_create_info *createInfo)
{
    swapChainWidth = createInfo->width;
    swapChainHeight = createInfo->height;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    auto error = device->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice, surface, &surfaceCapabilities);
    if (error)
        return false;

    uint32_t presentModeCount;
    error = device->fpGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, surface, &presentModeCount, nullptr);
    if (error)
        return false;

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    error = device->fpGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, surface, &presentModeCount, &presentModes[0]);
    if (error)
        return false;

    VkExtent2D swapchainExtent;
    if (surfaceCapabilities.currentExtent.width == (uint32_t)-1)
    {
        swapchainExtent.width = swapChainWidth;
        swapchainExtent.height = swapChainHeight;
    }
    else
    {
        swapchainExtent = surfaceCapabilities.currentExtent;
        swapChainWidth = swapchainExtent.width;
        swapChainHeight = swapchainExtent.height;
    }

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    uint32_t desiredNumberOfSwapchainImages = surfaceCapabilities.minImageCount + 1;
    if ((surfaceCapabilities.maxImageCount > 0) &&
        (desiredNumberOfSwapchainImages > surfaceCapabilities.maxImageCount))
        desiredNumberOfSwapchainImages = surfaceCapabilities.maxImageCount;

    VkSurfaceTransformFlagsKHR preTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        preTransform = surfaceCapabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchainInfo;
    memset(&swapchainInfo, 0, sizeof(swapchainInfo));
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = desiredNumberOfSwapchainImages;
    swapchainInfo.imageFormat = format;
    swapchainInfo.imageColorSpace = colorSpace;
    swapchainInfo.imageExtent = swapchainExtent;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.presentMode = swapchainPresentMode;
    swapchainInfo.oldSwapchain = handle;

    error = device->fpCreateSwapchainKHR(device->device, &swapchainInfo, nullptr, &handle);
    if (error)
        return false;

    uint32_t imageCount;
    error = device->fpGetSwapchainImagesKHR(device->device, handle, &imageCount, nullptr);
    if (error)
        return false;

    std::vector<VkImage> swapChainImages(imageCount);
    error = device->fpGetSwapchainImagesKHR(device->device, handle, &imageCount, &swapChainImages[0]);
    if (error)
        return false;

    // Color buffers descriptions
    agpu_texture_description colorDesc;
    memset(&colorDesc, 0, sizeof(colorDesc));
    colorDesc.type = AGPU_TEXTURE_2D;
    colorDesc.width = createInfo->width;
    colorDesc.height = createInfo->height;
    colorDesc.depthOrArraySize = 1;
    colorDesc.format = createInfo->colorbuffer_format;
    colorDesc.flags = agpu_texture_flags(AGPU_TEXTURE_FLAG_RENDER_TARGET | AGPU_TEXTURE_FLAG_RENDERBUFFER_ONLY);
    colorDesc.miplevels = 1;
    colorDesc.sample_count = 1;

    // Depth stencil buffer descriptions.
    agpu_texture_description depthStencilDesc;
    memset(&depthStencilDesc, 0, sizeof(depthStencilDesc));
    depthStencilDesc.type = AGPU_TEXTURE_2D;
    depthStencilDesc.width = createInfo->width;
    depthStencilDesc.height = createInfo->height;
    depthStencilDesc.depthOrArraySize = 1;
    depthStencilDesc.format = createInfo->depth_stencil_format;
    depthStencilDesc.flags = agpu_texture_flags(AGPU_TEXTURE_FLAG_RENDERBUFFER_ONLY);
    depthStencilDesc.miplevels = 1;

    bool hasDepth = hasDepthComponent(createInfo->depth_stencil_format);
    bool hasStencil = hasStencilComponent(createInfo->depth_stencil_format);
    if (hasDepth)
        depthStencilDesc.flags = agpu_texture_flags(depthStencilDesc.flags | AGPU_TEXTURE_FLAG_DEPTH);
    if (hasStencil)
        depthStencilDesc.flags = agpu_texture_flags(depthStencilDesc.flags | AGPU_TEXTURE_FLAG_STENCIL);

    agpu_texture_view_description colorViewDesc;
    agpu_texture_view_description depthStencilViewDesc;
    auto depthStencilViewPointer = &depthStencilViewDesc;
    if (!hasDepth && !hasStencil)
        depthStencilViewPointer = nullptr;

    framebuffers.resize(imageCount);
    for (size_t i = 0; i < imageCount; ++i)
    {
        auto colorImage = swapChainImages[i];
        agpu_texture *colorBuffer = nullptr;
        agpu_texture *depthStencilBuffer = nullptr;

        {
            colorBuffer = agpu_texture::createFromImage(device, &colorDesc, colorImage);
            if (!colorBuffer)
                return false;

            // Get the color buffer view description.
            colorBuffer->getFullViewDescription(&colorViewDesc);
        }

        {
            depthStencilBuffer = agpu_texture::create(device, &depthStencilDesc);
            if (!depthStencilBuffer)
            {
                colorBuffer->release();
                return false;
            }

            // Get the depth stencil buffer view description.
            depthStencilBuffer->getFullViewDescription(&depthStencilViewDesc);
        }

        auto framebuffer = agpu_framebuffer::create(device, swapChainWidth, swapChainHeight, 1, &colorViewDesc, depthStencilViewPointer);
        framebuffers[i] = framebuffer;

        // Release the references to the buffers.
        colorBuffer->release();
        depthStencilBuffer->release();

        if (!framebuffer)
            return false;
    }

    return true;
}

agpu_error _agpu_swap_chain::swapBuffers()
{
    return AGPU_UNIMPLEMENTED;
}

agpu_framebuffer *_agpu_swap_chain::getCurrentBackBuffer()
{
    return nullptr;
}

// Exported C interface
AGPU_EXPORT agpu_error agpuAddSwapChainReference(agpu_swap_chain* swap_chain)
{
    CHECK_POINTER(swap_chain);
    return swap_chain->retain();
}

AGPU_EXPORT agpu_error agpuReleaseSwapChain(agpu_swap_chain* swap_chain)
{
    CHECK_POINTER(swap_chain);
    return swap_chain->release();
}

AGPU_EXPORT agpu_error agpuSwapBuffers(agpu_swap_chain* swap_chain)
{
    CHECK_POINTER(swap_chain);
    return swap_chain->swapBuffers();
}

AGPU_EXPORT agpu_framebuffer* agpuGetCurrentBackBuffer(agpu_swap_chain* swap_chain)
{
    if (!swap_chain)
        return nullptr;
    return swap_chain->getCurrentBackBuffer();
}

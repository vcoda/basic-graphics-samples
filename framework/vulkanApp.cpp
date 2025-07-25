#include "vulkanApp.h"
#include "utilities.h"

VulkanApp::VulkanApp(const AppEntry& entry, const std::tstring& caption, uint32_t width, uint32_t height,
    bool depthBuffer /* false */):
    NativeApp(entry, caption, width, height),
    waitFence(&nullFence),
    timer(std::make_unique<Timer>()),
    vSync(false),
    depthBuffer(depthBuffer),
    negateViewport(false),
    presentWait(PresentationWait::Fence),
    bufferIndex(0),
    frameIndex(0)
{}

VulkanApp::~VulkanApp() {}

void VulkanApp::close()
{
    device->waitIdle();
    quit = true;
}

void VulkanApp::onIdle()
{
    onPaint();
}

void VulkanApp::onPaint()
{
    bufferIndex = swapchain->acquireNextImage(presentFinished);
    if (PresentationWait::Fence == presentWait)
    {   // Fence to be signaled when command buffer completed execution
        waitFences[bufferIndex]->reset();
        waitFence = &waitFences[bufferIndex];
    }
    render(bufferIndex);
    graphicsQueue->present(swapchain, bufferIndex, renderFinished);
    switch (presentWait)
    {
    case PresentationWait::Fence:
        if (waitFence)
        {
            (*waitFence)->wait();
            graphicsQueue->onIdle();
        }
        break;
    case PresentationWait::Queue:
        graphicsQueue->waitIdle();
        break;
    case PresentationWait::Device:
        // vkDeviceWaitIdle is equivalent to calling vkQueueWaitIdle
        // for all queues owned by device.
        device->waitIdle();
        break;
    }
    if (!vSync)
    {   // Cap fps
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ++frameIndex;
}

void VulkanApp::initialize()
{
    createInstance();
    createLogicalDevice();
    createSwapchain();
    createRenderPass();
    createFramebuffer();
    createCommandBuffers();
    createSyncPrimitives();
    createDescriptorPool();
    pipelineCache = std::make_unique<magma::PipelineCache>(device);
    shaderReflectionFactory = std::make_unique<ShaderReflectionFactory>(device);
}

void VulkanApp::createInstance()
{
    std::vector<const char*> layerNames;
#ifdef _DEBUG
    std::unique_ptr<magma::InstanceLayers> instanceLayers = std::make_unique<magma::InstanceLayers>();
    if (instanceLayers->KHRONOS_validation)
        layerNames.push_back("VK_LAYER_KHRONOS_validation");
    else if (instanceLayers->LUNARG_standard_validation)
        layerNames.push_back("VK_LAYER_LUNARG_standard_validation");
#endif // _DEBUG

    magma::NullTerminatedStringArray enabledExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
    #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME
    #elif defined(VK_USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    #endif // VK_USE_PLATFORM_XCB_KHR
    };
    instanceExtensions = std::make_unique<magma::InstanceExtensions>();
#ifdef VK_EXT_debug_utils
    if (instanceExtensions->EXT_debug_utils)
        enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    else
#endif //  VK_EXT_debug_utils
    {
    #ifdef VK_EXT_debug_report
        if (instanceExtensions->EXT_debug_report)
            enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    #endif
    }
    MAGMA_VLA(char, appName, caption.length() + 1);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    size_t count = 0;
    wcstombs_s(&count, appName, appName.length(), caption.c_str(), appName.length());
#else
    strcpy(appName, caption.c_str());
#endif
    const magma::Application appInfo(
        appName, 1,
        "Magma", 1,
        VK_API_VERSION_1_0);

    instance = std::make_unique<magma::Instance>(layerNames, enabledExtensions, nullptr, &appInfo, 0,
    #ifdef VK_EXT_debug_report
        utilities::reportCallback,
    #endif
    #ifdef VK_EXT_debug_utils
        nullptr,
    #endif
        nullptr); // userData
#ifdef VK_EXT_debug_report
    if (instanceExtensions->EXT_debug_report)
    {
        debugReportCallback = std::make_unique<magma::DebugReportCallback>(
            instance.get(), utilities::reportCallback);
    }
#endif // VK_EXT_debug_report

    physicalDevice = instance->getPhysicalDevice(0);
    const VkPhysicalDeviceProperties& properties = physicalDevice->getProperties();
    std::cout << "Running on " << properties.deviceName << "\n";

    instanceExtensions = std::make_unique<magma::InstanceExtensions>();
    extensions = std::make_unique<magma::DeviceExtensions>(physicalDevice);

    // https://stackoverflow.com/questions/48036410/why-doesnt-vulkan-use-the-standard-cartesian-coordinate-system
#if defined(VK_KHR_maintenance1) || defined(VK_AMD_negative_viewport_height)
    negateViewport = extensions->KHR_maintenance1 || extensions->AMD_negative_viewport_height;
#endif
}

void VulkanApp::createLogicalDevice()
{
    const magma::DeviceQueueDescriptor graphicsQueueDesc(physicalDevice.get(), VK_QUEUE_GRAPHICS_BIT, magma::QueuePriorityHighest);
    const magma::DeviceQueueDescriptor transferQueueDesc(physicalDevice.get(), VK_QUEUE_TRANSFER_BIT, magma::QueuePriorityDefault);
    std::set<magma::DeviceQueueDescriptor> queueDescriptors;
    queueDescriptors.insert(graphicsQueueDesc);
    queueDescriptors.insert(transferQueueDesc);

    // Enable some widely used features
    VkPhysicalDeviceFeatures features = {};
    features.fillModeNonSolid = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;
    features.textureCompressionBC = VK_TRUE;
    features.occlusionQueryPrecise = VK_TRUE;

    magma::NullTerminatedStringArray enabledExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
#ifdef VK_KHR_maintenance1
    if (extensions->KHR_maintenance1)
        enabledExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
    else
#endif // VK_KHR_maintenance1
    {
    #ifdef VK_AMD_negative_viewport_height
        if (extensions->AMD_negative_viewport_height)
            enabledExtensions.push_back(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
    #endif // VK_AMD_negative_viewport_height
    }
#ifdef VK_EXT_debug_marker
    if (extensions->EXT_debug_marker)
        enabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
#endif // VK_EXT_debug_marker

    const std::vector<const char*> noLayers;
    device = physicalDevice->createDevice(queueDescriptors, noLayers, enabledExtensions, features);
}

void VulkanApp::createSwapchain()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    surface = std::make_unique<magma::Win32Surface>(instance.get(), hInstance, hWnd);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    surface = std::make_unique<magma::XlibSurface>(instance.get(), dpy, window);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    surface = std::make_unique<magma::XcbSurface>(instance.get(), connection, window);
#endif // VK_USE_PLATFORM_XCB_KHR
    const magma::DeviceQueueDescriptor graphicsQueue(physicalDevice.get(), VK_QUEUE_GRAPHICS_BIT, {1.f});
    if (!physicalDevice->getSurfaceSupport(surface, graphicsQueue.queueFamilyIndex))
        throw std::runtime_error("surface not supported");
    // Get surface caps
    VkSurfaceCapabilitiesKHR surfaceCaps;
    surfaceCaps = physicalDevice->getSurfaceCapabilities(surface);
    assert(surfaceCaps.currentExtent.width == width);
    assert(surfaceCaps.currentExtent.height == height);
    // Find supported transform flags
    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        preTransform = surfaceCaps.currentTransform;
    // Find supported alpha composite
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (const auto alphaFlag : {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
    {
        if (surfaceCaps.supportedCompositeAlpha & alphaFlag)
        {
            compositeAlpha = alphaFlag;
            break;
        }
    }
    const std::vector<VkSurfaceFormatKHR> surfaceFormats = physicalDevice->getSurfaceFormats(surface);
    // Choose available present mode
    const std::vector<VkPresentModeKHR> presentModes = physicalDevice->getSurfacePresentModes(surface);
    VkPresentModeKHR presentMode;
    if (vSync)
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    else
    {   // Search for first appropriate present mode
        bool found = false;
        for (const auto mode : {
            VK_PRESENT_MODE_IMMEDIATE_KHR,  // AMD
            VK_PRESENT_MODE_MAILBOX_KHR,    // NVidia, Intel
            VK_PRESENT_MODE_FIFO_RELAXED_KHR})
        {
            if (std::find(presentModes.begin(), presentModes.end(), mode) != presentModes.end())
            {
                presentMode = mode;
                found = true;
                break;
            }
        }
        if (!found)
        {   // Must always be present
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
        }
    }
    magma::Swapchain::Initializer initializer;
    initializer.debugReportCallback = debugReportCallback.get();
    swapchain = std::make_unique<magma::Swapchain>(device, surface,
        std::max(surfaceCaps.minImageCount, 2U),
        surfaceFormats[0], surfaceCaps.currentExtent, 1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // Allow screenshots
        preTransform, compositeAlpha, presentMode, initializer);
}

void VulkanApp::createRenderPass()
{
    const magma::AttachmentDescription colorAttachment(swapchain->getSurfaceFormat().format, 1,
        magma::op::clearStore, // Color clear, store
        magma::op::dontCare,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    if (depthBuffer)
    {
        const VkFormat depthFormat = utilities::getSupportedDepthFormat(physicalDevice, false, true);
        const magma::AttachmentDescription depthStencilAttachment(depthFormat, 1,
            magma::op::clearStore, // Depth clear, store
            magma::op::dontCare, // Stencil don't care
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        const std::initializer_list<magma::AttachmentDescription> attachments = {colorAttachment, depthStencilAttachment};
        renderPass = std::make_unique<magma::RenderPass>(device, attachments);
    }
    else
    {
        renderPass = std::make_unique<magma::RenderPass>(device, colorAttachment);
    }
}

void VulkanApp::createFramebuffer()
{
    const VkSurfaceCapabilitiesKHR surfaceCaps = physicalDevice->getSurfaceCapabilities(surface);
    if (depthBuffer)
    {
        const VkFormat depthFormat = utilities::getSupportedDepthFormat(physicalDevice, false, true);
        constexpr bool dontSampled = false;
        std::unique_ptr<magma::Image> depthStencil = std::make_unique<magma::DepthStencilAttachment>(device, depthFormat, surfaceCaps.currentExtent, 1, 1, dontSampled);
        depthStencilView = std::make_shared<magma::UniqueImageView>(std::move(depthStencil));
    }
    for (const auto& image : swapchain->getImages())
    {
        std::vector<std::shared_ptr<magma::ImageView>> attachments;
        std::shared_ptr<magma::SharedImageView> colorView = std::make_shared<magma::SharedImageView>(std::move(image));
        attachments.emplace_back(std::move(colorView));
        if (depthBuffer)
            attachments.push_back(depthStencilView);
        std::unique_ptr<magma::Framebuffer> framebuffer = std::make_unique<magma::Framebuffer>(renderPass, attachments);
        framebuffers.emplace_back(std::move(framebuffer));
    }
}

void VulkanApp::createCommandBuffers()
{
    graphicsQueue = device->getQueue(VK_QUEUE_GRAPHICS_BIT, 0);
    commandPools[0] = std::make_unique<magma::CommandPool>(device, graphicsQueue->getFamilyIndex());
    // Create draw command buffers
    commandBuffers = commandPools[0]->allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, magma::core::countof(framebuffers));
    // Create image copy command buffer
    cmdImageCopy = commandPools[0]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    try
    {
        transferQueue = device->getQueue(VK_QUEUE_TRANSFER_BIT, 0);
        if (transferQueue)
        {
            commandPools[1] = std::make_unique<magma::CommandPool>(device, transferQueue->getFamilyIndex());
            // Create buffer copy command buffer
            cmdBufferCopy = commandPools[1]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        }
    } catch (...) { std::cout << "transfer queue not present" << std::endl; }
}

void VulkanApp::createSyncPrimitives()
{
    presentFinished = std::make_shared<magma::Semaphore>(device);
    renderFinished = std::make_shared<magma::Semaphore>(device);
    for (int i = 0; i < (int)commandBuffers.size(); ++i)
        waitFences.push_back(std::make_unique<magma::Fence>(device, nullptr, VK_FENCE_CREATE_SIGNALED_BIT));
}

void VulkanApp::createDescriptorPool()
{
    constexpr uint32_t maxDescriptorSets = 2;
    // Create descriptor pool enough for basic samples
    descriptorPool = std::make_shared<magma::DescriptorPool>(device, maxDescriptorSets,
        std::vector<magma::descriptor::DescriptorPool>{
            magma::descriptor::UniformBufferPool(4),
            magma::descriptor::DynamicUniformBufferPool(4),
            magma::descriptor::StorageBufferPool(4),
            magma::descriptor::CombinedImageSamplerPool(4)
        });
}

void VulkanApp::imageLayoutTransition(std::shared_ptr<magma::Image> image, VkImageLayout newLayout)
{
    cmdImageCopy->begin();
    image->layoutTransition(newLayout, cmdImageCopy);
    cmdImageCopy->end();
    submitCopyImageCommands();
}

void VulkanApp::submitCommandBuffer(uint32_t bufferIndex)
{
    graphicsQueue->submit(commandBuffers[bufferIndex],
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        presentFinished, // Wait for swapchain
        renderFinished, // Semaphore to be signaled when command buffer completed execution
        *waitFence); // Fence to be signaled when command buffer completed execution
}

void VulkanApp::submitCopyImageCommands()
{
    waitFences[0]->reset();
    graphicsQueue->submit(cmdImageCopy, 0, nullptr, nullptr, waitFences[0]);
    waitFences[0]->wait();
    graphicsQueue->onIdle();
}

void VulkanApp::submitCopyBufferCommands()
{
    waitFences[1]->reset();
    transferQueue->submit(cmdBufferCopy, 0, nullptr, nullptr, waitFences[1]);
    waitFences[1]->wait();
    transferQueue->onIdle();
}

#pragma once
#ifdef VK_USE_PLATFORM_WIN32_KHR
#include "winApp.h"
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#include "xlibApp.h"
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include "xcbApp.h"
#endif // VK_USE_PLATFORM_XCB_KHR
#include "magma/magma.h"
#include "rapid/rapid.h"
#include "graphicsPipeline.h"
#include "timer.h"

#ifdef VK_USE_PLATFORM_WIN32_KHR
typedef Win32App NativeApp;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
typedef XlibApp NativeApp;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
typedef XcbApp NativeApp;
#endif // VK_USE_PLATFORM_XCB_KHR

class VulkanApp : public NativeApp
{
protected:
    enum {
        FrontBuffer = 0,
        BackBuffer
    };

public:
    VulkanApp(const AppEntry& entry, const std::tstring& caption, uint32_t width, uint32_t height,
        bool depthBuffer = false);
    ~VulkanApp();
    virtual void render(uint32_t bufferIndex) = 0;
    virtual void onIdle() override;
    virtual void onPaint() override;

protected:
    virtual void initialize();
    virtual void createInstance();
    virtual void createLogicalDevice();
    virtual void createSwapchain(bool vSync);
    virtual void createRenderPass();
    virtual void createFramebuffer();
    virtual void createCommandBuffers();
    virtual void createSyncPrimitives();

    bool submitCommandBuffer(uint32_t bufferIndex);

    std::shared_ptr<magma::Instance> instance;
    std::shared_ptr<magma::DebugReportCallback> debugReportCallback;
    std::shared_ptr<magma::Surface> surface;
    std::shared_ptr<magma::PhysicalDevice> physicalDevice;
    std::shared_ptr<magma::Device> device;
    std::shared_ptr<magma::Swapchain> swapchain;
    std::unique_ptr<magma::InstanceExtensions> instanceExtensions;
    std::unique_ptr<magma::PhysicalDeviceExtensions> extensions;

    std::shared_ptr<magma::CommandPool> commandPools[2];
    std::vector<std::shared_ptr<magma::CommandBuffer>> commandBuffers;
    std::shared_ptr<magma::CommandBuffer> cmdImageCopy;
    std::shared_ptr<magma::CommandBuffer> cmdBufferCopy;

    std::shared_ptr<magma::DepthStencilAttachment> depthStencil;
    std::shared_ptr<magma::ImageView> depthStencilView;
    std::shared_ptr<magma::RenderPass> renderPass;
    std::vector<std::shared_ptr<magma::Framebuffer>> framebuffers;
    std::shared_ptr<magma::Queue> queue;
    std::shared_ptr<magma::Semaphore> presentFinished;
    std::shared_ptr<magma::Semaphore> renderFinished;
    std::vector<std::shared_ptr<magma::Fence>> waitFences;

    std::shared_ptr<magma::PipelineCache> pipelineCache;

    std::unique_ptr<Timer> timer;
    bool depthBuffer;
    bool negateViewport;
};

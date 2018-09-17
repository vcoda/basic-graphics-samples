#include "../framework/vulkanApp.h"

class ClearApp : public VulkanApp
{
public:
    ClearApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("01 - Clear window"), 512, 512)
    {   // Initialize basic framework
        initialize();
        int i = 0;
        // Prepare draw command buffers
        for (auto& cmdBuffer : commandBuffers)
        {
            cmdBuffer->begin();
            {
                cmdBuffer->setRenderArea(0, 0, width, height); // Render area defines clear rectangle
                cmdBuffer->beginRenderPass(renderPass, framebuffers[i],
                    magma::ClearColor(0.35f, 0.53f, 0.7f, 1.0f)); // Set our clear color
                cmdBuffer->endRenderPass();
            }
            cmdBuffer->end();
            ++i;
        }
    }

    virtual void createLogicalDevice() override
    {   // Default device without enabled features is enough for us
        device = physicalDevice->createDefaultDevice();
    }

    virtual void render(uint32_t bufferIndex) override
    {   // Submit commant buffer for execution
        queue->submit(
            commandBuffers[bufferIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
    }

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
    char padding[16];
#endif
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::make_unique<ClearApp>(entry);
}

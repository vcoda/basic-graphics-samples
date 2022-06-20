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
        for (std::shared_ptr<magma::CommandBuffer> cmdBuffer : commandBuffers)
        {
            cmdBuffer->begin();
            {
                constexpr magma::ClearColor clearColor(0.35f, 0.53f, 0.7f, 1.f);
                cmdBuffer->beginRenderPass(renderPass, framebuffers[i],
                    {clearColor}); // Set our clear color
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
        graphicsQueue->submit(
            commandBuffers[bufferIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<ClearApp>(new ClearApp(entry));
}

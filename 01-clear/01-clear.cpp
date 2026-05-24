#include "../framework/vulkanApp.h"

class ClearApp : public VulkanApp
{
public:
    ClearApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("01 - Clear framebuffer"), 512, 512)
    {   // Initialize basic framework
        initialize();
        // Record command buffers
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
    }

    void createLogicalDevice() override
    {   // Default device without enabled features is enough for us
        device = physicalDevice->createDefaultDevice();
    }

    void render(uint32_t bufferIndex) override
    {   // Submit commant buffer for execution
        submitCommandBuffer(bufferIndex);
    }

    void onResize(uint32_t width, uint32_t height) override
    {
        VulkanApp::onResize(width, height);
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {   // Set our clear color
                    magma::ClearColor(0.35f, 0.53f, 0.7f, 1.f)
                });
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<ClearApp>(new ClearApp(entry));
}

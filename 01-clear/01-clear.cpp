#include "../framework/vulkanApp.h"

class ClearApp : public VulkanApp
{
public:
    ClearApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("01 - Clear framebuffer"), 512, 512)
    {   // Initialize basic framework
        initialize();
        int i = 0;
        for (auto& cmdBuffer: commandBuffers)
        {   // Record command buffer
            cmdBuffer->begin();
            {
                cmdBuffer->beginRenderPass(renderPass, framebuffers[i],
                    {   // Set our clear color
                        magma::ClearColor(0.35f, 0.53f, 0.7f, 1.f)
                    });
                cmdBuffer->endRenderPass();
            }
            cmdBuffer->end();
            ++i;
        }
    }

    void createLogicalDevice() override
    {   // Default device without enabled features is enough for us
        device = physicalDevice->createDefaultDevice();
    }

    void render(uint32_t bufferIndex) override
    {   // Submit commant buffer for execution
        submitCommandBuffer(bufferIndex);
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<ClearApp>(new ClearApp(entry));
}

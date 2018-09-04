#include "../framework/vulkanApp.h"

class SimpleTriangleApp : public VulkanApp
{
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

public:
    SimpleTriangleApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("02 - Simple triangle"), 512, 512)
    {
        initialize();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        submitCmdBuffer(bufferIndex);
    }

    virtual void createLogicalDevice() override
    {
        device = physicalDevice->createDefaultDevice();
    }

    void setupPipeline()
    {
        graphicsPipeline.reset(new magma::GraphicsPipeline(device, pipelineCache,
            {
                VertexShader(device, "position.o"),
                FragmentShader(device, "fill.o")
            },
            magma::states::nullVertexInput,
            magma::states::triangleList,
            magma::states::fillCullBackCCW,
            magma::states::dontMultisample,
            magma::states::depthAlwaysDontWrite,
            magma::states::dontBlendWriteRGB,
            {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            nullptr,
            renderPass));
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], magma::clears::grayColor);
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindPipeline(graphicsPipeline);
                cmdBuffer->draw(3, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<IApplication>(new SimpleTriangleApp(entry));
}

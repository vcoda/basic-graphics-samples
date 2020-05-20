#include "../framework/vulkanApp.h"

class TriangleApp : public VulkanApp
{
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

public:
    TriangleApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("02 - Triangle"), 512, 512)
    {
        initialize();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    virtual void createLogicalDevice() override
    {
        device = physicalDevice->createDefaultDevice();
    }

    void setupPipeline()
    {
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "position.o", "fill.o",
            magma::renderstates::nullVertexInput,
            magma::renderstates::triangleList,
            magma::renderstates::fillCullBackCCW,
            magma::renderstates::dontMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendRgb,
            nullptr,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clears::grayColor});
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
    return std::make_unique<TriangleApp>(entry);
}

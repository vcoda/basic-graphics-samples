#include "../framework/vulkanApp.h"

class TriangleApp : public VulkanApp
{
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

public:
    TriangleApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("02 - Triangle"), 512, 512)
    {
        initialize();
        setupPipeline();
        recordCommandBuffer(Buffer::Front);
        recordCommandBuffer(Buffer::Back);
    }

    void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    void createLogicalDevice() override
    {
        device = physicalDevice->createDefaultDevice();
    }

    void setupPipeline()
    {
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "position", "fill",
            magma::renderstate::nullVertexInput,
            magma::renderstate::triangleList,
            magma::renderstate::fillCullBackCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            nullptr,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clear::gray});
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
    return std::unique_ptr<TriangleApp>(new TriangleApp(entry));
}

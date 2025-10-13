#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

class VertexBufferApp : public VulkanApp
{
    std::unique_ptr<magma::VertexBuffer> vertexBuffer;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

public:
    VertexBufferApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("03 - Vertex buffer"), 512, 512)
    {
        initialize();
        createVertexBuffer();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
    }

    void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    void createVertexBuffer()
    {
        // Take into account that unlike OpenGL, Vulkan Y axis points down the screen
        const magma::vt::Pos2fColor4ub vertices[] = {
            {{ 0.0f,-0.3f}, {255, 0, 0, 255}}, // top
            {{-0.6f, 0.3f}, {0, 255, 0, 255}}, // left
            {{ 0.6f, 0.3f}, {0, 0, 255, 255}}  // right
        };
        vertexBuffer = utilities::makeVertexBuffer(vertices, cmdBufferCopy);
    }

    void setupPipeline()
    {
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "passthrough", "fill",
            magma::renderstate::pos2fColor4ub,
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
                cmdBuffer->bindVertexBuffer(0, vertexBuffer);
                cmdBuffer->draw(3, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<VertexBufferApp>(new VertexBufferApp(entry));
}

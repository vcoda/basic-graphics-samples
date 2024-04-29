#include "../framework/vulkanApp.h"
#include "../framework/bufferFromArray.h"

class VertexBufferApp : public VulkanApp
{
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

public:
    VertexBufferApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("03 - Vertex buffer"), 512, 512)
    {
        initialize();
        createVertexBuffer();
        setupPipeline();
        recordCommandBuffer(Buffer::Front);
        recordCommandBuffer(Buffer::Back);
    }

    void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            rapid::float2 pos;
            uint8_t color[4];
        };

        // Take into account that unlike OpenGL, Vulkan Y axis points down the screen
        alignas(MAGMA_ALIGNMENT) Vertex vertices[] = {
            {{ 0.0f,-0.3f}, {255, 0, 0, 255}}, // top
            {{-0.6f, 0.3f}, {0, 255, 0, 255}}, // left
            {{ 0.6f, 0.3f}, {0, 0, 255, 255}}  // right
        };
        vertexBuffer = vertexBufferFromArray<magma::VertexBuffer>(cmdBufferCopy, vertices);
    }

    void setupPipeline()
    {
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "fill.o",
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
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
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

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
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            rapid::float2 pos;
            unsigned char color[4];
        };

        // Take into account that unlike OpenGL, Vulkan Y axis points down the screen
        constexpr auto _1 = std::numeric_limits<unsigned char>::max();
        constexpr alignas(16) Vertex vertices[] = {
            {{ 0.0f,-0.5f}, {_1, 0, 0, _1}}, // top
            {{-0.5f, 0.5f}, {0, _1, 0, _1}}, // left
            {{ 0.5f, 0.5f}, {0, 0, _1, _1}}  // right
        };
        vertexBuffer = vertexBufferFromArray<magma::VertexBuffer>(cmdBufferCopy, vertices);
    }

    void setupPipeline()
    {
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "fill.o",
            magma::renderstate::pos2fColor4b,
            magma::renderstate::triangleList,
            magma::renderstate::fillCullBackCCW,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clears::grayColor});
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

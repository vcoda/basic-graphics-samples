#include "../framework/vulkanApp.h"

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

    virtual void createLogicalDevice() override
    {
        device = physicalDevice->createDefaultDevice();
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
        const std::vector<Vertex> vertices = {
            {{ 0.0f,-0.5f}, {_1, 0, 0, _1}}, // top
            {{-0.5f, 0.5f}, {0, _1, 0, _1}}, // left
            {{ 0.5f, 0.5f}, {0, 0, _1, _1}}  // right
        };
        vertexBuffer = std::make_shared<magma::VertexBuffer>(device, vertices);
    }

    void setupPipeline()
    {
        graphicsPipeline = std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::PipelineShaderStage>{
                VertexShaderFile(device, "passthrough.o"),
                FragmentShaderFile(device, "fill.o")},
            magma::renderstates::pos2fColor4b,
            magma::renderstates::triangleList,
            magma::renderstates::fillCullBackCCW,
            magma::renderstates::noMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendWriteRgb,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            nullptr,
            renderPass);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
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
    return std::make_unique<VertexBufferApp>(entry);
}

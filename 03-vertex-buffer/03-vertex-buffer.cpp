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
        queue->submit(
            commandBuffers[bufferIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
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
        constexpr unsigned char _1 = std::numeric_limits<unsigned char>::max();
        const std::vector<Vertex> vertices = {
            {   // top
                {0.0f, -0.5f},
                {_1, 0, 0, _1}
            },
            {   // left 
                {-0.5f, 0.5f},
                {0, 0, _1, _1}
            },
            {   // right
                {0.5f, 0.5f},
                {0, _1, 0, _1}
            }
        };
        vertexBuffer.reset(new magma::VertexBuffer(device, vertices));
    }

    void setupPipeline()
    {
        graphicsPipeline.reset(new magma::GraphicsPipeline(device, pipelineCache,
            utilities::loadShaders(device, "passthrough.o", "fill.o"),
            magma::states::pos2Float_Col4Unorm,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], magma::ColorClear(.5f, .5f, .5f));
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
    return std::unique_ptr<IApplication>(new VertexBufferApp(entry));
}

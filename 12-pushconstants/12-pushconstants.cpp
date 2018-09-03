#include "../framework/vulkanApp.h"

class PushConstantsApp : public VulkanApp
{
    struct PushConstants
    {
        rapid::float4 vertexColors[3];
    } pushConstants;

    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

public:
    PushConstantsApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("12 - Push constants"), 512, 512)
    {
        initialize();
        createVertexBuffer();
        setupDescriptorSet();
        setupPipeline();
        timer->run();
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updateVertexColors();
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[bufferIndex];
        // To show push constants in dynamic, we have to rebuild command buffer each frame
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[bufferIndex], magma::clears::grayColor);
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->pushConstantBlock(pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, pushConstants);
                cmdBuffer->bindPipeline(graphicsPipeline);
                cmdBuffer->bindVertexBuffer(0, vertexBuffer);
                cmdBuffer->draw(3, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
        queue->submit(
            commandBuffers[bufferIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
    }

    void updateVertexColors()
    {
        static float angle = 0.f;
        angle += rapid::radians(timer->millisecondsElapsed() * 0.3f);
        float s, c;
        rapid::sincosEst(&s, &c, angle);
        s = s * .5f + .5f;
        c = c * .5f + .5f;
        pushConstants.vertexColors[0] = rapid::float4(s, 0.f, 0.f, 1.f);
        pushConstants.vertexColors[1] = rapid::float4(0.f, c, 0.f, 1.f);
        pushConstants.vertexColors[2] = rapid::float4(0.f, 0.f, 1.f - c, 1.f);
    }

    virtual void createLogicalDevice() override
    {
        device = physicalDevice->createDefaultDevice();
    }

    void createVertexBuffer()
    {
        constexpr unsigned char _1 = std::numeric_limits<unsigned char>::max();
        const std::vector<rapid::float2> vertices = {
            { 0.0f,-0.5f},
            {-0.5f, 0.5f},
            { 0.5f, 0.5f}
        };
        vertexBuffer.reset(new magma::VertexBuffer(device, vertices));
    }

    void setupDescriptorSet()
    {
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        descriptorPool.reset(new magma::DescriptorPool(device, 1, {uniformBufferDesc}));
        descriptorSetLayout.reset(new magma::DescriptorSetLayout(device, {
            magma::bindings::VertexStageBinding(0, uniformBufferDesc)
        }));
        // Specify push constant range
        const magma::pushconstants::VertexConstantRange<PushConstants> pushConstantRange;
        pipelineLayout.reset(new magma::PipelineLayout(descriptorSetLayout, {pushConstantRange}));
    }

    void setupPipeline()
    {
        graphicsPipeline.reset(new magma::GraphicsPipeline(device, pipelineCache,
            {
                VertexShader(device, "passthrough.o"),
                FragmentShader(device, "fill.o")
            },
            magma::states::pos2Float,
            magma::states::triangleList,
            magma::states::fillCullBackCCW,
            magma::states::dontMultisample,
            magma::states::depthAlwaysDontWrite,
            magma::states::dontBlendWriteRGB,
            {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass));
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<IApplication>(new PushConstantsApp(entry));
}

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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[bufferIndex], {magma::clears::grayColor});
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
        submitCommandBuffer(bufferIndex);
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
        constexpr auto _1 = std::numeric_limits<unsigned char>::max();
        const std::vector<rapid::float2> vertices = {
            { 0.0f,-0.5f},
            {-0.5f, 0.5f},
            { 0.5f, 0.5f}
        };
        vertexBuffer = std::make_shared<magma::VertexBuffer>(device, vertices);
    }

    void setupDescriptorSet()
    {
        constexpr magma::Descriptor oneUniformBuffer = magma::descriptors::UniformBuffer(1);
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, 1, oneUniformBuffer);
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            magma::bindings::VertexStageBinding(0, oneUniformBuffer));
        // Specify push constant range
        constexpr magma::pushconstants::VertexConstantRange<PushConstants> pushConstantRange;
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout, pushConstantRange);
    }

    void setupPipeline()
    {
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "fill.o",
            magma::renderstates::pos2f,
            magma::renderstates::triangleList,
            magma::renderstates::fillCullBackCCW,
            magma::renderstates::noMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendWriteRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::make_unique<PushConstantsApp>(entry);
}

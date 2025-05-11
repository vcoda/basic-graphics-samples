#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

class PushConstantsApp : public VulkanApp
{
    struct PushConstants
    {
        rapid::float4 vertexColors[3];
    } pushConstants;

    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer pushConstants = 0;
    } setTable;

    std::unique_ptr<magma::VertexBuffer> vertexBuffer;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

public:
    PushConstantsApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("14 - Push constants"), 512, 512)
    {
        initialize();
        createVertexBuffer();
        setupDescriptorSet();
        setupPipeline();
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        updateVertexColors();
        auto& cmdBuffer = commandBuffers[bufferIndex];
        // To show push constants in dynamic, we have to rebuild command buffer each frame
        cmdBuffer->reset(false);
        cmdBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[bufferIndex], {magma::clear::gray});
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->pushConstantBlock(graphicsPipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT, pushConstants);
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

    void createVertexBuffer()
    {
        rapid::float2 vertices[] = {
            { 0.0f,-0.3f},
            {-0.6f, 0.3f},
            { 0.6f, 0.3f}
        };
        vertexBuffer = utilities::makeVertexBuffer(vertices, cmdBufferCopy);
    }

    void setupDescriptorSet()
    {
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT);
    }

    void setupPipeline()
    {
        // Specify push constant range
        constexpr magma::push::VertexConstantRange<PushConstants> pushConstantRange;
        std::unique_ptr<magma::PipelineLayout> layout = std::make_unique<magma::PipelineLayout>(
            descriptorSet->getLayout(), pushConstantRange);
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "passthrough", "fill",
            magma::renderstate::pos2f,
            magma::renderstate::triangleList,
            magma::renderstate::fillCullBackCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            std::move(layout),
            renderPass, 0,
            pipelineCache);
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<PushConstantsApp>(new PushConstantsApp(entry));
}

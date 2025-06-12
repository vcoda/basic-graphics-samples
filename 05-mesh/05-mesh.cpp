#include "../framework/vulkanApp.h"
#include "quadric/include/teapot.h"

class MeshApp : public VulkanApp
{
    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer worldViewProj = 0;
    } setTable;

    std::unique_ptr<quadric::Teapot> mesh;
    std::unique_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> wireframePipeline;

    rapid::matrix viewProj;

public:
    MeshApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("05 - Mesh"), 512, 512, true)
    {
        initialize();
        setupView();
        createMesh();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 3.f, 8.f);
        const rapid::vector3 center(0.f, 2.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        constexpr float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        constexpr float zn = 1.f, zf = 100.f;
        const rapid::matrix view = rapid::lookAtRH(eye, center, up);
        const rapid::matrix proj = rapid::perspectiveFovRH(fov, aspect, zn, zf);
        viewProj = view * proj;
    }

    void updatePerspectiveTransform()
    {
        constexpr float speed = 0.05f;
        static float angle = 0.f;
        angle += timer->millisecondsElapsed() * speed;
        const rapid::matrix world = rapid::rotationY(rapid::radians(angle));
        magma::map(uniformBuffer,
            [this, &world](auto *worldViewProj)
            {
                *worldViewProj = world * viewProj;
            });
    }

    void createMesh()
    {
        constexpr uint16_t subdivisionDegree = 4;
        mesh = std::make_unique<quadric::Teapot>(subdivisionDegree, cmdBufferCopy);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_unique<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void setupDescriptorSet()
    {
        setTable.worldViewProj = uniformBuffer;
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, 0, shaderReflectionFactory, "transform");
    }

    void setupPipeline()
    {
        std::unique_ptr<magma::PipelineLayout> layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout());
        wireframePipeline = std::make_unique<GraphicsPipeline>(device,
            "transform", "normal",
            mesh->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::lineCullBackCcw
                           : magma::renderstate::lineCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
            magma::renderstate::dontBlendRgb,
            std::move(layout),
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clear::gray,
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(wireframePipeline, 0, descriptorSet);
                cmdBuffer->bindPipeline(wireframePipeline);
                mesh->draw(cmdBuffer);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<MeshApp>(new MeshApp(entry));
}

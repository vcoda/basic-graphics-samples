#include "../framework/vulkanApp.h"
#include "../framework/bezierMesh.h"
#include "teapot.h"

class MeshApp : public VulkanApp
{
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> wireframePipeline;

    std::unique_ptr<BezierPatchMesh> mesh;
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
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        timer->run();
    }

    virtual void render(uint32_t bufferIndex) override
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
        magma::helpers::mapScoped<rapid::matrix>(uniformBuffer, true, [this, &world](auto *worldViewProj)
        {
            *worldViewProj = world * viewProj;
        });
    }

    void createMesh()
    {
        constexpr uint32_t subdivisionDegree = 4;
        mesh = std::make_unique<BezierPatchMesh>(teapotPatches, kTeapotNumPatches, teapotVertices, subdivisionDegree, cmdBufferCopy);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void setupDescriptorSet()
    {   // Specify that we will use single uniform buffer
        constexpr magma::Descriptor oneUniformBuffer = magma::descriptors::UniformBuffer(1);
        // Create descriptor pool with single set
        constexpr uint32_t maxDescriptorSets = 1;
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, maxDescriptorSets, oneUniformBuffer);
        // Setup layout of descriptor set: here we describe that slot 0 in the vertex shader
        // will have uniform buffer binding.
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            magma::bindings::VertexStageBinding(0, oneUniformBuffer));
        // Connect our uniform buffer to binding slot 0
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformBuffer);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        wireframePipeline = std::make_shared<GraphicsPipeline>(device,
            "transform.o", "normal.o",
            mesh->getVertexInput(),
            magma::renderstates::triangleList,
            negateViewport ? magma::renderstates::lineCullBackCW : magma::renderstates::lineCullBackCCW,
            magma::renderstates::dontMultisample,
            magma::renderstates::depthLessOrEqual,
            magma::renderstates::dontBlendRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clears::grayColor,
                    magma::clears::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(wireframePipeline, descriptorSet);
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
    return std::make_unique<MeshApp>(entry);
}

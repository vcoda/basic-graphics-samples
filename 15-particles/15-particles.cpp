#include "../framework/vulkanApp.h"
#include "particlesystem.h"

// Use Space to reset particles + mouse to rotate scene
class ParticlesApp : public VulkanApp
{
    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer viewProj = 0;
    } setTable;

    std::unique_ptr<ParticleSystem> particles;
    std::unique_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

    static constexpr float fov = rapid::radians(60.f);
    rapid::matrix viewProj;

public:
    ParticlesApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("15 - Particles"), 512, 512, true)
    {
        initialize();
        initParticleSystem();
        setupView();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        particles->update(timer->secondsElapsed());
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);
    }

    void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case AppKey::Space:
            particles->reset();
            break;
        }
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    void initParticleSystem()
    {
        particles = std::unique_ptr<ParticleSystem>(new ParticleSystem());
        particles->setResolution(width, negateViewport ? -int32_t(height) : height);
        particles->setFieldOfView(fov);
        particles->setPointSize(1.f/3.f);
        particles->setMaxParticles(200);
        particles->setNumToRelease(10);
        particles->setReleaseInterval(0.05f);
        particles->setLifeCycle(5.0f);
        particles->setPosition(rapid::float3(0.f, 0.f, 0.f));
        particles->setVelocity(rapid::float3(0.f, 0.f, 0.f));
        particles->setGravity(rapid::float3(0.f, -9.8f, 0.f));
        particles->setWind(rapid::float3(0.f, 0.f, 0.f));
        particles->setVelocityScale(20.f);
        particles->setCollisionPlane(rapid::float3(0.f, 1.f, 0.f), rapid::float3(0.f, 0.f, 0.f));
        particles->initialize(device);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 3.f, 30.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
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
        const rapid::matrix world = rapid::rotationY(rapid::radians(spinX/2.f));
        magma::map(uniformBuffer,
            [this, &world](auto *worldViewProj)
            {
                *worldViewProj = world * viewProj;
            });
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_unique<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void setupDescriptorSet()
    {
        setTable.viewProj = uniformBuffer;
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, 0, shaderReflectionFactory, "pointSize");
    }

    void setupPipeline()
    {
        const magma::VertexInputStructure<ParticleSystem::ParticleVertex> vertexInput(0, {
            {0, &ParticleSystem::ParticleVertex::position},
            {1, &ParticleSystem::ParticleVertex::color}});
        constexpr magma::push::VertexFragmentConstantRange<ParticleSystem::Constants> pushConstantRange;
        auto layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout(), pushConstantRange);
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "pointSize", "particle",
            vertexInput,
            magma::renderstate::pointList,
            magma::renderstate::pointCullNoneCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::blendNormalRgb,
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
                    magma::clear::black,
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -int32_t(height) : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
                particles->draw(cmdBuffer, graphicsPipeline);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<ParticlesApp>(new ParticlesApp(entry));
}

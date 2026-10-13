#include "../framework/vulkanApp.h"
#include "particlesystem.h"

// Use geometry stage to generate particle quads instead of large points
//#define USE_GEOMETRY_SHADER
#ifdef USE_GEOMETRY_SHADER
    #define VS_FILE_NAME "transform"
    #define GS_FILE_NAME "billboard"
#else
    #define VS_FILE_NAME "pointSize"
    #define GS_FILE_NAME nullptr
#endif

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

    void createLogicalDevice() override
    {
        std::set<magma::DeviceQueueDescriptor> queueDescriptors;
        queueDescriptors.emplace(physicalDevice.get(), VK_QUEUE_GRAPHICS_BIT, magma::QueuePriorityHighest);
        VkPhysicalDeviceFeatures features = {};
    #ifdef USE_GEOMETRY_SHADER
        features.geometryShader = VK_TRUE;
    #else
        features.fillModeNonSolid = VK_TRUE;
        features.largePoints = VK_TRUE;
    #endif
        magma::NullTerminatedStringArray enabledExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    #ifdef VK_KHR_maintenance1
        if (extensions->KHR_maintenance1)
            enabledExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
        else
    #endif // VK_KHR_maintenance1
        {
        #ifdef VK_AMD_negative_viewport_height
            if (extensions->AMD_negative_viewport_height)
                enabledExtensions.push_back(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
        #endif // VK_AMD_negative_viewport_height
        }
    #ifdef VK_EXT_debug_marker
        if (extensions->EXT_debug_marker)
            enabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    #endif // VK_EXT_debug_marker
        const std::vector<const char*> noLayers;
        device = physicalDevice->createDevice(queueDescriptors, noLayers, enabledExtensions, features);
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

    void onResize(uint32_t width, uint32_t height) override
    {
        VulkanApp::onResize(width, height);
        particles->setResolution(width, negateViewport ? -int32_t(height) : height);
        setupView();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
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
    #ifdef USE_GEOMETRY_SHADER
        particles->setUseGeometryShader(true);
    #endif
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
            setTable, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT,
            nullptr, 0, shaderReflectionFactory, VS_FILE_NAME);
    }

    void setupPipeline()
    {
        static constexpr magma::VertexInputStructure<ParticleSystem::ParticleVertex, 2> vertexInput(
            {
                MAGMA_VERTEX_ATTRIBUTE(ParticleSystem::ParticleVertex, position, 0),
                MAGMA_VERTEX_ATTRIBUTE(ParticleSystem::ParticleVertex, color, 1),
            });
    #ifdef USE_GEOMETRY_SHADER
        constexpr magma::push::GeometryFragmentConstantRange<ParticleSystem::Constants> pushConstantRange;
    #else
        constexpr magma::push::VertexFragmentConstantRange<ParticleSystem::Constants> pushConstantRange;
    #endif
        auto layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout(), pushConstantRange);
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            VS_FILE_NAME, GS_FILE_NAME, "particle",
            vertexInput,
            magma::renderstate::pointList,
        #ifdef USE_GEOMETRY_SHADER
            magma::renderstate::fillCullNoneCcw,
        #else
            magma::renderstate::pointCullNoneCcw,
        #endif
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

#include "../framework/vulkanApp.h"
#include "quadric/include/plane.h"
#include "quadric/include/teapot.h"

#define NON_COHERENT_UNIFORM_BUFFER 0

template<class Type> using DynamicUniformBuffer =
#if NON_COHERENT_UNIFORM_BUFFER
    magma::NonCoherentDynamicUniformBuffer<Type>;
#else
    magma::DynamicUniformBuffer<Type>;
#endif

// Use L button + mouse to rotate scene
class OcclusionQueryApp : public VulkanApp
{
    struct TransformSetTable
    {
        magma::descriptor::DynamicUniformBuffer worldViewProj = 0;
    } setTable0;

    struct ColorSetTable
    {
        magma::descriptor::DynamicUniformBuffer color = 0;
    } setTable1;

    std::unique_ptr<quadric::Plane> plane;
    std::unique_ptr<quadric::Teapot> teapot;
    std::unique_ptr<magma::OcclusionQuery> occlusionQuery;
    std::unique_ptr<DynamicUniformBuffer<rapid::matrix>> transformUniforms;
    std::unique_ptr<DynamicUniformBuffer<rapid::vector4>> colorUniforms;
    std::unique_ptr<magma::DescriptorSet> descriptorSets[2];
    std::shared_ptr<magma::PipelineLayout> sharedLayout;
    std::unique_ptr<magma::GraphicsPipeline> teapotPipeline;
    std::unique_ptr<magma::GraphicsPipeline> planePipeline;

    rapid::matrix viewProj;

public:
    OcclusionQueryApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("11 - Occlusion query"), 512, 512, true)
    {
        initialize();
        setupView();
        createOcclusionQuery();
        createMeshes();
        createUniformBuffers();
        setupDescriptorSet();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
    }

    void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);
        constexpr bool waitForResult = true;
        showOcclusionResult(waitForResult);
    }

    void showOcclusionResult(bool waitForResult)
    {
        uint64_t sampleCount = 0;
        if (waitForResult)
            sampleCount = occlusionQuery->getResults<uint64_t>(0, 1, true).front();
        else
        {
            const magma::QueryPool::Result<uint64_t, uint64_t> result = occlusionQuery->getResultsWithAvailability<uint64_t>(0, 1).front();
            if (result.availability > 0)
                sampleCount = result.result;
            std::cout << "Query result: ";
            if (result.availability > 0)
                std::cout << sampleCount << std::endl;
            else // Not ready
                std::cout << "---" << std::endl;
        }
        const std::tstring caption = TEXT("11 - Occlusion query samples: ") + std::to_tstring(sampleCount);
        setWindowCaption(caption);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 10.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
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
        const rapid::matrix pitch = rapid::rotationX(rapid::radians(spinY/2.f));
        const rapid::matrix yaw = rapid::rotationY(rapid::radians(spinX/2.f));
        const rapid::matrix transPlane = rapid::translation(0.f, 0.f, 2.f);
        const rapid::matrix transMesh = rapid::translation(0.f, -2.f, 0.f);
        const rapid::matrix worldPlane = rapid::rotationX(rapid::radians(90.f)) * transPlane * pitch * yaw;
        const rapid::matrix worldMesh = transMesh * pitch * yaw;
        magma::map<rapid::matrix>(transformUniforms,
            [this, &worldPlane, &worldMesh](auto& transforms)
            {
                transforms[0] = worldPlane * viewProj;
                transforms[1] = worldMesh * viewProj;
            });
    }

    void createOcclusionQuery()
    {
       /* Not setting precise bit may be more efficient on some implementations,
          and should be used where it is sufficient to know a boolean result
          on whether any samples passed the per-fragment tests.
          In this case, some implementations may only return zero or one,
          indifferent to the actual number of samples passing the per-fragment tests. */
        constexpr bool precise = false;
        occlusionQuery = std::make_unique<magma::OcclusionQuery>(device, 1, precise);
    }

    void createMeshes()
    {
        constexpr bool twoSided = true;
        plane = std::make_unique<quadric::Plane>(6.f, 6.f, twoSided, cmdBufferCopy);
        constexpr uint16_t subdivisionDegree = 16;
        teapot = std::make_unique<quadric::Teapot>(subdivisionDegree, cmdBufferCopy);
    }

    void createUniformBuffers()
    {
    #if NON_COHERENT_UNIFORM_BUFFER
        constexpr bool ubFlag = true; // mappedPersistently
    #else
        const bool ubFlag = physicalDevice->features()->supportsDeviceLocalHostVisibleMemory(); // stagedPool
    #endif
        transformUniforms = std::make_unique<DynamicUniformBuffer<rapid::matrix>>(device, 2, ubFlag);
        colorUniforms = std::make_unique<DynamicUniformBuffer<rapid::vector4>>(device, 2, ubFlag);
        magma::map<rapid::vector4>(colorUniforms,
            [](auto& colors)
            {   // Update only once
                colors[0] = rapid::vector4(0.f, 0.f, 1.f, 1.f);
                colors[1] = rapid::vector4(1.f, 0.f, 0.f, 1.f);
            });
        updatePerspectiveTransform();
    }

    void setupDescriptorSet()
    {
        setTable0.worldViewProj = transformUniforms;
        descriptorSets[0] = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable0, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, 0, shaderReflectionFactory, "transform", 0);
        setTable1.color = colorUniforms;
        descriptorSets[1] = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable1, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, 0, shaderReflectionFactory, "transform", 1);
    }

    void setupPipeline()
    {
        sharedLayout = std::make_shared<magma::PipelineLayout>(
            std::initializer_list<magma::lent_ptr<const magma::DescriptorSetLayout>>{
                descriptorSets[0]->getLayout(),
                descriptorSets[1]->getLayout()
            });
        teapotPipeline = std::make_unique<GraphicsPipeline>(device,
            "transform", "fill",
            teapot->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCcw
                           : magma::renderstate::fillCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
            magma::renderstate::dontBlendRgb,
            sharedLayout,
            renderPass, 0,
            pipelineCache);
        planePipeline = std::make_unique<GraphicsPipeline>(device,
            "transform", "fill",
            plane->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCcw
                           : magma::renderstate::fillCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
            magma::renderstate::dontBlendRgb,
            sharedLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->resetQueryPool(occlusionQuery, 0, occlusionQuery->getQueryCount());
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clear::gray,
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                // Occluder
                cmdBuffer->bindPipeline(planePipeline);
                cmdBuffer->bindDescriptorSets(planePipeline, 0, {descriptorSets[0], descriptorSets[1]}, {0, 0});
                plane->draw(cmdBuffer);
                // Occludee
                cmdBuffer->bindPipeline(teapotPipeline);
                cmdBuffer->bindDescriptorSets(teapotPipeline, 0, {descriptorSets[0], descriptorSets[1]},
                    {
                        transformUniforms->getDynamicOffset(1),
                        colorUniforms->getDynamicOffset(1)
                    });
                cmdBuffer->beginQuery(occlusionQuery, 0);
                {
                    teapot->draw(cmdBuffer);
                }
                cmdBuffer->endQuery(occlusionQuery, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<OcclusionQueryApp>(new OcclusionQueryApp(entry));
}

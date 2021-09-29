#include "../framework/vulkanApp.h"
#include "quadric/include/plane.h"
#include "quadric/include/teapot.h"

// Use L button + mouse to rotate scene
class OcclusionQueryApp : public VulkanApp
{
    struct TransformSetLayout : public magma::DescriptorSetDeclaration
    {
        magma::binding::DynamicUniformBuffer worldViewProj = 0;
        MAGMA_REFLECT(&worldViewProj)
    } setLayout0;

    struct ColorSetLayout : public magma::DescriptorSetDeclaration
    {
        magma::binding::DynamicUniformBuffer color = 0;
        MAGMA_REFLECT(&color)
    } setLayout1;

    std::unique_ptr<quadric::Plane> plane;
    std::unique_ptr<quadric::Teapot> teapot;
    std::shared_ptr<magma::QueryPool> occlusionQuery;
    std::shared_ptr<magma::DynamicUniformBuffer<rapid::matrix>> transformUniforms;
    std::shared_ptr<magma::DynamicUniformBuffer<rapid::vector4>> colorUniforms;
    std::shared_ptr<magma::DescriptorSet> descriptorSets[2];
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> teapotPipeline;
    std::shared_ptr<magma::GraphicsPipeline> planePipeline;

    rapid::matrix viewProj;

public:
    OcclusionQueryApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("11 - Occlusion query"), 512, 512, true)
    {
        initialize();
        setupView();
        createOcclusionQuery();
        createMeshes();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);

        // Show occlusion query result
        const std::vector<VkDeviceSize> results = occlusionQuery->getResults(0, 1, true);
        if (!results.empty())
        {
            const std::tstring caption = TEXT("11 - Occlusion query samples passed : ") + std::to_tstring(results[0]);
            setWindowCaption(caption);
        }
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
        magma::helpers::mapScoped<rapid::matrix>(transformUniforms,
            [this, &worldPlane, &worldMesh](magma::helpers::AlignedUniformArray<rapid::matrix>& transforms)
            {
                transforms[0] = worldPlane * viewProj;
                transforms[1] = worldMesh * viewProj;
            });
    }

    void createOcclusionQuery()
    {
        occlusionQuery = std::make_shared<magma::OcclusionQuery>(device, 1);
    }

    void createMeshes()
    {
        constexpr bool twoSided = true;
        plane = std::make_unique<quadric::Plane>(6.f, 6.f, twoSided, cmdBufferCopy);
        constexpr uint16_t subdivisionDegree = 16;
        teapot = std::make_unique<quadric::Teapot>(subdivisionDegree, cmdBufferCopy);
    }

    void createUniformBuffer()
    {
        transformUniforms = std::make_shared<magma::DynamicUniformBuffer<rapid::matrix>>(device, 2);
        updatePerspectiveTransform();
        colorUniforms = std::make_shared<magma::DynamicUniformBuffer<rapid::vector4>>(device, 2);
        magma::helpers::mapScoped<rapid::vector4>(colorUniforms,
            [](magma::helpers::AlignedUniformArray<rapid::vector4>& colors)
        {   // Update only once
            colors[0] = rapid::vector4(0.f, 0.f, 1.f, 1.f);
            colors[1] = rapid::vector4(1.f, 0.f, 0.f, 1.f);
        });
    }

    void setupDescriptorSet()
    {
        setLayout0.worldViewProj = transformUniforms;
        descriptorSets[0] = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setLayout0, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, shaderReflectionFactory, "transform.o", 0);
        setLayout1.color = colorUniforms;
        descriptorSets[1] = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setLayout1, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, shaderReflectionFactory, "transform.o", 1);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(
            std::vector<std::shared_ptr<magma::DescriptorSetLayout>>{
                descriptorSets[0]->getLayout(),
                descriptorSets[1]->getLayout()
            });
        teapotPipeline = std::make_shared<GraphicsPipeline>(device,
            "transform.o", "fill.o",
            teapot->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCCW : magma::renderstate::fillCullBackCW,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
            magma::renderstate::dontBlendRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
        planePipeline = std::make_shared<GraphicsPipeline>(device,
            "transform.o", "fill.o",
            plane->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCCW : magma::renderstate::fillCullBackCW,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
            magma::renderstate::dontBlendRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->resetQueryPool(occlusionQuery);
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
                /* Not setting precise bit may be more efficient on some implementations,
                   and should be used where it is sufficient to know a boolean result
                   on whether any samples passed the per-fragment tests.
                   In this case, some implementations may only return zero or one,
                   indifferent to the actual number of samples passing the per-fragment tests. */
                constexpr bool precise = false;
                cmdBuffer->beginQuery(occlusionQuery, 0, precise);
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

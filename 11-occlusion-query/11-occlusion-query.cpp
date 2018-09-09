#include "../framework/vulkanApp.h"
#include "../framework/bezierMesh.h"
#include "../framework/planeMesh.h"
#include "teapot.h"

// Use L button + mouse to rotate scene
class OcclusionQueryApp : public VulkanApp
{
    std::unique_ptr<BezierPatchMesh> teapot;
    std::unique_ptr<PlaneMesh> plane;
    std::shared_ptr<magma::QueryPool> occlusionQuery;
    std::shared_ptr<magma::DynamicUniformBuffer<rapid::matrix>> transformUniforms;
    std::shared_ptr<magma::DynamicUniformBuffer<rapid::vector4>> colorUniforms;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayouts[2];
    std::shared_ptr<magma::DescriptorSet> descriptorSets[2];
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix viewProj;
    bool negateViewport = false;

public:
    OcclusionQueryApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("11 - Occlusion query"), 512, 512, true)
    {
        initialize();
        negateViewport = extensions->KHR_maintenance1 || extensions->AMD_negative_viewport_height;
        setupView();
        createOcclusionQuery();
        createMesh();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCmdBuffer(bufferIndex);

        // Show occlusion query result
        const std::vector<VkDeviceSize> results = occlusionQuery->getResults(0, 1, true);
        if (!results.empty())
        {
            const std::tstring caption = TEXT("11 - Occlusion query samples passed : ") + std::to_tstring(results[0]);
            static int i = 0;
            if (i++ % 100 == 0) // Update periodically, every frame may be slow
                setWindowCaption(caption);
        }
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 10.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        const float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        const float zn = 1.f, zf = 100.f;
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
        const rapid::matrix worldPlane = transPlane * pitch * yaw;
        const rapid::matrix worldMesh = transMesh * pitch * yaw;
        magma::helpers::mapScoped<rapid::matrix>(transformUniforms, true,
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

    void createMesh()
    {
        const uint32_t subdivisionDegree = 16;
        teapot = std::make_unique<BezierPatchMesh>(teapotPatches, kTeapotNumPatches, teapotVertices, subdivisionDegree, cmdBufferCopy);
        plane = std::make_unique<PlaneMesh>(6.f, 6.f, true, true, cmdBufferCopy);
    }

    void createUniformBuffer()
    {
        transformUniforms = std::make_shared<magma::DynamicUniformBuffer<rapid::matrix>>(device, 2);
        updatePerspectiveTransform();
        colorUniforms = std::make_shared<magma::DynamicUniformBuffer<rapid::vector4>>(device, 2);
        // Update only once
        magma::helpers::mapScoped<rapid::vector4>(colorUniforms, false,
            [](magma::helpers::AlignedUniformArray<rapid::vector4>& colors)
        {
            colors[0] = rapid::vector4(0.f, 0.f, 1.f, 1.f);
            colors[1] = rapid::vector4(1.f, 0.f, 0.f, 1.f);
        });
    }

    void setupDescriptorSet()
    {
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, 2,
            std::vector<magma::Descriptor>
            {
                magma::descriptors::DynamicUniformBuffer(2),
            });
        const magma::Descriptor uniformBufferDesc = magma::descriptors::DynamicUniformBuffer(1);
        // Setup first set layout
        descriptorSetLayouts[0] = std::make_shared<magma::DescriptorSetLayout>(device,
            magma::bindings::VertexStageBinding(0, uniformBufferDesc));
        descriptorSets[0] = descriptorPool->allocateDescriptorSet(descriptorSetLayouts[0]);
        descriptorSets[0]->update(0, transformUniforms);
        // Setup second set layout
        descriptorSetLayouts[1] = std::make_shared<magma::DescriptorSetLayout>(device,
            magma::bindings::VertexStageBinding(1, uniformBufferDesc));
        descriptorSets[1] = descriptorPool->allocateDescriptorSet(descriptorSetLayouts[1]);
        descriptorSets[1]->update(0, colorUniforms);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayouts);
        graphicsPipeline = std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::ShaderStage>
            {
                VertexShader(device, "transform.o"),
                FragmentShader(device, "fill.o")
            },
            teapot->getVertexInput(),
            magma::states::triangleList,
            negateViewport ? magma::states::fillCullBackCW : magma::states::fillCullBackCCW,
            magma::states::noMultisample,
            magma::states::depthLessOrEqual,
            magma::states::dontBlendWriteRGB,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->resetQueryPool(occlusionQuery);
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clears::grayColor,
                    magma::clears::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindPipeline(graphicsPipeline);
                // Occluder
                cmdBuffer->bindDescriptorSets(pipelineLayout, descriptorSets, {0, 0});
                plane->draw(cmdBuffer);
                // Occludee
                cmdBuffer->bindDescriptorSets(pipelineLayout, descriptorSets,
                    {
                        transformUniforms->getDynamicOffset(1),
                        colorUniforms->getDynamicOffset(1)
                    });
                /* Not setting precise bit may be more efficient on some implementations,
                   and should be used where it is sufficient to know a boolean result
                   on whether any samples passed the per-fragment tests.
                   In this case, some implementations may only return zero or one,
                   indifferent to the actual number of samples passing the per-fragment tests. */
                const bool precise = false;
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
    return std::make_unique<OcclusionQueryApp>(entry);
}

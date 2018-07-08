#include "../framework/vulkanApp.h"

class RenderToTextureApp : public VulkanApp
{
    struct Framebuffer
    {
        std::shared_ptr<magma::ColorAttachment2D> color;
        std::shared_ptr<magma::ImageView> colorView;
        std::shared_ptr<magma::ColorAttachment2D> msaaColor;
        std::shared_ptr<magma::ImageView> msaaColorView;
        std::shared_ptr<magma::RenderPass> renderPass;
        std::shared_ptr<magma::Framebuffer> framebuffer;
    } fb;

    std::shared_ptr<magma::CommandBuffer> rtCmdBuffer;
    std::shared_ptr<magma::Semaphore> rtSemaphore;
    std::shared_ptr<magma::GraphicsPipeline> rtPipeline;
    std::vector<magma::ShaderStage> rtShaderStages;

    std::vector<magma::ShaderStage> shaderStages;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::shared_ptr<magma::Sampler> nearestSampler;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    constexpr static uint32_t fbSize = 32;
    uint32_t msaaSamples = 4;

public:
    RenderToTextureApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("10 - Render to texture"), 512, 512)
    {
        initialize();
        createFramebuffer({fbSize, fbSize});
        createUniformBuffer();
        setupDescriptorSet();
        setupPipelines();
        recordRTCommandBuffer();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        timer->run();
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updateWorldTransform();
        queue->submit(
            rtCmdBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished, // Wait for swapchain
            rtSemaphore,
            nullptr);

        queue->submit(
            commandBuffers[bufferIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            rtSemaphore, // Wait for render-to-texture
            renderFinished,
            waitFences[bufferIndex]);
    }

    void updateWorldTransform()
    {
        static float angle = 0.f;
        const float speed = 0.03f;
        const float step = timer->millisecondsElapsed() * speed;
        angle += step;
        const rapid::matrix roll = rapid::rotationZ(rapid::radians(angle));
        magma::helpers::mapScoped<rapid::matrix>(uniformBuffer, true, [&roll](auto *world)
        {
            *world = roll;
        });
    }

    uint32_t maxSampleCount(VkSampleCountFlags sampleCounts) const
    {
        const std::vector<VkSampleCountFlagBits> sampleCountBits = {
            VK_SAMPLE_COUNT_1_BIT,
            VK_SAMPLE_COUNT_2_BIT,
            VK_SAMPLE_COUNT_4_BIT,
            VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_16_BIT,
            VK_SAMPLE_COUNT_32_BIT,
            VK_SAMPLE_COUNT_64_BIT
        };
        for (auto it = sampleCountBits.rbegin(); it < sampleCountBits.rend(); ++it)
        {
            const uint32_t countBit = *it;
            if (sampleCounts & countBit)
                return countBit;
        }
        return 1;
    }

    void createFramebuffer(const VkExtent2D& extent)
    {
        fb.color.reset(new magma::ColorAttachment2D(device, VK_FORMAT_R8G8B8A8_UNORM, extent, 1, 1));
        fb.colorView.reset(new magma::ImageView(fb.color));

        // Make sure that we are fit to hardware limits
        const VkImageFormatProperties formatProperties = physicalDevice->getImageFormatProperties(
            fb.color->getFormat(), VK_IMAGE_TYPE_2D, true, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        const uint32_t maxFormatSamples = maxSampleCount(formatProperties.sampleCounts);
        if (msaaSamples > maxFormatSamples)
            msaaSamples = maxFormatSamples;
        if (msaaSamples > 1)
        {
            fb.msaaColor.reset(new magma::ColorAttachment2D(device, fb.color->getFormat(), extent, 1, msaaSamples));
            fb.msaaColorView.reset(new magma::ImageView(fb.msaaColor));
            const magma::AttachmentDescription multisampleColorAttachment(fb.msaaColor->getFormat(), fb.msaaColor->getSamples(),
                magma::attachments::colorClearStoreOptimal);
            const magma::AttachmentDescription resolveColorAttachment(fb.color->getFormat(), 1,
                magma::attachments::colorDontCareStoreOptimal); // Don't care about clear as it will be used as MSAA resolve target
            fb.renderPass.reset(new magma::RenderPass(device, {multisampleColorAttachment, resolveColorAttachment}));
            fb.framebuffer.reset(new magma::Framebuffer(fb.renderPass, {fb.msaaColorView, fb.colorView}));
        }
        else
        {
            const magma::AttachmentDescription colorAttachment(fb.color->getFormat(), 1,
                magma::attachments::colorClearStoreReadOnly);
            fb.renderPass.reset(new magma::RenderPass(device, colorAttachment));
            fb.framebuffer.reset(new magma::Framebuffer(fb.renderPass, fb.colorView));
        }
    }

    void createUniformBuffer()
    {
        uniformBuffer.reset(new magma::UniformBuffer<rapid::matrix>(device));
    }

    void setupDescriptorSet()
    {
        const uint32_t maxDescriptorSets = 1;
        descriptorPool.reset(new magma::DescriptorPool(device, maxDescriptorSets, {
            magma::descriptors::UniformBuffer(1),
            magma::descriptors::CombinedImageSampler(1)
        }));
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        const magma::Descriptor imageSamplerDesc = magma::descriptors::CombinedImageSampler(1);
        descriptorSetLayout.reset(new magma::DescriptorSetLayout(device, {
            magma::bindings::VertexStageBinding(0, uniformBufferDesc),
            magma::bindings::FragmentStageBinding(1, imageSamplerDesc)
        }));
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        nearestSampler.reset(new magma::Sampler(device, magma::samplers::nearestMipmapNearestClampToEdge));
        descriptorSet->update(0, uniformBuffer);
        descriptorSet->update(1, fb.colorView, nearestSampler);
    }

    void setupPipelines()
    {
        pipelineLayout.reset(new magma::PipelineLayout(descriptorSetLayout));
        rtPipeline.reset(new magma::GraphicsPipeline(device, pipelineCache,
            {
                VertexShader(device, "triangle.o"),
                FragmentShader(device, "fill.o")
            },
            magma::states::nullVertexInput,
            magma::states::triangleList,
            magma::states::fillCullBackCCW,
            magma::MultisampleState(static_cast<VkSampleCountFlagBits>(msaaSamples)),
            magma::states::depthAlwaysDontWrite,
            magma::states::dontBlendWriteRGB,
            {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            fb.renderPass));
        graphicsPipeline.reset(new magma::GraphicsPipeline(device, pipelineCache,
            {
                VertexShader(device, "quad.o"),
                FragmentShader(device, "texture.o")
            },
            magma::states::nullVertexInput,
            magma::states::triangleStrip,
            magma::states::fillCullBackCCW,
            magma::states::dontMultisample,
            magma::states::depthAlwaysDontWrite,
            magma::states::blendNormalWriteRGB,
            {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass));
    }

    void recordRTCommandBuffer()
    {
        rtCmdBuffer = commandPools[0]->allocateCommandBuffer(true);
        rtSemaphore.reset(new magma::Semaphore(device));

        rtCmdBuffer->begin();
        {
            rtCmdBuffer->setRenderArea(0, 0, fb.framebuffer->getExtent());
            rtCmdBuffer->beginRenderPass(fb.renderPass, fb.framebuffer, magma::clears::blackColor);
            {
                rtCmdBuffer->setViewport(magma::Viewport(0, 0, fb.framebuffer->getExtent()));
                rtCmdBuffer->setScissor(magma::Scissor(0, 0, fb.framebuffer->getExtent()));
                rtCmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet);
                rtCmdBuffer->bindPipeline(rtPipeline);
                rtCmdBuffer->draw(3, 0);
            }
            rtCmdBuffer->endRenderPass();
        }
        rtCmdBuffer->end();
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], magma::clears::grayColor);
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet);
                cmdBuffer->bindPipeline(graphicsPipeline);
                cmdBuffer->draw(4, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<IApplication>(new RenderToTextureApp(entry));
}

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
        constexpr float speed = 0.03f;
        static float angle = 0.f;
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
        fb.color = std::make_shared<magma::ColorAttachment2D>(device, VK_FORMAT_R8G8B8A8_UNORM, extent, 1, 1);
        fb.colorView = std::make_shared<magma::ImageView>(fb.color);

        // Make sure that we are fit to hardware limits
        const VkImageFormatProperties formatProperties = physicalDevice->getImageFormatProperties(
            fb.color->getFormat(), VK_IMAGE_TYPE_2D, true, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        const uint32_t maxFormatSamples = maxSampleCount(formatProperties.sampleCounts);
        if (msaaSamples > maxFormatSamples)
            msaaSamples = maxFormatSamples;
        if (msaaSamples > 1)
        {
            fb.msaaColor = std::make_shared<magma::ColorAttachment2D>(device, fb.color->getFormat(), extent, 1, msaaSamples);
            fb.msaaColorView = std::make_shared<magma::ImageView>(fb.msaaColor);
            const magma::AttachmentDescription multisampleColorAttachment(fb.msaaColor->getFormat(), fb.msaaColor->getSamples(),
                magma::attachments::colorClearStoreOptimal);
            const magma::AttachmentDescription resolveColorAttachment(fb.color->getFormat(), 1,
                magma::attachments::colorDontCareStoreOptimal); // Don't care about clear as it will be used as MSAA resolve target
            fb.renderPass = std::make_shared<magma::RenderPass>(device,
                std::initializer_list<magma::AttachmentDescription>
                {
                    multisampleColorAttachment,
                    resolveColorAttachment
                });
            fb.framebuffer = std::make_shared<magma::Framebuffer>(fb.renderPass,
                std::vector<std::shared_ptr<const magma::ImageView>>
                {
                    fb.msaaColorView,
                    fb.colorView
                });
        }
        else
        {
            const magma::AttachmentDescription colorAttachment(fb.color->getFormat(), 1,
                magma::attachments::colorClearStoreReadOnly);
            fb.renderPass = std::make_shared<magma::RenderPass>(device, colorAttachment);
            fb.framebuffer = std::make_shared<magma::Framebuffer>(fb.renderPass, fb.colorView);
        }
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void setupDescriptorSet()
    {
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        const magma::Descriptor imageSamplerDesc = magma::descriptors::CombinedImageSampler(1);
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, 1,
            std::vector<magma::Descriptor>
            {
                uniformBufferDesc,
                imageSamplerDesc
            });
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            std::initializer_list<magma::DescriptorSetLayout::Binding>
            {
                magma::bindings::VertexStageBinding(0, uniformBufferDesc),
                magma::bindings::FragmentStageBinding(1, imageSamplerDesc)
            });
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        nearestSampler = std::make_shared<magma::Sampler>(device, magma::samplers::nearestMipmapNearestClampToEdge);
        descriptorSet->update(0, uniformBuffer);
        descriptorSet->update(1, fb.colorView, nearestSampler);
    }

    void setupPipelines()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        rtPipeline = std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::PipelineShaderStage>
            {
                VertexShader(device, "triangle.o"),
                FragmentShader(device, "fill.o")
            },
            magma::renderstates::nullVertexInput,
            magma::renderstates::triangleList,
            magma::renderstates::fillCullBackCCW,
            magma::MultisampleState(static_cast<VkSampleCountFlagBits>(msaaSamples)),
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendWriteRGB,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            fb.renderPass);
        graphicsPipeline = std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::PipelineShaderStage>
            {
                VertexShader(device, "quad.o"),
                FragmentShader(device, "texture.o")
            },
            magma::renderstates::nullVertexInput,
            magma::renderstates::triangleStrip,
            magma::renderstates::fillCullBackCCW,
            magma::renderstates::noMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::blendNormalWriteRGB,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass);
    }

    void recordRTCommandBuffer()
    {
        rtCmdBuffer = commandPools[0]->allocateCommandBuffer(true);
        rtSemaphore = std::make_shared<magma::Semaphore>(device);

        rtCmdBuffer->begin();
        {
            rtCmdBuffer->setRenderArea(0, 0, fb.framebuffer->getExtent());
            rtCmdBuffer->beginRenderPass(fb.renderPass, fb.framebuffer, {magma::clears::blackColor});
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
        std::shared_ptr<magma::CommandBuffer>& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clears::grayColor});
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
    return std::make_unique<RenderToTextureApp>(entry);
}

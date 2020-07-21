#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

class RenderToTextureApp : public VulkanApp
{
    constexpr static uint32_t fbSize = 128;

    struct Framebuffer
    {
        std::shared_ptr<magma::ColorAttachment> color;
        std::shared_ptr<magma::ImageView> colorView;
        std::shared_ptr<magma::DepthStencilAttachment> depth;
        std::shared_ptr<magma::ImageView> depthView;
        std::shared_ptr<magma::RenderPass> renderPass;
        std::shared_ptr<magma::Framebuffer> framebuffer;
    } fb;

    std::shared_ptr<magma::CommandBuffer> rtCmdBuffer;
    std::shared_ptr<magma::Semaphore> rtSemaphore;
    std::shared_ptr<magma::GraphicsPipeline> rtPipeline;

    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::shared_ptr<magma::Sampler> nearestSampler;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> texPipeline;

public:
    RenderToTextureApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("10.a - Render to texture"), 512, 512)
    {
        initialize();
        createFramebuffer({fbSize, fbSize});
        createVertexBuffer();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipelines();
        recordOffscreenCommandBuffer(fb);
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        timer->run();
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updateWorldTransform();
        constexpr VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        queue->submit(rtCmdBuffer, stageMask,
            presentFinished, // Wait for swapchain
            rtSemaphore,
            nullptr);
        queue->submit(commandBuffers[bufferIndex], stageMask,
            rtSemaphore, // Wait for render-to-texture
            renderFinished,
            waitFences[bufferIndex]);
    }

    void updateWorldTransform()
    {
        constexpr float speed = 0.02f;
        static float angle = 0.f;
        const float step = timer->millisecondsElapsed() * speed;
        angle += step;
        const rapid::matrix roll = rapid::rotationZ(rapid::radians(angle));
        magma::helpers::mapScoped<rapid::matrix>(uniformBuffer,
            [&roll](auto *world)
            {
                *world = roll;
            });
    }

    void createFramebuffer(const VkExtent2D& extent)
    {
        // Create color attachment
        fb.color = std::make_shared<magma::ColorAttachment>(device, VK_FORMAT_R8G8B8A8_UNORM, extent, 1, 1);
        fb.colorView = std::make_shared<magma::ImageView>(fb.color);
        // Create depth attachment
        const VkFormat depthFormat = utilities::getSupportedDepthFormat(physicalDevice, false, true);
        fb.depth = std::make_shared<magma::DepthStencilAttachment>(device, depthFormat, extent, 1, 1);
        fb.depthView = std::make_shared<magma::ImageView>(fb.depth);
        // Don't care about initial layout
        constexpr VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // Define that color attachment can be cleared, can store shader output and should be read-only image
        const magma::AttachmentDescription colorAttachment(fb.color->getFormat(), 1,
            magma::op::clearStore, // Color clear, store
            magma::op::dontCare, // Inapplicable
            initialLayout,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); // Should be read-only in the shader when a render pass instance ends
        // Define that depth attachment can be cleared and can store shader output
        const magma::AttachmentDescription depthAttachment(fb.depth->getFormat(), 1,
            magma::op::clearStore, // Depth clear, store
            magma::op::dontCare, // Don't care about stencil
            initialLayout,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL); // Stay as depth/stencil attachment
        // Render pass defines attachment formats, load/store operations and final layouts
        fb.renderPass = std::shared_ptr<magma::RenderPass>(new magma::RenderPass(
            device, {colorAttachment, depthAttachment}));
        // Framebuffer defines render pass, color/depth/stencil image views and dimensions
        fb.framebuffer = std::shared_ptr<magma::Framebuffer>(new magma::Framebuffer(
            fb.renderPass, {fb.colorView, fb.depthView}));
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            rapid::float2 position;
            rapid::float2 uv;
        };

        constexpr float hw = 0.75f;
        constexpr float hh = 0.75f;
        const Vertex vertices[] = {
            {   // top left
                {-hw, -hh},
                {0.f, 0.f},
            },
            {   // bottom left
                {-hw, hh},
                {0.f, 1.f},
            },
            {   // top right
                {hw, -hh},
                {1.f, 0.f},
            },
            {   // bottom right
                {hw, hh},
                {1.f, 1.f},
            }
        };
        vertexBuffer = std::make_shared<magma::VertexBuffer>(cmdBufferCopy, vertices);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void setupDescriptorSet()
    {
        constexpr magma::Descriptor oneUniformBuffer = magma::descriptors::UniformBuffer(1);
        constexpr magma::Descriptor oneImageSampler = magma::descriptors::CombinedImageSampler(1);
        descriptorPool = std::shared_ptr<magma::DescriptorPool>(new magma::DescriptorPool(device, 1,
            {
                oneUniformBuffer,
                oneImageSampler
            }));
        descriptorSetLayout = std::shared_ptr<magma::DescriptorSetLayout>(new magma::DescriptorSetLayout(device,
            {
                magma::bindings::VertexStageBinding(0, oneUniformBuffer),
                magma::bindings::FragmentStageBinding(1, oneImageSampler)
            }));
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        nearestSampler = std::make_shared<magma::Sampler>(device, magma::samplers::magMinMipNearestClampToEdge);
        descriptorSet->update(0, uniformBuffer);
        descriptorSet->update(1, fb.colorView, nearestSampler);
    }

    void setupPipelines()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        rtPipeline = std::make_shared<GraphicsPipeline>(device,
            "triangle.o", "fill.o",
            magma::renderstates::nullVertexInput,
            magma::renderstates::triangleList,
            magma::renderstates::fillCullBackCCW,
            magma::renderstates::dontMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendRgb,
            pipelineLayout,
            fb.renderPass, 0,
            pipelineCache);
        texPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "tex.o",
            magma::renderstates::pos2fTex2f,
            magma::renderstates::triangleStrip,
            magma::renderstates::fillCullBackCCW,
            magma::renderstates::dontMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordOffscreenCommandBuffer(const Framebuffer& fb)
    {
        rtCmdBuffer = std::make_shared<magma::PrimaryCommandBuffer>(commandPools[0]);
        rtCmdBuffer->begin();
        {
            constexpr magma::ClearColor clearColor(0.35f, 0.53f, 0.7f, 1.0f);
            rtCmdBuffer->beginRenderPass(fb.renderPass, fb.framebuffer,
                {
                    clearColor,
                    magma::clears::depthOne
                });
            {
                rtCmdBuffer->setViewport(magma::Viewport(0, 0, fb.framebuffer->getExtent()));
                rtCmdBuffer->setScissor(magma::Scissor(0, 0, fb.framebuffer->getExtent()));
                rtCmdBuffer->bindDescriptorSet(rtPipeline, descriptorSet);
                rtCmdBuffer->bindPipeline(rtPipeline);
                rtCmdBuffer->draw(3, 0);
            }
            rtCmdBuffer->endRenderPass();
        }
        rtCmdBuffer->end();
        rtSemaphore = std::make_shared<magma::Semaphore>(device);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clears::grayColor
                });
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(texPipeline, descriptorSet);
                cmdBuffer->bindPipeline(texPipeline);
                cmdBuffer->bindVertexBuffer(0, vertexBuffer);
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

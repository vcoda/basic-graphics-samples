#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

class RenderToTextureApp : public VulkanApp
{
    struct Framebuffer
    {
        constexpr static uint32_t width = 128;
        constexpr static uint32_t height = 128;

        std::shared_ptr<magma::ImageView> colorView;
        std::shared_ptr<magma::ImageView> depthView;
        std::shared_ptr<magma::RenderPass> renderPass;
        std::unique_ptr<magma::Framebuffer> framebuffer;
    } fb;

    struct RtDescriptorSetTable
    {
        magma::descriptor::UniformBuffer world = 0;
    } setTableRt;

    struct TxDescriptorSetTable
    {
        magma::descriptor::CombinedImageSampler texture = 0;
    } setTableTx;

    std::unique_ptr<magma::VertexBuffer> vertexBuffer;
    std::unique_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::unique_ptr<magma::Sampler> nearestSampler;
    std::unique_ptr<magma::CommandBuffer> rtCmdBuffer;
    std::unique_ptr<magma::Semaphore> rtSemaphore;
    std::unique_ptr<magma::DescriptorSet> rtDescriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> rtPipeline;
    std::unique_ptr<magma::DescriptorSet> txDescriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> txPipeline;

public:
    RenderToTextureApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("10.a - Render to texture"), 512, 512)
    {
        initialize();
        createFramebuffer({fb.width, fb.height});
        createVertexBuffer();
        createUniformBuffer();
        createSampler();
        setupDescriptorSets();
        setupPipelines();
        recordOffscreenCommandBuffer(fb);
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        updateWorldTransform();
        constexpr VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        graphicsQueue->submit(rtCmdBuffer, stageMask,
            presentFinished[frameIndex], // Wait for swapchain
            rtSemaphore, // Signal when render-to-texture finished
            nullptr);
        const std::unique_ptr<magma::Fence>& frameFence = (PresentationWait::Fence == presentWait) ? waitFences[frameIndex] : nullFence;
        graphicsQueue->submit(commandBuffers[bufferIndex], stageMask,
            rtSemaphore, // Wait for render-to-texture
            renderFinished[frameIndex], // Semaphore to be signaled when command buffer completed execution
            frameFence); // Fence to be signaled when command buffer completed execution
    }

    void updateWorldTransform()
    {
        constexpr float speed = 0.02f;
        static float angle = 0.f;
        const float step = timer->millisecondsElapsed() * speed;
        angle += step;
        const rapid::matrix roll = rapid::rotationZ(rapid::radians(angle));
        magma::map(uniformBuffer,
            [&roll](auto *world)
            {
                *world = roll;
            });
    }

    void createFramebuffer(const VkExtent2D& extent)
    {
        constexpr bool sampled = true;
        constexpr bool dontSampled = false;
        constexpr VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
        // Create color attachment
        std::unique_ptr<magma::Image> color = std::make_unique<magma::ColorAttachment>(device, colorFormat, extent, 1, 1, sampled);
        fb.colorView = std::make_shared<magma::UniqueImageView>(std::move(color));
        // Create depth attachment
        const VkFormat depthFormat = utilities::getSupportedDepthFormat(physicalDevice, false, true);
        std::unique_ptr<magma::Image> depth = std::make_unique<magma::DepthStencilAttachment>(device, depthFormat, extent, 1, 1, dontSampled);
        fb.depthView = std::make_shared<magma::UniqueImageView>(std::move(depth));
        // Don't care about initial layout
        constexpr VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // Define that color attachment can be cleared, can store shader output and should be read-only image
        const magma::AttachmentDescription colorAttachment(colorFormat, 1,
            magma::op::clearStore, // Color clear, store
            magma::op::dontCare, // Inapplicable
            initialLayout,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); // Should be read-only in the shader when a render pass instance ends
        // Define that depth attachment can be cleared and can store shader output
        const magma::AttachmentDescription depthAttachment(depthFormat, 1,
            magma::op::clearStore, // Depth clear, store
            magma::op::dontCare, // Don't care about stencil
            initialLayout,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL); // Stay as depth/stencil attachment
        // Render pass defines attachment formats, load/store operations and final layouts
        fb.renderPass = std::shared_ptr<magma::RenderPass>(new magma::RenderPass(
            device, {colorAttachment, depthAttachment}));
        // Framebuffer defines render pass, color/depth/stencil image views and dimensions
        fb.framebuffer = std::unique_ptr<magma::Framebuffer>(new magma::Framebuffer(
            fb.renderPass, {fb.colorView, fb.depthView}));
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            float x, y;
            float u, v;
        };

        constexpr float w = 0.75f, h = 0.75f;
        const Vertex vertices[] = {
            {-w, -h, 0.f, 0.f},
            {-w,  h, 0.f, 1.f},
            { w, -h, 1.f, 0.f},
            { w,  h, 1.f, 1.f}
        };
        vertexBuffer = utilities::makeVertexBuffer(vertices, cmdBufferCopy);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_unique<magma::UniformBuffer<rapid::matrix>>(device, false);
    }

    void createSampler()
    {
        nearestSampler = std::make_unique<magma::Sampler>(device, magma::sampler::magMinMipNearestClampToEdge);
    }

    void setupDescriptorSets()
    {
        setTableRt.world = uniformBuffer;
        rtDescriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTableRt, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, 0, shaderReflectionFactory, "triangle");
        setTableTx.texture = {fb.colorView, nearestSampler};
        txDescriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTableTx, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, 0, shaderReflectionFactory, "tex");
    }

    void setupPipelines()
    {
        std::unique_ptr<magma::PipelineLayout> rtLayout = std::make_unique<magma::PipelineLayout>(rtDescriptorSet->getLayout());
        rtPipeline = std::make_unique<GraphicsPipeline>(device,
            "triangle", "fill",
            magma::renderstate::nullVertexInput,
            magma::renderstate::triangleList,
            magma::renderstate::fillCullBackCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            std::move(rtLayout),
            fb.renderPass, 0,
            pipelineCache);
        std::unique_ptr<magma::PipelineLayout> txLayout = std::make_unique<magma::PipelineLayout>(txDescriptorSet->getLayout());
        txPipeline = std::make_unique<GraphicsPipeline>(device,
            "passthrough", "tex",
            magma::renderstate::pos2fTex2f,
            magma::renderstate::triangleStrip,
            magma::renderstate::fillCullBackCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            std::move(txLayout),
            renderPass, 0,
            pipelineCache);
    }

    void recordOffscreenCommandBuffer(const Framebuffer& fb)
    {
        /* VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT specifies that
           a command buffer can be resubmitted to a queue while it is in
           the pending state, and recorded into multiple primary command buffers. */
        rtCmdBuffer = std::make_unique<magma::PrimaryCommandBuffer>(commandPools[0]);
        rtCmdBuffer->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
        {
            rtCmdBuffer->beginRenderPass(fb.renderPass, fb.framebuffer,
                {
                    magma::ClearColor(0.35f, 0.53f, 0.7f, 1.f),
                    magma::clear::depthOne
                });
            {
                rtCmdBuffer->setViewport(magma::Viewport(0, 0, fb.framebuffer->getExtent()));
                rtCmdBuffer->setScissor(magma::Scissor(0, 0, fb.framebuffer->getExtent()));
                rtCmdBuffer->bindDescriptorSet(rtPipeline, 0, rtDescriptorSet);
                rtCmdBuffer->bindPipeline(rtPipeline);
                rtCmdBuffer->draw(3, 0);
            }
            rtCmdBuffer->endRenderPass();
        }
        rtCmdBuffer->end();
        rtSemaphore = std::make_unique<magma::Semaphore>(device);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clear::gray
                });
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(txPipeline, 0, txDescriptorSet);
                cmdBuffer->bindPipeline(txPipeline);
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
    return std::unique_ptr<RenderToTextureApp>(new RenderToTextureApp(entry));
}

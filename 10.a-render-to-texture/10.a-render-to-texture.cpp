#include "../framework/vulkanApp.h"
#include "../framework/bufferFromArray.h"
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

    struct RtDescriptorSetTable : magma::DescriptorSetTable
    {
        magma::descriptor::UniformBuffer world = 0;
        MAGMA_REFLECT(world)
    } setTableRt;

    struct TxDescriptorSetTable : magma::DescriptorSetTable
    {
        magma::descriptor::CombinedImageSampler texture = 0;
        MAGMA_REFLECT(texture)
    } setTableTx;

    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::shared_ptr<magma::Sampler> nearestSampler;
    std::shared_ptr<magma::CommandBuffer> rtCmdBuffer;
    std::shared_ptr<magma::Semaphore> rtSemaphore;
    std::shared_ptr<magma::DescriptorSet> rtDescriptorSet;
    std::shared_ptr<magma::PipelineLayout> rtPipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> rtPipeline;
    std::shared_ptr<magma::DescriptorSet> txDescriptorSet;
    std::shared_ptr<magma::PipelineLayout> txPipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> txPipeline;

public:
    RenderToTextureApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("10.a - Render to texture"), 512, 512)
    {
        initialize();
        createFramebuffer({fbSize, fbSize});
        createVertexBuffer();
        createUniformBuffer();
        createSampler();
        setupDescriptorSets();
        setupPipelines();
        recordOffscreenCommandBuffer(fb);
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        updateWorldTransform();
        constexpr VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        graphicsQueue->submit(rtCmdBuffer, stageMask,
            presentFinished, // Wait for swapchain
            rtSemaphore, // Signal when render-to-texture finished
            nullptr);
        graphicsQueue->submit(commandBuffers[bufferIndex], stageMask,
            rtSemaphore, // Wait for render-to-texture
            renderFinished, // Signal when command buffer execution finished
            (WaitMethod::Fence == waitMethod) ? waitFences[bufferIndex] : nullptr);
    }

    void updateWorldTransform()
    {
        constexpr float speed = 0.02f;
        static float angle = 0.f;
        const float step = timer->millisecondsElapsed() * speed;
        angle += step;
        const rapid::matrix roll = rapid::rotationZ(rapid::radians(angle));
        magma::helpers::mapScoped(uniformBuffer,
            [&roll](auto *world)
            {
                *world = roll;
            });
    }

    void createFramebuffer(const VkExtent2D& extent)
    {
        constexpr bool sampled = true;
        constexpr bool dontSampled = false;
        // Create color attachment
        fb.color = std::make_shared<magma::ColorAttachment>(device, VK_FORMAT_R8G8B8A8_UNORM, extent, 1, 1, sampled);
        fb.colorView = std::make_shared<magma::ImageView>(fb.color);
        // Create depth attachment
        const VkFormat depthFormat = utilities::getSupportedDepthFormat(physicalDevice, false, true);
        fb.depth = std::make_shared<magma::DepthStencilAttachment>(device, depthFormat, extent, 1, 1, dontSampled);
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
        vertexBuffer = vertexBufferFromArray<magma::VertexBuffer>(cmdBufferCopy, vertices);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device, false);
    }

    void createSampler()
    {
        nearestSampler = std::make_shared<magma::Sampler>(device, magma::sampler::magMinMipNearestClampToEdge);
    }

    void setupDescriptorSets()
    {
        setTableRt.world = uniformBuffer;
        rtDescriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setTableRt, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, shaderReflectionFactory, "triangle.o");
        setTableTx.texture = {fb.colorView, nearestSampler};
        txDescriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setTableTx, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, shaderReflectionFactory, "tex.o");
    }

    void setupPipelines()
    {
        rtPipelineLayout = std::make_shared<magma::PipelineLayout>(rtDescriptorSet->getLayout());
        rtPipeline = std::make_shared<GraphicsPipeline>(device,
            "triangle.o", "fill.o",
            magma::renderstate::nullVertexInput,
            magma::renderstate::triangleList,
            magma::renderstate::fillCullBackCCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            rtPipelineLayout,
            fb.renderPass, 0,
            pipelineCache);
        txPipelineLayout = std::make_shared<magma::PipelineLayout>(txDescriptorSet->getLayout());
        txPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "tex.o",
            magma::renderstate::pos2fTex2f,
            magma::renderstate::triangleStrip,
            magma::renderstate::fillCullBackCCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            txPipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordOffscreenCommandBuffer(const Framebuffer& fb)
    {
        /* VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT specifies that
           a command buffer can be resubmitted to a queue while it is in
           the pending state, and recorded into multiple primary command buffers. */
        rtCmdBuffer = std::make_shared<magma::PrimaryCommandBuffer>(commandPools[0]);
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
        rtSemaphore = std::make_shared<magma::Semaphore>(device);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
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

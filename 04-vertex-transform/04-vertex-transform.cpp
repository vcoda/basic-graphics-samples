#include "../framework/vulkanApp.h"

class VertexTransformApp : public VulkanApp
{
    // https://www.evl.uic.edu/ralph/508S98/coordinates.html
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb204853(v=vs.85).aspx
    // OpenGL uses right-handed coordinate system, whilst Direct3D and RenderMan use left-handed
    constexpr static bool rhs = true;

    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix viewProj;

public:
    VertexTransformApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("04 - Vertex transform"), 512, 512)
    {
        initialize();
        setupView();
        createVertexBuffer();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        timer->run();
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        queue->submit(
            commandBuffers[bufferIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 3.5f);
        const rapid::vector3 center(0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        const float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        const float zn = 0.1f, zf = 100.f;
        rapid::matrix view, proj;
        if (rhs)
        {
            view = rapid::lookAtRH(eye, center, up);
            proj = rapid::perspectiveFovRH(fov, aspect, zn, zf);
        }
        else
        {
            view = rapid::lookAtLH(eye, center, up);
            proj = rapid::perspectiveFovLH(fov, aspect, zn, zf);
        }
        viewProj = view * proj;
    }

    void updatePerspectiveTransform()
    {
        const float speed = 0.05f;
        const float step = timer->millisecondsElapsed() * speed;
        static float angle = 0.f;
        angle += rhs ? step : -step; // Preserve direction
        const rapid::matrix world = rapid::rotationY(rapid::radians(angle));
        magma::helpers::mapScoped<rapid::matrix>(uniformBuffer, true, [this, &world](auto *worldViewProj)
        {
            *worldViewProj = world * viewProj;
        });
    }

    virtual void createLogicalDevice() override
    {
        device = physicalDevice->createDefaultDevice();
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            rapid::float3 position;
            unsigned char color[4];
        };
        // Take into account that unlike OpenGL, Vulkan Y axis points down the screen
        constexpr unsigned char _1 = std::numeric_limits<unsigned char>::max();
        const std::vector<Vertex> vertices = {
            {   // top
                {0.0f, -1.0f, 0.0f},
                {_1, 0, 0, _1}
            },
            {   // left
                {-1.0f, 1.0f, 0.0f},
                {0, 0, _1, _1 }
            },
            {   // right
                {1.0f, 1.0f, 0.0f},
                {0, _1, 0, _1 }
            }
        };
        vertexBuffer.reset(new magma::VertexBuffer(device, vertices));
    }

    void createUniformBuffer()
    {
        uniformBuffer.reset(new magma::UniformBuffer<rapid::matrix>(device));
    }

    void setupDescriptorSet()
    {
        // Create descriptor pool
        const uint32_t maxDescriptorSets = 1; // One set is enough for us
        descriptorPool.reset(new magma::DescriptorPool(device, maxDescriptorSets, {
            magma::descriptors::UniformBuffer(1), // Allocate simply one uniform buffer
        }));
        // Setup descriptor set layout:
        // Here we describe that slot 0 in vertex shader will have uniform buffer binding
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        descriptorSetLayout.reset(new magma::DescriptorSetLayout(device, {
            magma::bindings::VertexStageBinding(0, uniformBufferDesc)
        }));
        // Connect our uniform buffer to binding point
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformBuffer);
    }

    void setupPipeline()
    {
        pipelineLayout.reset(new magma::PipelineLayout(descriptorSetLayout));
        graphicsPipeline.reset(new magma::GraphicsPipeline(device, pipelineCache,
            {
                VertexShader(device, "transform.o"),
                FragmentShader(device, "frontFace.o")
            },
            magma::states::pos3Float_Col4UNorm,
            magma::states::triangleList,
            rhs ? magma::states::fillCullNoneCCW
                : magma::states::fillCullNoneCW,
            magma::states::dontMultisample,
            magma::states::depthAlwaysDontWrite,
            magma::states::dontBlendWriteRGB,
            {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass));
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
                cmdBuffer->bindVertexBuffer(0, vertexBuffer);
                cmdBuffer->draw(3, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<IApplication>(new VertexTransformApp(entry));
}

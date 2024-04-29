#include "../framework/vulkanApp.h"
#include "../framework/bufferFromArray.h"

class VertexTransformApp : public VulkanApp
{
    // OpenGL uses right-handed coordinate system, whilst Direct3D and RenderMan use left-handed
    // https://www.evl.uic.edu/ralph/508S98/coordinates.html
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb204853(v=vs.85).aspx
    constexpr static bool rhs = true;

    struct DescriptorSetTable : magma::DescriptorSetTable
    {
        magma::descriptor::UniformBuffer worldViewProj = 0;
        MAGMA_REFLECT(worldViewProj)
    } setTable;

    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
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
        recordCommandBuffer(Buffer::Front);
        recordCommandBuffer(Buffer::Back);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 2.3f);
        const rapid::vector3 center(0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        constexpr float fov = rapid::radians(45.f);
        const float aspect = width/(float)height;
        constexpr float zn = 0.1f, zf = 100.f;
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
        constexpr float speed = 0.05f;
        const float step = timer->millisecondsElapsed() * speed;
        static float angle = 0.f;
        angle += rhs ? step : -step; // Preserve direction
        const rapid::matrix world = rapid::rotationY(rapid::radians(angle));
        magma::helpers::mapScoped(uniformBuffer,
            [this, &world](auto *worldViewProj)
            {
                *worldViewProj = world * viewProj;
            });
    }

    void createVertexBuffer()
    {
        struct Vertex
        {
            float pos[2];
            uint8_t color[4];
        };

        // Take into account that unlike OpenGL, Vulkan Y axis points down the screen
        alignas(MAGMA_ALIGNMENT) Vertex vertices[] = {
            {{ 0.0f,-0.3f}, {255, 0, 0, 255}}, // top
            {{-0.6f, 0.3f}, {0, 255, 0, 255}}, // left
            {{ 0.6f, 0.3f}, {0, 0, 255, 255}}  // right
        };
        vertexBuffer = vertexBufferFromArray<magma::VertexBuffer>(cmdBufferCopy, vertices);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
    }

    void setupDescriptorSet()
    {
        setTable.worldViewProj = uniformBuffer;
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, shaderReflectionFactory, "transform.o");
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "transform.o", "frontFace.o",
            magma::renderstate::pos2fColor4ub,
            magma::renderstate::triangleList,
            rhs ? magma::renderstate::fillCullNoneCcw
                : magma::renderstate::fillCullNoneCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clear::gray});
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
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
    return std::unique_ptr<VertexTransformApp>(new VertexTransformApp(entry));
}

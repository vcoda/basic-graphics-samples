#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"
#include "quadric/include/knot.h"

#define CAPTION_STRING(name) TEXT("13 - Specialization constants (" name ")")

// Use Space to toggle between albedo and shading.
// Use 1, 2, 3 to change shading branch.
// Use L button + mouse to rotate knot.
class SpecializationApp : public VulkanApp
{
    enum ShadingType : uint32_t
    {
        Normal = 0, Diffuse, Specular, CellShade, Albedo,
        MaxShaderVariants
    };

    struct Constants
    {
        // If the specialization constant is of type boolean,
        // size must be the byte size of VkBool32
        VkBool32 colorFill;
        int shadingType;
    };

    struct alignas(16) UniformBlock
    {
        rapid::matrix worldView;
        rapid::matrix worldViewProj;
        rapid::matrix normalMatrix;
    };

    struct SetLayout : magma::DescriptorSetLayoutReflection
    {
        magma::binding::UniformBuffer transforms = 0;
        MAGMA_REFLECT(&transforms)
    } setLayout;

    const std::unordered_map<ShadingType, std::tstring> captions = {
        {ShadingType::Normal, CAPTION_STRING("Normals")},
        {ShadingType::Diffuse, CAPTION_STRING("Diffuse")},
        {ShadingType::Specular, CAPTION_STRING("Specular")},
        {ShadingType::CellShade, CAPTION_STRING("Cell-shading")},
        {ShadingType::Albedo, CAPTION_STRING("Albedo")}
    };

    std::unique_ptr<quadric::Knot> mesh;
    std::shared_ptr<magma::ShaderModule> vertexShader;
    std::shared_ptr<magma::ShaderModule> fragmentShader;
    std::shared_ptr<magma::UniformBuffer<UniformBlock>> uniformBuffer;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::unique_ptr<magma::GraphicsPipelines> pipelines;
    std::vector<std::shared_ptr<magma::CommandBuffer>> commandBuffers[2];

    rapid::matrix view;
    rapid::matrix proj;
    ShadingType shadingType = ShadingType::Normal;
    bool colorFill = true; // Albedo
    int pipelineIndex = ShadingType::Albedo;

public:
    SpecializationApp(const AppEntry& entry):
        VulkanApp(entry, CAPTION_STRING("Albedo"), 512, 512, true)
    {
        initialize();
        setupView();
        createMesh();
        loadShaders();
        createUniformBuffer();
        setupDescriptorSet();
        buildPipelines();
        for (uint32_t i = 0; i < ShadingType::MaxShaderVariants; ++i)
        {
            recordCommandBuffer(FrontBuffer, i);
            recordCommandBuffer(BackBuffer, i);
        }
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        graphicsQueue->submit(
            commandBuffers[bufferIndex][pipelineIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished,
            renderFinished,
            waitFences[bufferIndex]);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case '1': shadingType = ShadingType::Normal; break;
        case '2': shadingType = ShadingType::Diffuse; break;
        case '3': shadingType = ShadingType::Specular; break;
        case '4': shadingType = ShadingType::CellShade; break;
        case AppKey::Space: colorFill = !colorFill;
            break;
        }
        if (colorFill)
            pipelineIndex = ShadingType::Albedo;
        else
            pipelineIndex = shadingType;
        setWindowCaption(captions.at(ShadingType(pipelineIndex)));
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    virtual void createCommandBuffers() override
    {
        VulkanApp::createCommandBuffers();
        commandBuffers[FrontBuffer] = commandPools[0]->allocateCommandBuffers(ShadingType::MaxShaderVariants, true);
        commandBuffers[BackBuffer] = commandPools[0]->allocateCommandBuffers(ShadingType::MaxShaderVariants, true);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 5.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        constexpr float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        constexpr float zn = 1.f, zf = 100.f;
        view = rapid::lookAtRH(eye, center, up);
        proj = rapid::perspectiveFovRH(fov, aspect, zn, zf);
    }

    void updatePerspectiveTransform()
    {
        const rapid::matrix pitch = rapid::rotationX(rapid::radians(spinY/2.f));
        const rapid::matrix yaw = rapid::rotationY(rapid::radians(spinX/2.f));
        const rapid::matrix world = pitch * yaw;
        magma::helpers::mapScoped(uniformBuffer,
            [this, &world](auto *block)
            {
                block->worldView = world * view;
                block->worldViewProj = world * view * proj;
                block->normalMatrix = rapid::transpose(rapid::inverse(block->worldView));
            });
    }

    void createMesh()
    {
        constexpr float radius = 0.25f;
        constexpr uint16_t turns = 3;
        constexpr uint16_t sides = 32;
        constexpr uint16_t rings = 256;
        mesh = std::make_unique<quadric::Knot>(radius, turns, sides, rings, false, cmdBufferCopy);
    }

    void loadShaders()
    {
        aligned_vector<char> bytecode = utilities::loadBinaryFile("transform.o");
        vertexShader = std::make_shared<magma::ShaderModule>(device, (const magma::SpirvWord *)bytecode.data(), bytecode.size());
        bytecode = utilities::loadBinaryFile("specialized.o");
        fragmentShader = std::make_shared<magma::ShaderModule>(device, (const magma::SpirvWord *)bytecode.data(), bytecode.size());
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<UniformBlock>>(device);
    }

    void setupDescriptorSet()
    {
        setLayout.transforms = uniformBuffer;
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setLayout, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, shaderReflectionFactory, "transform.o");
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
    }

    magma::FragmentShaderStage specializeFragmentStage(ShadingType shadingType, const char *entrypoint) const
    {
        Constants constants;
        constants.colorFill = MAGMA_BOOLEAN(ShadingType::Albedo == shadingType);
        constants.shadingType = static_cast<int>(shadingType);
        std::shared_ptr<magma::Specialization> specialization(std::make_shared<magma::Specialization>(constants,
            std::initializer_list<magma::SpecializationEntry>{
                magma::SpecializationEntry(0, &Constants::colorFill),
                magma::SpecializationEntry(1, &Constants::shadingType)
            }));
        return magma::FragmentShaderStage(fragmentShader, entrypoint, specialization);
    }

    void buildPipelines()
    {
        const magma::VertexShaderStage vertexStage(vertexShader, "main");
        const std::vector<VkDynamicState> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        pipelines = std::make_unique<magma::GraphicsPipelines>(MaxShaderVariants);
        for (uint32_t i = 0; i < ShadingType::MaxShaderVariants; ++i)
        {
            const magma::FragmentShaderStage fragmentStage = specializeFragmentStage((ShadingType)i, "main");
            pipelines->newPipeline(
                {vertexStage, fragmentStage},
                mesh->getVertexInput(),
                magma::renderstate::triangleList,
                magma::TesselationState(),
                magma::ViewportState(),
                negateViewport ? magma::renderstate::fillCullBackCCW : magma::renderstate::fillCullBackCW,
                magma::renderstate::dontMultisample,
                magma::renderstate::depthLessOrEqual,
                magma::renderstate::dontBlendRgb,
                dynamicStates,
                pipelineLayout,
                renderPass, 0);
        }
        pipelines->buildPipelines(device, pipelineCache);
    }

    void recordCommandBuffer(uint32_t bufferIndex, uint32_t pipelineIndex)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[bufferIndex][pipelineIndex];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[bufferIndex],
                {
                    magma::clear::gray,
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(pipelines->getPipeline(pipelineIndex), 0, descriptorSet);
                cmdBuffer->bindPipeline(pipelines->getPipeline(pipelineIndex));
                mesh->draw(cmdBuffer);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<SpecializationApp>(new SpecializationApp(entry));
}

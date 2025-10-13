#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"
#include "quadric/include/knot.h"

#define CAPTION_STRING(name) TEXT("13 - Specialization constants (" name ")")

// Use Space to toggle between albedo and shading.
// Use 1, 2, 3, 4, 5 to change shading branch.
// Use L button + mouse to rotate knot.
class SpecializationApp : public VulkanApp
{
    enum ShadingType : uint32_t
    {
        Normal = 0, FaceNormal, Diffuse, Specular, CellShade, Albedo,
        MaxPermutations
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

    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer transforms = 0;
    } setTable;

    const std::unordered_map<ShadingType, std::tstring> captions = {
        {ShadingType::Normal, CAPTION_STRING("Normals")},
        {ShadingType::FaceNormal, CAPTION_STRING("Face normals")},
        {ShadingType::Diffuse, CAPTION_STRING("Diffuse")},
        {ShadingType::Specular, CAPTION_STRING("Specular")},
        {ShadingType::CellShade, CAPTION_STRING("Cell-shading")},
        {ShadingType::Albedo, CAPTION_STRING("Albedo")}
    };

    std::unique_ptr<quadric::Knot> mesh;
    std::shared_ptr<magma::ShaderModule> vertexShader;
    std::shared_ptr<magma::ShaderModule> fragmentShader;
    std::unique_ptr<magma::UniformBuffer<UniformBlock>> uniformBuffer;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> sharedLayout;
    std::shared_ptr<magma::RenderPass> sharedRenderPass;
    std::unique_ptr<magma::GraphicsPipelineBatch> pipelineBatch;
    std::vector<std::vector<std::shared_ptr<magma::CommandBuffer>>> commandBuffers;

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
        for (uint32_t pipelineIndex = 0; pipelineIndex < ShadingType::MaxPermutations; ++pipelineIndex)
        {
            for (uint32_t bufferIndex = 0; bufferIndex < (uint32_t)commandBuffers.size(); ++bufferIndex)
                recordCommandBuffer(bufferIndex, pipelineIndex);
        }
    }

    void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        const std::unique_ptr<magma::Fence>& frameFence = (PresentationWait::Fence == presentWait) ? waitFences[frameIndex] : nullFence;
        graphicsQueue->submit(
            commandBuffers[bufferIndex][pipelineIndex],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            presentFinished[frameIndex], // Wait for swapchain
            renderFinished[frameIndex], // Semaphore to be signaled when command buffer completed execution
            frameFence);
    }

    void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case '1': shadingType = ShadingType::Normal; break;
        case '2': shadingType = ShadingType::FaceNormal; break;
        case '3': shadingType = ShadingType::Diffuse; break;
        case '4': shadingType = ShadingType::Specular; break;
        case '5': shadingType = ShadingType::CellShade; break;
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

    void createCommandBuffers() override
    {
        VulkanApp::createCommandBuffers();
        for (size_t i = 0; i < framebuffers.size(); ++i)
        {
            std::vector<std::shared_ptr<magma::CommandBuffer>> forEachPermutationCmdBuffers = commandPools[0]->allocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, ShadingType::MaxPermutations);
            commandBuffers.emplace_back(std::move(forEachPermutationCmdBuffers));
        }
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
        magma::map(uniformBuffer,
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
        constexpr uint16_t sides = 16;
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
        uniformBuffer = std::make_unique<magma::UniformBuffer<UniformBlock>>(device);
    }

    void setupDescriptorSet()
    {
        setTable.transforms = uniformBuffer;
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, 0, shaderReflectionFactory, "transform");
        sharedLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
    }

    magma::FragmentShaderStage specializeFragmentStage(ShadingType shadingType, const char *entrypoint) const
    {
        Constants constants;
        constants.colorFill = MAGMA_BOOLEAN(ShadingType::Albedo == shadingType);
        constants.shadingType = static_cast<int>(shadingType);
        std::shared_ptr<magma::Specialization> specialization = std::make_shared<magma::Specialization>(constants,
            std::initializer_list<magma::SpecializationEntry>{
                magma::SpecializationEntry(0, &Constants::colorFill),
                magma::SpecializationEntry(1, &Constants::shadingType)
            });
        return magma::FragmentShaderStage(fragmentShader, entrypoint, std::move(specialization));
    }

    void buildPipelines()
    {
        const magma::VertexShaderStage vertexStage(vertexShader, "main");
        const std::vector<VkDynamicState> dynamicStates{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        sharedRenderPass = std::move(renderPass);
        pipelineBatch = std::make_unique<magma::GraphicsPipelineBatch>(device);
        for (uint32_t i = 0; i < ShadingType::MaxPermutations; ++i)
        {
            const magma::FragmentShaderStage fragmentStage = specializeFragmentStage((ShadingType)i, "main");
            pipelineBatch->batchPipeline(
                {vertexStage, fragmentStage},
                mesh->getVertexInput(),
                magma::renderstate::triangleList,
                magma::TesselationState(),
                magma::ViewportState(),
                negateViewport ? magma::renderstate::fillCullBackCcw
                               : magma::renderstate::fillCullBackCw,
                magma::renderstate::dontMultisample,
                magma::renderstate::depthLessOrEqual,
                magma::renderstate::dontBlendRgb,
                dynamicStates,
                sharedLayout,
                sharedRenderPass, 0);
        }
        std::future<void> buildResult = pipelineBatch->buildPipelinesAsync(pipelineCache);
        std::future_status status;
        do
        {
            status = buildResult.wait_for(std::chrono::milliseconds(1));
            std::cout << "You can do something useful here while waiting for completion of graphics pipeline building" << std::endl;
        } while (status != std::future_status::ready);
    }

    void recordCommandBuffer(uint32_t bufferIndex, uint32_t pipelineIndex)
    {
        auto& cmdBuffer = commandBuffers[bufferIndex][pipelineIndex];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(sharedRenderPass, framebuffers[bufferIndex],
                {
                    magma::clear::gray,
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -int32_t(height) : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(pipelineBatch->getPipeline(pipelineIndex), 0, descriptorSet);
                cmdBuffer->bindPipeline(pipelineBatch->getPipeline(pipelineIndex));
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

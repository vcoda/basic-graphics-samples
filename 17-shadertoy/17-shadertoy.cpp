#include <fstream>
#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"
#include "watchdog.h"

// Build this application with Release configuration as shaderc_combined.lib was built in release mode.

class ShaderToyApp : public VulkanApp
{
    struct alignas(16) BuiltInUniforms
    {
        rapid::float2 iResolution;
        rapid::float2 iMouse;
        float iTime;
    };

    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer builtinUniforms = 0;
    } setTable;

    std::unique_ptr<FileWatchdog> watchdog;
    std::unique_ptr<magma::aux::ShaderCompiler> glslCompiler;
    std::shared_ptr<magma::ShaderModule> vertexShader;
    std::shared_ptr<magma::ShaderModule> fragmentShader;
    std::unique_ptr<magma::UniformBuffer<BuiltInUniforms>> builtinUniforms;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

    std::atomic<bool> rebuildCommandBuffers;
    int mouseX = 0;
    int mouseY = 0;
    bool dragging = false;

public:
    ShaderToyApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("17 - ShaderToy"), 512, 512),
        rebuildCommandBuffers(false)
    {
        initialize();
        vertexShader = compileShader("quad.vert");
        fragmentShader = compileShader("shader.frag");
        initializeWatchdog();
        createUniformBuffer();
        setupDescriptorSet();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        if (rebuildCommandBuffers)
        {
            waitFences[1 - bufferIndex]->wait();
            setupPipeline();
            for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
                recordCommandBuffer(i);
            rebuildCommandBuffers = false;
        }
        updateUniforms();
        submitCommandBuffer(bufferIndex);
    }

    void onMouseMove(int x, int y) override
    {
        if (dragging)
        {
            mouseX = x;
            mouseY = y;
        }
    }

    void onMouseLButton(bool down, int x, int y) override
    {
        dragging = down;
        if (dragging)
        {
            mouseX = x;
            mouseY = y;
        }
    }

    void updateUniforms()
    {
        magma::map(builtinUniforms,
            [this](auto *builtin)
            {
                static float totalTime = 0.0f;
                totalTime += timer->secondsElapsed();
                builtin->iResolution.x = static_cast<float>(width);
                builtin->iResolution.y = static_cast<float>(height);
                builtin->iMouse.x = static_cast<float>(mouseX);
                builtin->iMouse.y = static_cast<float>(mouseY);
                builtin->iTime = totalTime;
            });
    }

    std::shared_ptr<magma::ShaderModule> compileShader(const std::string& filename)
    {
        std::shared_ptr<magma::ShaderModule> shaderModule;
        std::ifstream file(filename);
        if (file.is_open())
        {
            std::string source((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());
            std::cout << "compiling shader \"" << filename << "\"" << std::endl;
            shaderc_shader_kind shaderKind;
            if (filename.find(".vert") != std::string::npos)
                shaderKind = shaderc_glsl_default_vertex_shader;
            else
                shaderKind = shaderc_glsl_default_fragment_shader;
            if (!glslCompiler)
                glslCompiler = std::make_unique<magma::aux::ShaderCompiler>(device, nullptr);
            shaderModule = glslCompiler->compileShader(source, "main", shaderKind);
        }
        else
        {
            throw std::runtime_error("failed to open file \"" + std::string(filename) + "\"");
        }
        return shaderModule;
    }

    void initializeWatchdog()
    {
        auto onModified = [this](const std::string& filename) -> void
        {
            try
            {
                if (filename.find(".vert") != std::string::npos)
                    vertexShader = compileShader(filename);
                else
                    fragmentShader = compileShader(filename);
                rebuildCommandBuffers = true;
            } catch (const std::exception& exception)
            {
                std::cout << exception.what();
            }
        };

        constexpr std::chrono::milliseconds pollFrequency(500);
        watchdog = std::make_unique<FileWatchdog>(pollFrequency);
        watchdog->watchFor("quad.vert", onModified);
        watchdog->watchFor("shader.frag", onModified);
    }

    void createUniformBuffer()
    {
        builtinUniforms = std::make_unique<magma::UniformBuffer<BuiltInUniforms>>(device);
    }

    void setupDescriptorSet()
    {
        setTable.builtinUniforms = builtinUniforms;
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    void setupPipeline()
    {
        std::vector<magma::PipelineShaderStage> shaderStages = {
            magma::PipelineShaderStage(vertexShader),
            magma::PipelineShaderStage(fragmentShader)
        };
        auto layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_unique<magma::GraphicsPipeline>(device,
            shaderStages,
            magma::renderstate::nullVertexInput,
            magma::renderstate::triangleStrip,
            magma::TesselationState(),
            magma::ViewportState(0, 0, width, height),
            magma::renderstate::fillCullBackCcw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            std::initializer_list<VkDynamicState>{},
            std::move(layout),
            renderPass, 0,
            nullptr,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
        cmdBuffer->reset(false);
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clear::gray});
            {
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
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
    return std::unique_ptr<ShaderToyApp>(new ShaderToyApp(entry));
}

#include <fstream>
#include "graphicsPipeline.h"

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<magma::Device> device,
    const char *vertexShaderFileName,
    const char *fragmentShaderFileName,
    const magma::VertexInputState& vertexInputState,
    const magma::InputAssemblyState& inputAssemblyState,
    const magma::RasterizationState& rasterizationState,
    const magma::MultisampleState& multisampleState,
    const magma::DepthStencilState& depthStencilState,
    const magma::ColorBlendState& colorBlendState,
    std::shared_ptr<magma::PipelineLayout> layout,
    std::shared_ptr<magma::RenderPass> renderPass,
    uint32_t subpass /* 0 */,
    std::shared_ptr<magma::PipelineCache> pipelineCache /* nullptr */):
    magma::GraphicsPipeline(device,
    std::vector<magma::PipelineShaderStage>{
        loadShader(device, vertexShaderFileName),
        loadShader(device, fragmentShaderFileName)},
    vertexInputState,inputAssemblyState, rasterizationState,
    multisampleState, depthStencilState, colorBlendState,
    // Define dynamic states that are used by default
    std::initializer_list<VkDynamicState>{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR},
    std::move(layout),
    std::move(renderPass),
    subpass,
    nullptr,
    std::move(pipelineCache),
    nullptr, 0)
{}

magma::PipelineShaderStage GraphicsPipeline::loadShader(
    std::shared_ptr<magma::Device> device, const char *shaderFileName) const
{
    std::ifstream file(shaderFileName, std::ios::in | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("file \"" + std::string(shaderFileName) + "\" not found");
    std::vector<char> bytecode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (bytecode.size() % sizeof(magma::SpirvWord))
        throw std::runtime_error("size of \"" + std::string(shaderFileName) + "\" bytecode must be a multiple of SPIR-V word");
    auto allocator = device->getHostAllocator();
    constexpr bool reflect = true;
    std::shared_ptr<magma::ShaderModule> module(std::make_shared<magma::ShaderModule>(std::move(device),
        reinterpret_cast<const magma::SpirvWord *>(bytecode.data()), bytecode.size(), 0,
        std::move(allocator), reflect, 0, nullptr));
    const VkShaderStageFlagBits stage = module->getReflection()->getShaderStage();
    const char *const entrypoint = module->getReflection()->getEntryPointName(0);
    return magma::PipelineShaderStage(stage, std::move(module), entrypoint);
}

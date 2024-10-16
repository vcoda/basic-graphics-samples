#pragma once
#include "magma/magma.h"

class GraphicsPipeline : public magma::GraphicsPipeline
{
public:
    explicit GraphicsPipeline(std::shared_ptr<magma::Device> device,
        const char *vertexShaderFileName,
        const char *fragmentShaderFileName,
        const magma::VertexInputState& vertexInputState,
        const magma::InputAssemblyState& inputAssemblyState,
        const magma::RasterizationState& rasterizationState,
        const magma::MultisampleState& multisampleState,
        const magma::DepthStencilState& depthStencilState,
        const magma::ColorBlendState& colorBlendState,
        magma::core::variant_ptr<magma::PipelineLayout> layout,
        std::shared_ptr<magma::RenderPass> renderPass,
        uint32_t subpass = 0,
        const std::unique_ptr<magma::PipelineCache>& pipelineCache = nullptr);

private:
    magma::PipelineShaderStage loadShader(std::shared_ptr<magma::Device> device,
        const char *fileName) const;
};

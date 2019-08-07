#pragma once
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <stdexcept>
#include "magma/objects/shaderModule.h"
#include "magma/shaders/shaderStages.h"

template<typename ShaderStageType>
class SpirvShaderFile
{
public:
    explicit SpirvShaderFile(std::shared_ptr<magma::Device> device,
        const std::string& filename,
        const char *const entrypoint = "main",
        std::shared_ptr<magma::Specialization> specialization = nullptr,
        VkPipelineShaderStageCreateFlags flags = 0)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("file \"" + filename + "\" not found");
        std::vector<char> bytecode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (bytecode.size() % sizeof(magma::SpirvWord))
            throw std::runtime_error("size of \"" + filename + "\" must be a multiple of SPIR-V word");
        std::shared_ptr<magma::ShaderModule> module = std::make_shared<magma::ShaderModule>(std::move(device),
            reinterpret_cast<const magma::SpirvWord *>(bytecode.data()), bytecode.size());
        stage = std::make_shared<ShaderStageType>(std::move(module), entrypoint, std::move(specialization), flags);
    }
    operator magma::PipelineShaderStage() { return *stage; }

private:
    std::shared_ptr<magma::PipelineShaderStage> stage;
};

using VertexShaderFile = SpirvShaderFile<magma::VertexShaderStage>;
using FragmentShaderFile = SpirvShaderFile<magma::FragmentShaderStage>;
using ComputeShaderFile = SpirvShaderFile<magma::ComputeShaderStage>;

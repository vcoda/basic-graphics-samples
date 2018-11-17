#include <vector>
#include <fstream>
#include <cassert>
#include "shader.h"
#include "../third-party/magma/magma.h"

Shader::Shader(std::shared_ptr<magma::Device> device, const std::string& filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        const std::string msg = "failed to open file \"" + filename + "\"";
        throw std::runtime_error(msg.c_str());
    }
    std::vector<char> bytecode((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    assert(bytecode.size() % sizeof(uint32_t) == 0);
    module = std::make_shared<magma::ShaderModule>(device,
        reinterpret_cast<const uint32_t *>(bytecode.data()),
        static_cast<size_t>(bytecode.size()));
}

VertexShader::VertexShader(std::shared_ptr<magma::Device> device, const std::string& filename,
    const char *const entrypoint /* "main" */):
    Shader(device, filename)
{
    stage = std::make_shared<magma::VertexShaderStage>(std::move(module), entrypoint);
}

GeometryShader::GeometryShader(std::shared_ptr<magma::Device> device, const std::string& filename,
    const char *const entrypoint /* "main" */):
    Shader(device, filename)
{
    stage = std::make_shared<magma::GeometryShaderStage>(std::move(module), entrypoint);
}

FragmentShader::FragmentShader(std::shared_ptr<magma::Device> device, const std::string& filename,
    const char *const entrypoint /* "main" */):
    Shader(device, filename)
{
    stage = std::make_shared<magma::FragmentShaderStage>(std::move(module), entrypoint);
}

ComputeShader::ComputeShader(std::shared_ptr<magma::Device> device, const std::string& filename,
    const char *const entrypoint /* "main" */):
    Shader(device, filename)
{
    stage = std::make_shared<magma::ComputeShaderStage>(std::move(module), entrypoint);
}

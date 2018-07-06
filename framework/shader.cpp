#include <vector>
#include <fstream>
#include <cassert>
#include "shader.h"
#include "../third-party/magma/magma.h"

Shader::Shader(std::shared_ptr<magma::Device> device, const std::string& filename)
{
    std::ifstream file(filename,
        std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        const std::string msg = "failed to open file \"" + filename + "\"";
        throw std::runtime_error(msg.c_str());
    }
    const std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);
    assert(size % sizeof(uint32_t) == 0);
    std::vector<char> bytecode(static_cast<size_t>(size));
    file.read(bytecode.data(), size);
    file.close();
    module = std::make_shared<magma::ShaderModule>(device,
        reinterpret_cast<const uint32_t *>(bytecode.data()), size);
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

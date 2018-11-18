#pragma once
#include <memory>
#include <string>

namespace magma
{
    class Device;
    class ShaderModule;
    class PipelineShaderStage;
}

class Shader
{
public:
    Shader(std::shared_ptr<magma::Device> device,
        const std::string& filename);

    operator magma::PipelineShaderStage&()
        { return *stage; }
    operator const magma::PipelineShaderStage&() const
        { return *stage; }

protected:
    std::shared_ptr<magma::ShaderModule> module;
    std::shared_ptr<magma::PipelineShaderStage> stage;
};

class VertexShader : public Shader
{
public:
    VertexShader(std::shared_ptr<magma::Device> device,
        const std::string& filename,
        const char *const entrypoint = "main");
};

class GeometryShader : public Shader
{
public:
    GeometryShader(std::shared_ptr<magma::Device> device,
        const std::string& filename,
        const char *const entrypoint = "main");
};

class FragmentShader : public Shader
{
public:
    FragmentShader(std::shared_ptr<magma::Device> device,
        const std::string& filename,
        const char *const entrypoint = "main");
};

class ComputeShader : public Shader
{
public:
    ComputeShader(std::shared_ptr<magma::Device> device,
        const std::string& filename,
        const char *const entrypoint = "main");
};

#pragma once
#include <fstream>

class ShaderReflectionFactory : public magma::IShaderReflectionFactory
{
public:
    explicit ShaderReflectionFactory(std::shared_ptr<magma::Device> device):
        device(std::move(device))
    {}

    virtual std::shared_ptr<const magma::ShaderReflection> getReflection(const std::string& shaderFileName) override
    {
        std::ifstream file(shaderFileName, std::ios::in | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("file \"" + std::string(shaderFileName) + "\" not found");
        std::vector<char> bytecode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (bytecode.size() % sizeof(magma::SpirvWord))
            throw std::runtime_error("size of \"" + std::string(shaderFileName) + "\" bytecode must be a multiple of SPIR-V word");
        auto allocator = device->getHostAllocator();
        std::shared_ptr<magma::ShaderModule> shaderModule = std::make_shared<magma::ShaderModule>(device,
            reinterpret_cast<const magma::SpirvWord *>(bytecode.data()), bytecode.size(), 0,
            std::move(allocator), 0, true);
        return shaderModule->getReflection();
    }

private:
    std::shared_ptr<magma::Device> device;
};

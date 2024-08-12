#pragma once
#include <fstream>

class ShaderReflectionFactory : public magma::IShaderReflectionFactory
{
public:
    explicit ShaderReflectionFactory(std::shared_ptr<magma::Device> device):
        device(std::move(device))
    {}

    const std::unique_ptr<const magma::ShaderReflection>& getReflection(const std::string& fileName) override
    {
        const std::string shaderFileName = fileName + std::string(".o");
        std::ifstream file(shaderFileName, std::ios::in | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("file \"" + shaderFileName + "\" not found");
        std::vector<char> bytecode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (bytecode.size() % sizeof(magma::SpirvWord))
            throw std::runtime_error("size of \"" + shaderFileName + "\" bytecode must be a multiple of SPIR-V word");
        auto allocator = device->getHostAllocator();
        constexpr bool reflect = true;
        shaderModule = std::make_unique<magma::ShaderModule>(device,
            reinterpret_cast<const magma::SpirvWord *>(bytecode.data()), bytecode.size(), 0,
            std::move(allocator), reflect, 0);
        return shaderModule->getReflection();
    }

private:
    std::shared_ptr<magma::Device> device;
    std::unique_ptr<magma::ShaderModule> shaderModule;
};

#pragma once
#include <vector>
#include <cassert>
#include "alignedAllocator.h"
#include "magma/objects/shaderModule.h"
#include "gliml/gliml.h"

namespace utilities
{
    template<class Type>
    using aligned_vector = std::vector<Type, aligned_allocator<Type>>;

    aligned_vector<char> loadBinaryFile(const std::string& filename);
    VkFormat getBCFormat(const gliml::context& ctx);

    template <typename ShaderStageType>
    inline ShaderStageType loadShader(std::shared_ptr<magma::Device> device, const std::string& filename, const char *entrypoint)
    {
        const aligned_vector<char> bytecode = loadBinaryFile(filename);
        assert(bytecode.size() % sizeof(uint32_t) == 0);
        std::shared_ptr<magma::ShaderModule> module(new magma::ShaderModule(device, (const uint32_t *)bytecode.data(), bytecode.size()));
        return ShaderStageType(module, entrypoint);
    }

	inline std::vector<magma::ShaderStage> loadShaders(
		std::shared_ptr<magma::Device> device, 
		const std::string& vertexFileName, 
		const std::string& fragmentFileName)
	{
		std::vector<magma::ShaderStage> shaderStages;
		shaderStages.push_back(loadShader<magma::VertexShaderStage>(device, vertexFileName, "main"));
        shaderStages.push_back(loadShader<magma::FragmentShaderStage>(device, fragmentFileName, "main"));
		return shaderStages;
	}
} // namespace utilities

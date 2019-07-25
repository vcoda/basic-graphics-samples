#include <fstream>
#include "../framework/vulkanApp.h"
#include "../framework/cubeMesh.h"
#include "../framework/utilities.h"

// Use PgUp/PgDown to select texture lod
class TextureArrayApp : public VulkanApp
{
    struct alignas(16) TexParameters
    {
        float lod;
    };

    std::unique_ptr<CubeMesh> mesh;
    std::shared_ptr<magma::Image2DArray> imageArray;
    std::shared_ptr<magma::ImageView> imageArrayView;
    std::shared_ptr<magma::Sampler> anisotropicSampler;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformWorldViewProj;
    std::shared_ptr<magma::UniformBuffer<TexParameters>> uniformTexParameters;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix viewProj;
    float lod = 0.f;
    bool negateViewport = false;

public:
    TextureArrayApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("07 - Texture array"), 512, 512, true)
    {
        initialize();
        negateViewport = extensions->KHR_maintenance1 || extensions->AMD_negative_viewport_height;
        setupView();
        createMesh();
        loadDDSTextureArray({"dice1.dds" , "dice2.dds", "dice3.dds", "dice4.dds", "dice5.dds", "dice6.dds"});
        createSampler();
        createUniformBuffers();
        setupDescriptorSet();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        timer->run();
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCmdBuffer(bufferIndex);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case AppKey::PgUp:
            if (lod < imageArray->getMipLevels() - 1)
            {
                lod += 1.f;
                updateLod();
            }
            break;
        case AppKey::PgDn:
            if (lod > 0.f)
            {
                lod -= 1.f;
                updateLod();
            }
            break;
        }
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 0.f, 7.f);
        const rapid::vector3 center(0.f, 0.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        constexpr float fov = rapid::radians(60.f);
        const float aspect = width/(float)height;
        constexpr float zn = 1.f, zf = 100.f;
        const rapid::matrix view = rapid::lookAtRH(eye, center, up);
        const rapid::matrix proj = rapid::perspectiveFovRH(fov, aspect, zn, zf);
        viewProj = view * proj;
    }

    void updatePerspectiveTransform()
    {
        constexpr float speed = 0.1f;
        static float angle = 0.f;
        angle += timer->millisecondsElapsed() * speed;
        const float radians = rapid::radians(angle);
        const rapid::matrix pitch = rapid::rotationX(radians);
        const rapid::matrix yaw = rapid::rotationY(radians);
        const rapid::matrix roll = rapid::rotationZ(radians);
        const rapid::matrix world = pitch * yaw * roll;
        magma::helpers::mapScoped<rapid::matrix>(uniformWorldViewProj, true, [this, &world](auto *worldViewProj)
        {
            *worldViewProj = world * viewProj;
        });
    }

    void updateLod()
    {
        magma::helpers::mapScoped<TexParameters>(uniformTexParameters, true, [this](auto *block)
        {
            block->lod = lod;
        });
        std::cout << "Texture LOD: " << lod << "\n";
    }

    void createMesh()
    {
        mesh = std::make_unique<CubeMesh>(cmdBufferCopy);
    }

    void loadDDSTextureArray(const std::vector<std::string>& filenames)
    {
        std::vector<std::shared_ptr<std::ifstream>> files;
        std::streamoff totalSize = 0;
        for (const std::string& filename : filenames)
        {   // Open binary stream
            auto file = std::make_shared<std::ifstream>("textures/" + filename, std::ios::in | std::ios::binary | std::ios::ate);
            if (!file->is_open())
                throw std::runtime_error("failed to open file \"" + filename + "\"");
            totalSize += file->tellg();
            files.push_back(file);
        }
        std::vector<gliml::context> ctxArray;
        std::vector<VkDeviceSize> baseMipOffsets;
        std::shared_ptr<magma::Buffer> buffer = std::make_shared<magma::SrcTransferBuffer>(device, totalSize);
        magma::helpers::mapScoped<uint8_t>(buffer, [&](uint8_t *data)
        {   // Read all data to single buffer
            for (auto& file : files)
            {
                const std::streamoff size = file->tellg();
                file->seekg(0, std::ios::beg);
                file->read(reinterpret_cast<char *>(data), size);
                file->close();
                gliml::context ctx;
                ctx.enable_dxt(true);
                if (!ctx.load(data, static_cast<unsigned>(size)))
                    throw std::runtime_error("failed to load DDS texture");
                ctxArray.push_back(ctx);
                // Skip DDS header
                baseMipOffsets.push_back(reinterpret_cast<const uint8_t *>(ctx.image_data(0, 0)) - data); 
                data += size;
            }
        });
        const VkFormat format = utilities::getBCFormat(ctxArray.front());
        const VkExtent2D extent = {ctxArray.front().image_width(0, 0), ctxArray.front().image_height(0, 0)};
        // Assert that all files have the same format and dimensions
        for (size_t i = 1; i < ctxArray.size(); ++i)
        {
            const gliml::context& ctx = ctxArray[i];
            MAGMA_ASSERT(utilities::getBCFormat(ctx) == format);
            MAGMA_ASSERT(static_cast<uint32_t>(ctx.image_width(0, 0)) == extent.width);
            MAGMA_ASSERT(static_cast<uint32_t>(ctx.image_height(0, 0)) == extent.height);
        }
        // Setup memory layout in transfer buffer
        std::vector<VkDeviceSize> mipOffsets;
        VkDeviceSize lastMipSize = 0;
        uint32_t arrayIndex = 0;
        for (const auto& ctx : ctxArray)
        {   // Take into account DDS header and last mip level of previous mipmap chain
            mipOffsets.push_back(baseMipOffsets[arrayIndex] + lastMipSize);
            const int mipLevels = ctx.num_mipmaps(0);
            for (int level = 1; level < mipLevels; ++level)
            {   // Compute relative offset
                const intptr_t mipOffset = (const uint8_t *)ctx.image_data(0, level) - (const uint8_t *)ctx.image_data(0, level - 1);
                mipOffsets.push_back(mipOffset);
            }
            lastMipSize = ctx.image_size(0, mipLevels - 1);
            ++arrayIndex;
        }
        // Upload texture array data from buffer
        const uint32_t arrayLayers = MAGMA_COUNT(ctxArray);
        imageArray = std::make_shared<magma::Image2DArray>(device, format, extent, arrayLayers, buffer, 0, mipOffsets, cmdImageCopy);
        // Create image view for pixel shader
        imageArrayView = std::make_shared<magma::ImageView>(imageArray);
    }

    void createSampler()
    {
        anisotropicSampler = std::make_shared<magma::Sampler>(device, magma::samplers::magMinLinearMipAnisotropicClampToEdge);
    }

    void createUniformBuffers()
    {
        uniformWorldViewProj = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
        uniformTexParameters = std::make_shared<magma::UniformBuffer<TexParameters>>(device);
        updateLod();
    }

    void setupDescriptorSet()
    {   // Create descriptor pool
        constexpr uint32_t maxDescriptorSets = 1; // One set is enough for us
        const magma::Descriptor uniformBufferDesc = magma::descriptors::UniformBuffer(1);
        const magma::Descriptor imageSamplerDesc = magma::descriptors::CombinedImageSampler(1);
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, maxDescriptorSets,
            std::vector<magma::Descriptor>
            {
                magma::descriptors::UniformBuffer(2), // Allocate two uniform buffers
                imageSamplerDesc // Allocate one combined image sampler
            });
        // Setup descriptor set layout
        descriptorSetLayout = std::make_shared<magma::DescriptorSetLayout>(device,
            std::initializer_list<magma::DescriptorSetLayout::Binding>
            {
                magma::bindings::VertexStageBinding(0, uniformBufferDesc),   // Bind transforms to slot 0 in vertex shader
                magma::bindings::FragmentStageBinding(1, uniformBufferDesc), // Bind texture lod to slot 1 in fragment shader
                magma::bindings::FragmentStageBinding(2, imageSamplerDesc)   // Bind image array sampler to slot 2 in fragment shader
            });
        // Allocate and update descriptor set
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformWorldViewProj);
        descriptorSet->update(1, uniformTexParameters);
        descriptorSet->update(2, imageArrayView, anisotropicSampler);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        graphicsPipeline = std::make_shared<magma::GraphicsPipeline>(device, pipelineCache,
            std::vector<magma::PipelineShaderStage>
            {
                VertexShader(device, "transform.o"),
                FragmentShader(device, "textureArray.o")
            },
            mesh->getVertexInput(),
            magma::renderstates::triangleStrip,
            negateViewport ? magma::renderstates::fillCullBackCCW : magma::renderstates::fillCullBackCW,
            magma::renderstates::noMultisample,
            magma::renderstates::depthLess,
            magma::renderstates::dontBlendWriteRgb,
            std::initializer_list<VkDynamicState>{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
            pipelineLayout,
            renderPass);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->setRenderArea(0, 0, width, height);
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clears::grayColor,
                    magma::clears::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet);
                cmdBuffer->bindPipeline(graphicsPipeline);
                mesh->draw(cmdBuffer);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::make_unique<TextureArrayApp>(entry);
}

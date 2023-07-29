#include <fstream>
#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"
#include "quadric/include/cube.h"

// Use PgUp/PgDown to select texture lod
class TextureArrayApp : public VulkanApp
{
    struct alignas(16) TexParameters
    {
        float lod;
    };

    struct DescriptorSetTable : magma::DescriptorSetTable
    {
        magma::descriptor::UniformBuffer worldViewProj = 0;
        magma::descriptor::UniformBuffer texParameters = 1;
        magma::descriptor::CombinedImageSampler imageArray = 2;
        MAGMA_REFLECT(worldViewProj, texParameters, imageArray)
    } setTable;

    std::unique_ptr<quadric::Cube> mesh;
    std::shared_ptr<magma::ImageView> imageArrayView;
    std::shared_ptr<magma::Sampler> anisotropicSampler;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformWorldViewProj;
    std::shared_ptr<magma::UniformBuffer<TexParameters>> uniformTexParameters;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix viewProj;
    float lod = 0.f;

public:
    TextureArrayApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("07 - Texture array"), 512, 512, true)
    {
        initialize();
        setupView();
        createMesh();
        loadTextureArray({"dice1.dds" , "dice2.dds", "dice3.dds", "dice4.dds", "dice5.dds", "dice6.dds"});
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
        submitCommandBuffer(bufferIndex);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case AppKey::PgUp:
            if (lod < imageArrayView->getImage()->getMipLevels() - 1)
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
        magma::helpers::mapScoped(uniformWorldViewProj,
            [this, &world](auto *worldViewProj)
            {
                *worldViewProj = world * viewProj;
            });
    }

    void updateLod()
    {
        magma::helpers::mapScoped<TexParameters>(uniformTexParameters,
            [this](auto *block)
            {
                block->lod = lod;
            });
        std::cout << "Texture LOD: " << lod << "\n";
    }

    void createMesh()
    {
        mesh = std::make_unique<quadric::Cube>(cmdBufferCopy);
    }

    void loadTextureArray(const std::initializer_list<std::string>& filenames)
    {
        std::list<std::ifstream> files;
        std::streamoff totalSize = 0;
        for (const std::string& filename: filenames)
        {   // Open binary stream
            files.emplace_back("textures/" + filename, std::ios::in | std::ios::binary | std::ios::ate);
            if (!files.back().is_open())
                throw std::runtime_error("failed to open file \"" + filename + "\"");
            totalSize += files.back().tellg();
        }
        std::list<gliml::context> ctxArray;
        std::shared_ptr<magma::SrcTransferBuffer> buffer = std::make_shared<magma::SrcTransferBuffer>(device, totalSize);
        VkDeviceSize baseMipOffset = 0ull;
        magma::helpers::mapScoped<uint8_t>(buffer, [&](uint8_t *data)
        {   // Read all data to single buffer
            for (auto& file: files)
            {
                const std::streamoff size = file.tellg();
                file.seekg(0, std::ios::beg);
                file.read(reinterpret_cast<char *>(data), size);
                file.close();
                ctxArray.emplace_back();
                gliml::context& ctx = ctxArray.back();
                ctx.enable_dxt(true);
                if (!ctx.load(data, static_cast<unsigned>(size)))
                    throw std::runtime_error("failed to load DDS texture");
                // Skip DDS header of the first file
                if (0ull == baseMipOffset)
                    baseMipOffset = reinterpret_cast<const uint8_t *>(ctx.image_data(0, 0)) - data;
                data += size;
            }
        });
        const VkFormat format = utilities::getBlockCompressedFormat(ctxArray.front());
        const VkExtent2D extent = {
            (uint32_t)ctxArray.front().image_width(0, 0),
            (uint32_t)ctxArray.front().image_height(0, 0)
        };
        // Assert that all files have the same format and dimensions
        for (const auto& ctx: ctxArray)
        {
            MAGMA_ASSERT(utilities::getBlockCompressedFormat(ctx) == format);
            MAGMA_ASSERT(static_cast<uint32_t>(ctx.image_width(0, 0)) == extent.width);
            MAGMA_ASSERT(static_cast<uint32_t>(ctx.image_height(0, 0)) == extent.height);
        }
        // Setup texture array data description
        const uint8_t *frontImageFirstMipData = (const uint8_t *)ctxArray.front().image_data(0, 0);
        std::vector<magma::Image::Mip> mipMaps;
        for (const auto& ctx: ctxArray)
        {
            for (int level = 0; level < ctx.num_mipmaps(0); ++level)
            {
                magma::Image::Mip mip;
                mip.extent.width = ctx.image_width(0, level);
                mip.extent.height = ctx.image_height(0, level);
                mip.extent.depth = 1;
                mip.bufferOffset = (const uint8_t *)ctx.image_data(0, level) - frontImageFirstMipData;
                MAGMA_ASSERT(mip.bufferOffset < totalSize);
                mipMaps.push_back(mip);
            }
        }
        // Upload texture array data from buffer
        cmdImageCopy->begin();
        const magma::Image::CopyLayout bufferLayout{baseMipOffset, 0, 0};
        std::shared_ptr<magma::Image2DArray> imageArray = std::make_shared<magma::Image2DArray>(cmdImageCopy,
            format, MAGMA_COUNT(ctxArray), buffer, mipMaps, bufferLayout);
        cmdImageCopy->end();
        submitCopyImageCommands();
        // Create image view for fragment shader
        imageArrayView = std::make_shared<magma::ImageView>(std::move(imageArray));
    }

    void createSampler()
    {
        anisotropicSampler = std::make_shared<magma::Sampler>(device, magma::sampler::magMinLinearMipAnisotropicClampToEdge);
    }

    void createUniformBuffers()
    {
        uniformWorldViewProj = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
        uniformTexParameters = std::make_shared<magma::UniformBuffer<TexParameters>>(device);
        updateLod();
    }

    void setupDescriptorSet()
    {
        setTable.worldViewProj = uniformWorldViewProj;
        setTable.texParameters = uniformTexParameters;
        setTable.imageArray = {imageArrayView, anisotropicSampler};
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, shaderReflectionFactory, "textureArray.o");
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "transform.o", "textureArray.o",
            mesh->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCCw
                           : magma::renderstate::fillCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLess,
            magma::renderstate::dontBlendRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::clear::gray,
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
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
    return std::unique_ptr<TextureArrayApp>(new TextureArrayApp(entry));
}

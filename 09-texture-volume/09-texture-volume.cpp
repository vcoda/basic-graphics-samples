#include <fstream>
#include "../framework/vulkanApp.h"

// Use PgUp/PgDown to change accomodation power
class TextureVolumeApp : public VulkanApp
{
    struct alignas(16) IntegrationParameters
    {
        float power;
    };

    struct DescriptorSetTable : magma::DescriptorSetTable
    {
        magma::descriptor::UniformBuffer normalMatrix = 0;
        magma::descriptor::UniformBuffer integrationParameters = 1;
        magma::descriptor::CombinedImageSampler volume = 2;
        magma::descriptor::CombinedImageSampler lookup = 3;
        MAGMA_REFLECT(normalMatrix, integrationParameters, volume, lookup)
    } setTable;

    std::shared_ptr<magma::ImageView> volume;
    std::shared_ptr<magma::ImageView> lookup;
    std::shared_ptr<magma::Sampler> nearestSampler;
    std::shared_ptr<magma::Sampler> trilinearSampler;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::shared_ptr<magma::UniformBuffer<IntegrationParameters>> uniformParameters;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    float power = 0.4f;

public:
    TextureVolumeApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("09 - Volume texture"), 512, 512)
    {
        initialize();
        loadTextures();
        createSampler();
        createUniformBuffers();
        setupDescriptorSet();
        setupPipeline();
        recordCommandBuffer(Buffer::Front);
        recordCommandBuffer(Buffer::Back);
    }

    void render(uint32_t bufferIndex) override
    {
        updateTransform();
        submitCommandBuffer(bufferIndex);
    }

    void onKeyDown(char key, int repeat, uint32_t flags) override
    {
        switch (key)
        {
        case AppKey::PgUp:
            if (power < 1.f)
            {
                power += 0.05f;
                updatePower();
            }
            break;
        case AppKey::PgDn:
            if (power > 0.1f)
            {
                power -= 0.05f;
                updatePower();
            }
            break;
        case AppKey::Space:
            break;
        }
        VulkanApp::onKeyDown(key, repeat, flags);
    }

    void updateTransform()
    {
        const rapid::matrix pitch = rapid::rotationX(rapid::radians(-spinY/2.f));
        const rapid::matrix yaw = rapid::rotationY(rapid::radians(spinX/2.f));
        const rapid::matrix world = pitch * yaw;

        magma::helpers::mapScoped(uniformBuffer,
            [this, &world](auto *normal)
            {
                *normal = rapid::transpose(rapid::inverse(world));
            });
    }

    void updatePower()
    {
        magma::helpers::mapScoped(uniformParameters,
            [this](auto *block)
            {
                block->power = power;
            });
        std::cout << "Power: " << power << "\n";
    }

    std::shared_ptr<magma::ImageView> loadVolumeTexture(const std::string& filename, uint32_t width, uint32_t height, uint32_t depth, std::shared_ptr<magma::SrcTransferBuffer> buffer)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            std::ifstream zipfile(filename + ".zip", std::ios::in | std::ios::binary | std::ios::ate);
            if (!zipfile.is_open())
                throw std::runtime_error("failed to open file \"" + filename + "\"");
            else
                throw std::runtime_error("unpack \"" + filename + ".zip\" before running sample");
        }
        const std::streamoff size = file.tellg();
        MAGMA_ASSERT(size == width * height * depth);
        file.seekg(0, std::ios::beg);
        VkDeviceSize bufferOffset = buffer->getPrivateData();
        magma::helpers::mapScopedRange<uint8_t>(buffer, bufferOffset, (VkDeviceSize)size,
            [&file, size](uint8_t *data)
            {   // Read data to buffer
                file.read(reinterpret_cast<char *>(data), size);
                file.close();
            });
        buffer->setPrivateData(bufferOffset + size);
        // Setup texture data description
        magma::Image::Mip volumeMip;
        volumeMip.extent = VkExtent3D{width, height, depth};
        volumeMip.bufferOffset = 0;
        const std::vector<magma::Image::Mip> mipMaps = {volumeMip};
        const magma::Image::CopyLayout bufferLayout{bufferOffset, 0, 0};
        // Upload volume data from buffer
        std::shared_ptr<magma::Image3D> image = std::make_shared<magma::Image3D>(cmdImageCopy, VK_FORMAT_R8_UNORM, std::move(buffer), mipMaps, bufferLayout);
        // Create image view for shader
        return std::make_shared<magma::ImageView>(std::move(image));
    }

    std::shared_ptr<magma::ImageView> loadTransferFunctionTexture(const std::string& filename, uint32_t width, std::shared_ptr<magma::SrcTransferBuffer> buffer)
    {
        std::ifstream file(filename, std::ifstream::in);
        if (!file.is_open())
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        const VkDeviceSize size = width * sizeof(uint32_t);
        VkDeviceSize bufferOffset = buffer->getPrivateData();
        magma::helpers::mapScopedRange<uint8_t>(buffer, bufferOffset, size,
            [&file, size](uint8_t *data)
            {
                memset(data, 0, size); // Clear first as not an entire buffer may be filled
                file.read(reinterpret_cast<char *>(data), size);
                file.close();
            });
        buffer->setPrivateData(bufferOffset + size);
        // Upload texture data from buffer
        magma::Image::Mip mip;
        mip.extent = {width, 1, 1};
        mip.bufferOffset = 0;
        const std::vector<magma::Image::Mip> mipMaps = {mip};
        const magma::Image::CopyLayout bufferLayout{bufferOffset, 0, 0};
        std::shared_ptr<magma::Image1D> image = std::make_shared<magma::Image1D>(cmdImageCopy, VK_FORMAT_R8G8B8A8_UNORM, std::move(buffer), mipMaps, bufferLayout);
        // Create image view for shader
        return std::make_shared<magma::ImageView>(std::move(image));
    }

    void loadTextures()
    {
        constexpr VkDeviceSize bufferSize = 16 * 1024 * 1024;
        auto buffer = std::make_shared<magma::SrcTransferBuffer>(device, bufferSize);
        cmdImageCopy->begin();
        {
            volume = loadVolumeTexture("head256.raw", 256, 256, 225, buffer);
            lookup = loadTransferFunctionTexture("tff.dat", 256, buffer);
        }
        cmdImageCopy->end();
        submitCopyImageCommands();
    }

    void createSampler()
    {
        nearestSampler = std::make_shared<magma::Sampler>(device, magma::sampler::magMinMipNearestClampToEdge);
        trilinearSampler = std::make_shared<magma::Sampler>(device, magma::sampler::magMinMipLinearClampToEdge);
    }

    void createUniformBuffers()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
        uniformParameters = std::make_shared<magma::UniformBuffer<IntegrationParameters>>(device);
        updatePower();
    }

    void setupDescriptorSet()
    {
        setTable.normalMatrix = uniformBuffer;
        setTable.integrationParameters = uniformParameters;
        setTable.volume = {volume, trilinearSampler};
        setTable.lookup = {lookup, nearestSampler};
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, shaderReflectionFactory, "raycast.o");
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "quad.o", "raycast.o",
            magma::renderstate::nullVertexInput,
            magma::renderstate::triangleStrip,
            magma::renderstate::fillCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clear::white});
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
                cmdBuffer->bindPipeline(graphicsPipeline);
                cmdBuffer->draw(4, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<TextureVolumeApp>(new TextureVolumeApp(entry));
}

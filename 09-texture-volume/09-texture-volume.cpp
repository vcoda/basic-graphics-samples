#include <fstream>
#include "../framework/vulkanApp.h"

// Use PgUp/PgDown to change accomodation power
class TextureVolumeApp : public VulkanApp
{
    struct alignas(16) IntegrationParameters
    {
        float power;
    };

    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer normalMatrix = 0;
        magma::descriptor::UniformBuffer integrationParameters = 1;
        magma::descriptor::CombinedImageSampler volume = 2;
        magma::descriptor::CombinedImageSampler lookup = 3;
    } setTable;

    std::unique_ptr<magma::ImageView> volume;
    std::unique_ptr<magma::ImageView> lookup;
    std::unique_ptr<magma::Sampler> nearestSampler;
    std::unique_ptr<magma::Sampler> trilinearSampler;
    std::unique_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::unique_ptr<magma::UniformBuffer<IntegrationParameters>> uniformParameters;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

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
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
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
        magma::map(uniformBuffer,
            [this, &world](auto *normal)
            {
                *normal = rapid::transpose(rapid::inverse(world));
            });
    }

    void updatePower()
    {
        magma::map(uniformParameters,
            [this](auto *block)
            {
                block->power = power;
            });
        std::cout << "Power: " << power << "\n";
    }

    std::unique_ptr<magma::ImageView> loadVolumeTexture(const std::string& filename, uint32_t width, uint32_t height, uint32_t depth, const std::unique_ptr<magma::SrcTransferBuffer>& buffer)
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
        magma::mapRange<uint8_t>(buffer, bufferOffset, (VkDeviceSize)size,
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
        const magma::Mipmap mipMap = {volumeMip};
        const magma::Image::CopyLayout bufferLayout{bufferOffset, 0, 0};
        // Upload volume data from buffer
        auto image = std::make_unique<magma::Image3D>(cmdImageCopy, VK_FORMAT_R8_UNORM, std::move(buffer), mipMap, bufferLayout);
        // Create image view for shader
        return std::make_unique<magma::UniqueImageView>(std::move(image));
    }

    std::unique_ptr<magma::ImageView> loadTransferFunctionTexture(const std::string& filename, uint32_t width, const std::unique_ptr<magma::SrcTransferBuffer>& buffer)
    {
        std::ifstream file(filename, std::ifstream::in);
        if (!file.is_open())
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        const VkDeviceSize size = width * sizeof(uint32_t);
        VkDeviceSize bufferOffset = buffer->getPrivateData();
        magma::mapRange<uint8_t>(buffer, bufferOffset, size,
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
        const magma::Mipmap mipMap = {mip};
        const magma::Image::CopyLayout bufferLayout{bufferOffset, 0, 0};
        auto image = std::make_unique<magma::Image1D>(cmdImageCopy, VK_FORMAT_R8G8B8A8_UNORM, std::move(buffer), mipMap, bufferLayout);
        // Create image view for shader
        return std::make_unique<magma::UniqueImageView>(std::move(image));
    }

    void loadTextures()
    {
        constexpr VkDeviceSize bufferSize = 16 * 1024 * 1024;
        auto buffer = std::make_unique<magma::SrcTransferBuffer>(device, bufferSize);
        cmdImageCopy->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {
            volume = loadVolumeTexture("head256.raw", 256, 256, 225, buffer);
            lookup = loadTransferFunctionTexture("tff.dat", 256, buffer);
        }
        cmdImageCopy->end();
        submitCopyImageCommands();
    }

    void createSampler()
    {
        nearestSampler = std::make_unique<magma::Sampler>(device, magma::sampler::magMinMipNearestClampToEdge);
        trilinearSampler = std::make_unique<magma::Sampler>(device, magma::sampler::magMinMipLinearClampToEdge);
    }

    void createUniformBuffers()
    {
        uniformBuffer = std::make_unique<magma::UniformBuffer<rapid::matrix>>(device);
        uniformParameters = std::make_unique<magma::UniformBuffer<IntegrationParameters>>(device);
        updatePower();
    }

    void setupDescriptorSet()
    {
        setTable.normalMatrix = uniformBuffer;
        setTable.integrationParameters = uniformParameters;
        setTable.volume = {volume, trilinearSampler};
        setTable.lookup = {lookup, nearestSampler};
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr, 0, shaderReflectionFactory, "raycast");
    }

    void setupPipeline()
    {
        auto layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "quad", "raycast",
            magma::renderstate::nullVertexInput,
            magma::renderstate::triangleStrip,
            magma::renderstate::fillCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthAlwaysDontWrite,
            magma::renderstate::dontBlendRgb,
            std::move(layout),
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        auto& cmdBuffer = commandBuffers[index];
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

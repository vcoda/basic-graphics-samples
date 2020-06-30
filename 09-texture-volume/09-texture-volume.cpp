#include <fstream>
#include "../framework/vulkanApp.h"

// Use PgUp/PgDown to change accomodation power
class TextureVolumeApp : public VulkanApp
{
    struct Texture
    {
        std::shared_ptr<magma::Image> image;
        std::shared_ptr<magma::ImageView> imageView;
    };

    struct alignas(16) IntegrationParameters
    {
        float power;
    };

    Texture volume;
    Texture lookup;
    std::shared_ptr<magma::Sampler> nearestSampler;
    std::shared_ptr<magma::Sampler> trilinearSampler;
    std::shared_ptr<magma::VertexBuffer> vertexBuffer;
    std::shared_ptr<magma::UniformBuffer<rapid::matrix>> uniformBuffer;
    std::shared_ptr<magma::UniformBuffer<IntegrationParameters>> uniformParameters;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::GraphicsPipeline> graphicsPipeline;

    float power = 0.4f;

public:
    TextureVolumeApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("09 - Volume texture"), 512, 512)
    {
        initialize();
        volume = loadVolumeTexture("head256.raw", 256, 256, 225);
        lookup = loadTransferFunctionTexture("tff.dat", 256);
        createSampler();
        createVertexBuffer();
        createUniformBuffers();
        setupDescriptorSet();
        setupPipeline();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        updateTransform();
        submitCommandBuffer(bufferIndex);
    }

    virtual void onKeyDown(char key, int repeat, uint32_t flags) override
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

        magma::helpers::mapScoped<rapid::matrix>(uniformBuffer,
            [this, &world](auto *normal)
            {
                *normal = rapid::transpose(rapid::inverse(world));
            });
    }

    void updatePower()
    {
        magma::helpers::mapScoped<IntegrationParameters>(uniformParameters,
            [this](auto *block)
            {
                block->power = power;
            });
        std::cout << "Power: " << power << "\n";
    }

    Texture loadVolumeTexture(const std::string& filename, uint32_t width, uint32_t height, uint32_t depth)
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
        std::shared_ptr<magma::SrcTransferBuffer> buffer = std::make_shared<magma::SrcTransferBuffer>(device, size);
        magma::helpers::mapScoped<uint8_t>(buffer,
            [&file, &size](uint8_t *data)
            {   // Read data to buffer
                file.read(reinterpret_cast<char *>(data), size);
            });
        // Upload volume data from buffer
        const VkExtent3D extent{width, height, depth};
        auto image = std::make_shared<magma::Image3D>(cmdImageCopy, VK_FORMAT_R8_UNORM, extent, std::move(buffer));
        // Create image view for shader
        auto imageView = std::make_shared<magma::ImageView>(image);
        return {image, imageView};
    }

    Texture loadTransferFunctionTexture(const std::string& filename, uint32_t width)
    {
        std::ifstream file(filename, std::ifstream::in);
        if (!file.is_open())
            throw std::runtime_error("failed to open file \"" + filename + "\"");
        std::shared_ptr<magma::SrcTransferBuffer> buffer = std::make_shared<magma::SrcTransferBuffer>(device, width * sizeof(uint32_t));
        magma::helpers::mapScoped<uint8_t>(buffer,
            [&file, width](uint8_t *data)
            {
                const std::size_t size = width * sizeof(uint32_t);
                memset(data, 0, size); // Clear first as not an entire buffer may be filled
                file.read(reinterpret_cast<char *>(data), size);
            });
        // Upload texture data from buffer
        magma::Image::MipmapLayout mipSizes{0};
        auto image = std::make_shared<magma::Image1D>(cmdImageCopy, VK_FORMAT_R8G8B8A8_UNORM, width, std::move(buffer), mipSizes);
        // Create image view for shader
        auto imageView = std::make_shared<magma::ImageView>(image);
        return {image, imageView};
    }

    void createSampler()
    {
        nearestSampler = std::make_shared<magma::Sampler>(device, magma::samplers::magMinMipNearestClampToEdge);
        trilinearSampler = std::make_shared<magma::Sampler>(device, magma::samplers::magMinMipLinearClampToEdge);
    }

    void createVertexBuffer()
    {
        const std::vector<rapid::float2> vertices = {
            {-1.f, -1.f}, {-1.f, 1.f}, {1.f, -1.f}, {1.f, 1.f}
        };
        vertexBuffer = std::make_shared<magma::VertexBuffer>(cmdBufferCopy, vertices);
    }

    void createUniformBuffers()
    {
        uniformBuffer = std::make_shared<magma::UniformBuffer<rapid::matrix>>(device);
        uniformParameters = std::make_shared<magma::UniformBuffer<IntegrationParameters>>(device);
        updatePower();
    }

    void setupDescriptorSet()
    {
        constexpr magma::Descriptor oneUniformBuffer = magma::descriptors::UniformBuffer(1);
        constexpr magma::Descriptor oneImageSampler = magma::descriptors::CombinedImageSampler(1);
        // Create descriptor pool
        constexpr uint32_t maxDescriptorSets = 1;
        descriptorPool = std::shared_ptr<magma::DescriptorPool>(new magma::DescriptorPool(device, maxDescriptorSets,
            {
                magma::descriptors::UniformBuffer(2),
                magma::descriptors::CombinedImageSampler(2)
            }));
        // Setup descriptor set layout
        descriptorSetLayout = std::shared_ptr<magma::DescriptorSetLayout>(new magma::DescriptorSetLayout(device,
            {
                magma::bindings::FragmentStageBinding(0, oneUniformBuffer),
                magma::bindings::FragmentStageBinding(1, oneUniformBuffer),
                magma::bindings::FragmentStageBinding(2, oneImageSampler),
                magma::bindings::FragmentStageBinding(3, oneImageSampler)
            }));
        // Allocate and update descriptor set
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, uniformBuffer);
        descriptorSet->update(1, uniformParameters);
        descriptorSet->update(2, volume.imageView, trilinearSampler);
        descriptorSet->update(3, lookup.imageView, nearestSampler);
    }

    void setupPipeline()
    {
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
        graphicsPipeline = std::make_shared<GraphicsPipeline>(device,
            "passthrough.o", "raycast.o",
            magma::renderstates::pos2f,
            magma::renderstates::triangleStrip,
            magma::renderstates::fillCullNoneCCW,
            magma::renderstates::dontMultisample,
            magma::renderstates::depthAlwaysDontWrite,
            magma::renderstates::dontBlendRgb,
            pipelineLayout,
            renderPass, 0,
            pipelineCache);
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clears::whiteColor});
            {
                cmdBuffer->setViewport(0, 0, width, height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, descriptorSet);
                cmdBuffer->bindPipeline(graphicsPipeline);
                cmdBuffer->bindVertexBuffer(0, vertexBuffer);
                cmdBuffer->draw(4, 0);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::make_unique<TextureVolumeApp>(entry);
}

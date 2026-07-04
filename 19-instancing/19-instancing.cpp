#include "../framework/vulkanApp.h"
#include "quadric/include/cube.h"

inline float random()
{
    return float(rand()) / float(RAND_MAX);
}

inline float random(float min, float max)
{
    return min + (max - min) * random();
}

class InstancingApp : public VulkanApp
{
    struct alignas(16) UniformBlock
    {
        rapid::matrix view;
        rapid::matrix viewProj;
    };

    struct DescriptorSetTable
    {
        magma::descriptor::UniformBuffer transforms = 0;
        magma::descriptor::StorageBuffer instanceTransforms = 1;
    } setTable;

    std::unique_ptr<quadric::Cube> mesh;
    std::unique_ptr<magma::UniformBuffer<UniformBlock>> uniformBuffer;
    std::unique_ptr<magma::StorageBuffer> instanceTransforms;
    std::unique_ptr<magma::DescriptorSet> descriptorSet;
    std::unique_ptr<magma::GraphicsPipeline> graphicsPipeline;

    rapid::matrix view;
    rapid::matrix proj;
    uint32_t instanceCount = 0;

public:
    InstancingApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("19 - Vulkan city"), 1280, 720, true)
    {
        initialize();
        setupView();
        createMesh();
        createUniformBuffer();
        instanceCount = buildVulkanCity();
        setupDescriptorSet();
        setupPipeline();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
        timer->run();
    }

    void render(uint32_t bufferIndex) override
    {
        updatePerspectiveTransform();
        submitCommandBuffer(bufferIndex);
    }

    void onResize(uint32_t width, uint32_t height) override
    {
        VulkanApp::onResize(width, height);
        setupView();
        for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
            recordCommandBuffer(i);
    }

    void setupView()
    {
        const rapid::vector3 eye(0.f, 130.f, 520.f);
        const rapid::vector3 center(0.f, 30.f, 0.f);
        const rapid::vector3 up(0.f, 1.f, 0.f);
        constexpr float fov = rapid::radians(20.f);
        const float aspect = width/(float)height;
        constexpr float zn = 1.f, zf = 1000.f;
        view = rapid::lookAtRH(eye, center, up);
        proj = rapid::perspectiveFovRH(fov, aspect, zn, zf);
    }

    void updatePerspectiveTransform()
    {
        constexpr float speed = 0.01f;
        static float angle = 0.f;
        angle += timer->millisecondsElapsed() * speed;
        const rapid::matrix rotation = rapid::rotationY(rapid::radians(angle));
        magma::map(uniformBuffer,
            [this, &rotation](auto *block)
            {
                block->view = rotation * view;
                block->viewProj = block->view * proj;
            });
    }

    void createMesh()
    {
        mesh = std::make_unique<quadric::Cube>(cmdBufferCopy);
    }

    void createUniformBuffer()
    {
        uniformBuffer = std::make_unique<magma::UniformBuffer<UniformBlock>>(device);
    }

    uint32_t buildVulkanCity()
    {
        constexpr uint32_t GridX = 100;
        constexpr uint32_t GridZ = 100;
        constexpr float CityRadius = std::min(GridX, GridZ) * 0.48f;
        constexpr float DowntownRadius = 10.0f;
        constexpr float CellSize = 3.f;
        constexpr float HouseMin = .5f;
        constexpr float HouseMax = 2.f;
        constexpr float cx = (GridX - 1) * .5f;
        constexpr float cz = (GridZ - 1) * .5f;

        const float maxDist = std::sqrt(cx * cx + cz * cz);
        std::vector<rapid::matrix> transforms;

        for (uint32_t z = 0; z < GridZ; ++z)
        {
            for (uint32_t x = 0; x < GridX; ++x)
            {   // Streets, unused places
                if ((x % 15) == 0 || (z % 25) == 0)
                    continue;
                if (random() < 0.2f)
                    continue;

                float dx = x - cx;
                float dz = z - cz;
                float d = std::sqrt(dx * dx + dz * dz);

                // Non-uniform city limits
                float angle = std::atan2(dz, dx);
                float nonUniformRadius = CityRadius * (1.f
                    + 0.18f * std::sin(angle * 2.f + 1.4f)
                    + 0.10f * std::sin(angle * 5.f + 3.2f)
                    + 0.06f * std::sin(angle * 11.f + 0.7f));
                nonUniformRadius += random(-2.f, 2.f);
                if (d > nonUniformRadius)
                    continue;

                // Gaussian falloff
                float t = std::exp(-(d * d) / (2.f * DowntownRadius * DowntownRadius));
                float height;
                float h = random();
                if (h < t * 0.2f)
                    height = random(10.f, 40.f); // Skyscrapers
                else
                    height = random(1.f, 8.f);

                // Building bounds
                const float sx = random(HouseMin, HouseMax);
                const float sz = random(HouseMin, HouseMax);
                float mx = (CellSize - sx) * 0.5f;
                float mz = (CellSize - sz) * 0.5f;
                // Random offset
                float ox = dx * CellSize + random(-mx, mx);
                float oz = dz * CellSize + random(-mz, mz);

                // Calculate building's transform
                rapid::matrix scale = rapid::scaling(sx, height * .5f, sz);
                rapid::matrix translation = rapid::translation(ox, height * .5f, oz);
                rapid::matrix world = scale * translation;
                transforms.push_back(world);
            }
        }

        const VkDeviceSize bufferSize = transforms.size() * sizeof(rapid::matrix);
        instanceTransforms = std::make_unique<magma::StorageBuffer>(cmdBufferCopy, bufferSize, transforms.data());
        return (uint32_t)transforms.size();
    }

    void setupDescriptorSet()
    {
        setTable.transforms = uniformBuffer;
        setTable.instanceTransforms = instanceTransforms;
        descriptorSet = std::make_unique<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, 0, shaderReflectionFactory, "building");
    }

    void setupPipeline()
    {
        auto layout = std::make_unique<magma::PipelineLayout>(descriptorSet->getLayout());
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device,
            "building", "diffuse",
            mesh->getVertexInput(),
            magma::renderstate::triangleList,
            negateViewport ? magma::renderstate::fillCullBackCcw
                           : magma::renderstate::fillCullBackCw,
            magma::renderstate::dontMultisample,
            magma::renderstate::depthLessOrEqual,
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
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index],
                {
                    magma::ClearColor(0.35f, 0.53f, 0.7f, 1.f),
                    magma::clear::depthOne
                });
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -int32_t(height) : height);
                cmdBuffer->setScissor(0, 0, width, height);
                cmdBuffer->bindDescriptorSet(graphicsPipeline, 0, descriptorSet);
                cmdBuffer->bindPipeline(graphicsPipeline);
                mesh->drawInstanced(cmdBuffer, instanceCount);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<InstancingApp>(new InstancingApp(entry));
}

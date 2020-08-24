#include <iomanip>
#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

class ComputeApp : public VulkanApp
{
    const std::vector<float> numbers = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};

    std::shared_ptr<magma::StorageBuffer> inputBuffers[2];
    std::shared_ptr<magma::StorageBuffer> outputBuffer;
    std::shared_ptr<magma::DstTransferBuffer> readbackBuffer;
    std::shared_ptr<magma::DescriptorPool> descriptorPool;
    std::shared_ptr<magma::DescriptorSetLayout> descriptorSetLayout;
    std::shared_ptr<magma::DescriptorSet> descriptorSet;
    std::shared_ptr<magma::PipelineLayout> pipelineLayout;
    std::shared_ptr<magma::ComputePipeline> computeSum;
    std::shared_ptr<magma::ComputePipeline> computeMul;
    std::shared_ptr<magma::ComputePipeline> computePower;
    std::shared_ptr<magma::Fence> fence;

public:
    ComputeApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("18 - Compute shader"), 512, 512)
    {
        initialize();
        createInputOutputBuffers();
        setupDescriptorSet();
        computeSum = createComputePipeline("sum.o", "sum");
        computeMul = createComputePipeline("mul.o", "mul");
        computePower = createComputePipeline("power.o", "power");
        printInputValues(numbers, "a");
        printInputValues(numbers, "b");
        compute(computeSum, "a + b");
        compute(computeMul, "a * b");
        compute(computePower, "2^a");
        close();
    }

    virtual void createLogicalDevice() override
    {
        const std::vector<float> priority = {1.0f};
        const std::vector<magma::DeviceQueueDescriptor> queueDescriptors = {
            magma::DeviceQueueDescriptor(physicalDevice, VK_QUEUE_COMPUTE_BIT, priority),
            magma::DeviceQueueDescriptor(physicalDevice, VK_QUEUE_TRANSFER_BIT, priority),
        };
        const std::vector<const char*> noLayers;
        std::vector<const char*> noExtensions;
        VkPhysicalDeviceFeatures noFeatures = {0};
        device = physicalDevice->createDevice(queueDescriptors, noLayers, noExtensions, noFeatures);
    }

    virtual void createCommandBuffers() override
    {
        queue = device->getQueue(VK_QUEUE_COMPUTE_BIT, 0);
        commandPools[0] = std::make_shared<magma::CommandPool>(device, queue->getFamilyIndex());
        commandBuffers = commandPools[0]->allocateCommandBuffers(1, true);

        std::shared_ptr<magma::Queue> transferQueue = device->getQueue(VK_QUEUE_TRANSFER_BIT, 0);
        commandPools[1] = std::make_shared<magma::CommandPool>(device, transferQueue->getFamilyIndex());
        cmdBufferCopy = std::make_shared<magma::PrimaryCommandBuffer>(commandPools[1]);

        fence = std::make_shared<magma::Fence>(device, true);
    }

    void createInputOutputBuffers()
    {
        inputBuffers[0] = std::make_shared<magma::StorageBuffer>(cmdBufferCopy, numbers);
        inputBuffers[1] = std::make_shared<magma::StorageBuffer>(cmdBufferCopy, numbers);
        const VkDeviceSize outputSize = static_cast<VkDeviceSize>(sizeof(float) * numbers.size());
        outputBuffer = std::make_shared<magma::StorageBuffer>(cmdBufferCopy, nullptr, outputSize);
        readbackBuffer = std::make_shared<magma::DstTransferBuffer>(device, sizeof(float) * numbers.size());
    }

    void setupDescriptorSet()
    {
        descriptorPool = std::make_shared<magma::DescriptorPool>(device, 1, magma::descriptors::StorageBuffer(3));
        constexpr magma::Descriptor oneStorageBuffer = magma::descriptors::StorageBuffer(1);
        descriptorSetLayout = std::shared_ptr<magma::DescriptorSetLayout>(new magma::DescriptorSetLayout(device,
            {
                magma::bindings::ComputeStageBinding(0, oneStorageBuffer),
                magma::bindings::ComputeStageBinding(1, oneStorageBuffer),
                magma::bindings::ComputeStageBinding(2, oneStorageBuffer),
            }));
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, inputBuffers[0]);
        descriptorSet->update(1, inputBuffers[1]);
        descriptorSet->update(2, outputBuffer);
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSetLayout);
    }

    std::shared_ptr<magma::ComputePipeline> createComputePipeline(const char *filename, const char *entrypoint) const
    {
        const aligned_vector<char> bytecode = utilities::loadBinaryFile(filename);
        auto computeShader = std::make_shared<magma::ShaderModule>(device, (const magma::SpirvWord *)bytecode.data(), bytecode.size());
        return std::make_shared<magma::ComputePipeline>(device,
            magma::ComputeShaderStage(computeShader, entrypoint),
            pipelineLayout, pipelineCache);
    }

    void compute(std::shared_ptr<magma::ComputePipeline> pipeline, const char *description)
    {   // Record command buffer
        std::shared_ptr<magma::CommandBuffer> computeCmdBuffer = commandBuffers[0];
        computeCmdBuffer->begin();
        {   // Ensure that transfer write is finished before compute shader execution
            const std::vector<magma::BufferMemoryBarrier> bufferMemoryBarriers = {
                {inputBuffers[0], magma::barriers::transferWriteShaderRead},
                {inputBuffers[0], magma::barriers::transferWriteShaderRead},
            };
            computeCmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                {}, bufferMemoryBarriers, {});
            // Bind input and output buffers
            computeCmdBuffer->bindDescriptorSet(pipeline, descriptorSet);
            // Bind pipeline
            computeCmdBuffer->bindPipeline(pipeline);
            // Run compute shader
            const uint32_t workgroups = static_cast<uint32_t>(outputBuffer->getMemory()->getSize()/sizeof(float));
            computeCmdBuffer->dispatch(workgroups, 1, 1);
            // Ensure that shader writes are finished before transfer readback
            computeCmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                magma::BufferMemoryBarrier(outputBuffer, magma::barriers::shaderWriteTransferRead));
            // Copy output local buffer to readback buffer
            computeCmdBuffer->copyBuffer(outputBuffer, readbackBuffer);
            /* The memory dependency defined by signaling a fence and waiting on the host
               does not guarantee that the results of memory accesses will be visible
               to the host, or that the memory is available. To provide that guarantee,
               the application must insert a memory barrier between the device writes
               and the end of the submission that will signal the fence,
               to guarantee completion of the writes. */
            computeCmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                magma::BufferMemoryBarrier(readbackBuffer, magma::barriers::transferWriteHostRead));
        }
        computeCmdBuffer->end();
        // Execute
        fence->reset();
        queue->submit(computeCmdBuffer, 0, nullptr, nullptr, fence);
        fence->wait();
        // Output computed values
        printOutputValues(description);
    }

    void printInputValues(const std::vector<float>& values, const char *description)
    {
        std::cout << std::setw(6) << std::left << description << ": ";
        for (const auto val : values)
            std::cout << std::setw(4) << std::right << val << ", ";
        std::cout << std::endl;
    }

    void printOutputValues(const char *description)
    {
        magma::helpers::mapScoped<float>(readbackBuffer, [&](float *values)
        {
            std::cout << std::setw(6) << std::left << description << ": ";
            const uint32_t count = static_cast<uint32_t>(readbackBuffer->getMemory()->getSize()/sizeof(float));
            for (uint32_t i = 0; i < count; ++i)
                std::cout << std::setw(4) << std::right << values[i] << ", ";
            std::cout << std::endl;
        });
    }

    // This stuff not used in compute application
    virtual void createSwapchain(bool vSync) override {}
    virtual void createRenderPass() override {}
    virtual void createFramebuffer() override {}
    virtual void createSyncPrimitives() override {}
    virtual void render(uint32_t bufferIndex) override {}
    virtual void onPaint() override {}
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<ComputeApp>(new ComputeApp(entry));
}

#include <iomanip>
#include "../framework/vulkanApp.h"

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
    std::shared_ptr<magma::CommandBuffer> computeCmdBuffer;
    std::shared_ptr<magma::Fence> fence;

public:
    ComputeApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("15 - Compute shader"), 512, 512)
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
            magma::DeviceQueueDescriptor(VK_QUEUE_COMPUTE_BIT, physicalDevice, priority),
            magma::DeviceQueueDescriptor(VK_QUEUE_TRANSFER_BIT, physicalDevice, priority),
        };
        const std::vector<const char*> noLayers;
        std::vector<const char*> noExtensions;
        VkPhysicalDeviceFeatures noFeatures = {0};
        device = physicalDevice->createDevice(queueDescriptors, noLayers, noExtensions, noFeatures);
    }

    virtual void createCommandBuffers() override
    {
        queue = device->getQueue(VK_QUEUE_COMPUTE_BIT, 0);
        commandPools[0].reset(new magma::CommandPool(device, queue->getFamilyIndex()));
        computeCmdBuffer = commandPools[0]->allocateCommandBuffer(true);

        std::shared_ptr<magma::Queue> transferQueue = device->getQueue(VK_QUEUE_TRANSFER_BIT, 0);
        commandPools[1].reset(new magma::CommandPool(device, transferQueue->getFamilyIndex()));
        cmdBufferCopy = commandPools[1]->allocateCommandBuffer(true);

        fence.reset(new magma::Fence(device, true));
    }

    void createInputOutputBuffers()
    {
        inputBuffers[0].reset(new magma::StorageBuffer(cmdBufferCopy, numbers));
        inputBuffers[1].reset(new magma::StorageBuffer(cmdBufferCopy, numbers));
        const VkDeviceSize size = sizeof(float) * numbers.size();
        outputBuffer.reset(new magma::StorageBuffer(device, size));
        readbackBuffer.reset(new magma::DstTransferBuffer(device, size));
    }

    void setupDescriptorSet()
    {
        const uint32_t maxDescriptorSets = 1;
        descriptorPool.reset(new magma::DescriptorPool(device, maxDescriptorSets, {
            magma::descriptors::StorageBuffer(3),
        }));
        const magma::Descriptor storageBufferDesc = magma::descriptors::StorageBuffer(1);
        descriptorSetLayout.reset(new magma::DescriptorSetLayout(device, {
            magma::bindings::ComputeStageBinding(0, storageBufferDesc),
            magma::bindings::ComputeStageBinding(1, storageBufferDesc),
            magma::bindings::ComputeStageBinding(2, storageBufferDesc),
        }));
        descriptorSet = descriptorPool->allocateDescriptorSet(descriptorSetLayout);
        descriptorSet->update(0, inputBuffers[0]);
        descriptorSet->update(1, inputBuffers[1]);
        descriptorSet->update(2, outputBuffer);
        pipelineLayout.reset(new magma::PipelineLayout(descriptorSetLayout));
    }

    std::shared_ptr<magma::ComputePipeline> createComputePipeline(const char *filename, const char *entrypoint)
    {
        return std::shared_ptr<magma::ComputePipeline>(new magma::ComputePipeline(device,
            pipelineCache,
            utilities::loadShader<magma::ComputeShaderStage>(device, filename, entrypoint),
            pipelineLayout));
    }

    void compute(std::shared_ptr<magma::ComputePipeline> pipeline, const char *description)
    {
        // Record command buffer
        computeCmdBuffer->begin();
        {
            // Ensure that transfer write is finished before compute shader execution
            computeCmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                {inputBuffers[0], inputBuffers[1]}, magma::barriers::transferWriteShaderRead);
            // Bind input and output buffers
            computeCmdBuffer->bindDescriptorSet(pipelineLayout, descriptorSet, VK_PIPELINE_BIND_POINT_COMPUTE);
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
    return std::unique_ptr<IApplication>(new ComputeApp(entry));
}

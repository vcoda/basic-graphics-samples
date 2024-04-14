#include <iomanip>
#include "../framework/vulkanApp.h"
#include "../framework/utilities.h"

class ComputeApp : public VulkanApp
{
    const std::vector<float> numbers = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};

    struct DescriptorSetTable : magma::DescriptorSetTable
    {
        magma::descriptor::StorageBuffer inputBuffer0 = 0;
        magma::descriptor::StorageBuffer inputBuffer1 = 1;
        magma::descriptor::StorageBuffer outputBuffer = 2;
        MAGMA_REFLECT(inputBuffer0, inputBuffer1, outputBuffer)
    } setTable;

    std::shared_ptr<magma::StorageBuffer> inputBuffers[2];
    std::shared_ptr<magma::StorageBuffer> outputBuffer;
    std::shared_ptr<magma::DstTransferBuffer> readbackBuffer;
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

    void createLogicalDevice() override
    {
        const std::vector<float> priority = {1.f};
        const std::vector<magma::DeviceQueueDescriptor> queueDescriptors = {
            magma::DeviceQueueDescriptor(physicalDevice, VK_QUEUE_COMPUTE_BIT, priority),
            magma::DeviceQueueDescriptor(physicalDevice, VK_QUEUE_TRANSFER_BIT, priority),
        };
        const std::vector<const char*> noLayers;
        std::vector<const char*> noExtensions;
        VkPhysicalDeviceFeatures noFeatures = {0};
        device = physicalDevice->createDevice(queueDescriptors, noLayers, noExtensions, noFeatures);
    }

    void createCommandBuffers() override
    {
        graphicsQueue = device->getQueue(VK_QUEUE_COMPUTE_BIT, 0);
        commandPools[0] = std::make_shared<magma::CommandPool>(device, graphicsQueue->getFamilyIndex());
        commandBuffers = commandPools[0]->allocateCommandBuffers(1, true);
        transferQueue = device->getQueue(VK_QUEUE_TRANSFER_BIT, 0);
        commandPools[1] = std::make_shared<magma::CommandPool>(device, transferQueue->getFamilyIndex());
        cmdBufferCopy = std::make_shared<magma::PrimaryCommandBuffer>(commandPools[1]);
        fence = std::make_shared<magma::Fence>(device, nullptr, true);
    }

    void createInputOutputBuffers()
    {
        const VkDeviceSize bufferSize = static_cast<VkDeviceSize>(numbers.size() * sizeof(float));
        inputBuffers[0] = std::make_shared<magma::StorageBuffer>(cmdBufferCopy, bufferSize, numbers.data());
        inputBuffers[1] = std::make_shared<magma::StorageBuffer>(cmdBufferCopy, bufferSize, numbers.data());
        outputBuffer = std::make_shared<magma::StorageBuffer>(cmdBufferCopy, bufferSize, nullptr);
        readbackBuffer = std::make_shared<magma::DstTransferBuffer>(device, bufferSize);
    }

    void setupDescriptorSet()
    {
        setTable.inputBuffer0 = inputBuffers[0];
        setTable.inputBuffer1 = inputBuffers[1];
        setTable.outputBuffer = outputBuffer;
        descriptorSet = std::make_shared<magma::DescriptorSet>(descriptorPool,
            setTable, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr, shaderReflectionFactory, "sum.o");
        pipelineLayout = std::make_shared<magma::PipelineLayout>(descriptorSet->getLayout());
    }

    std::shared_ptr<magma::ComputePipeline> createComputePipeline(const char *filename, const char *entrypoint) const
    {
        const aligned_vector<char> bytecode = utilities::loadBinaryFile(filename);
        auto computeShader = std::make_shared<magma::ShaderModule>(device, (const magma::SpirvWord *)bytecode.data(), bytecode.size());
        return std::make_shared<magma::ComputePipeline>(device,
            magma::ComputeShaderStage(computeShader, entrypoint),
            pipelineLayout, nullptr, pipelineCache);
    }

    void compute(std::shared_ptr<magma::ComputePipeline> pipeline, const char *description)
    {   // Record command buffer
        std::shared_ptr<magma::CommandBuffer> computeCmdBuffer = commandBuffers[0];
        computeCmdBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        {   // Ensure that transfer write is finished before compute shader execution
            const std::vector<magma::BufferMemoryBarrier> bufferMemoryBarriers = {
                {inputBuffers[0], magma::barrier::transferWriteShaderRead},
                {inputBuffers[0], magma::barrier::transferWriteShaderRead},
            };
            computeCmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                {}, bufferMemoryBarriers, {});
            // Bind input and output buffers
            computeCmdBuffer->bindDescriptorSet(pipeline, 0, descriptorSet);
            // Bind pipeline
            computeCmdBuffer->bindPipeline(pipeline);
            // Run compute shader
            const uint32_t workgroups = static_cast<uint32_t>(outputBuffer->getMemory()->getSize()/sizeof(float));
            computeCmdBuffer->dispatch(workgroups, 1, 1);
            // Ensure that shader writes are finished before transfer readback
            computeCmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                magma::BufferMemoryBarrier(outputBuffer, magma::barrier::shaderWriteTransferRead));
            // Copy output local buffer to readback buffer
            computeCmdBuffer->copyBuffer(outputBuffer, readbackBuffer);
            /* The memory dependency defined by signaling a fence and waiting on the host
               does not guarantee that the results of memory accesses will be visible
               to the host, or that the memory is available. To provide that guarantee,
               the application must insert a memory barrier between the device writes
               and the end of the submission that will signal the fence,
               to guarantee completion of the writes. */
            computeCmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                magma::BufferMemoryBarrier(readbackBuffer, magma::barrier::transferWriteHostRead));
        }
        computeCmdBuffer->end();
        // Execute
        fence->reset();
        graphicsQueue->submit(computeCmdBuffer, 0, nullptr, nullptr, fence);
        fence->wait();
        computeCmdBuffer->reset(false); // Reset and record again between each submission
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
        magma::helpers::mapScoped<float>(readbackBuffer,
            [&](float *values)
            {
                std::cout << std::setw(6) << std::left << description << ": ";
                const uint32_t count = static_cast<uint32_t>(readbackBuffer->getMemory()->getSize()/sizeof(float));
                for (uint32_t i = 0; i < count; ++i)
                    std::cout << std::setw(4) << std::right << values[i] << ", ";
                std::cout << std::endl;
            });
    }

    // This stuff not used in compute application
    void createSwapchain() override {}
    void createRenderPass() override {}
    void createFramebuffer() override {}
    void createSyncPrimitives() override {}
    void render(uint32_t bufferIndex) override {}
    void onPaint() override {}
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<ComputeApp>(new ComputeApp(entry));
}

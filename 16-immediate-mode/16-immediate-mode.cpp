#include "../framework/vulkanApp.h"

class ImmediateModeApp : public VulkanApp
{
    std::unique_ptr<magma::aux::ImmediateRender> ir;

public:
    ImmediateModeApp(const AppEntry& entry):
        VulkanApp(entry, TEXT("16 - Immediate mode"), 512, 512)
    {
        initialize();
        createImmediateRender();
        drawPoints();
        drawLines();
        drawLineStrip();
        drawLineLoop();
        drawPolygon();
        drawQuads();
        drawQuadStrip();
        drawTriangles();
        drawTriangleStrip();
        drawTriangleFan();
        recordCommandBuffer(FrontBuffer);
        recordCommandBuffer(BackBuffer);
        ir->reset();
    }

    void createImmediateRender()
    {
        constexpr uint32_t maxVertexCount = 1024;
        ir = std::make_unique<magma::aux::ImmediateRender>(maxVertexCount, device, pipelineCache, nullptr, renderPass);
        ir->setLineWidth(2.f);
    }

    virtual void createLogicalDevice() override
    {
        const magma::DeviceQueueDescriptor graphicsQueue(physicalDevice, VK_QUEUE_GRAPHICS_BIT, {1.f});

        VkPhysicalDeviceFeatures features = {VK_FALSE};
        features.fillModeNonSolid = VK_TRUE;
        features.wideLines = VK_TRUE;

        std::vector<const char*> enabledExtensions;
        enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        if (extensions->AMD_negative_viewport_height)
            enabledExtensions.push_back(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
        else if (extensions->KHR_maintenance1)
            enabledExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

        device = physicalDevice->createDevice({graphicsQueue}, {}, enabledExtensions, features);
    }

    virtual void render(uint32_t bufferIndex) override
    {
        submitCommandBuffer(bufferIndex);
    }

    void drawPoints()
    {
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        {
            ir->color(1.f, 0.f, 0.f);
            ir->pointSize(3.0f);
            ir->vertex(-0.937f, 0.830f);
            ir->color(1.f, 1.f, 0.f);
            ir->pointSize(4.0f);
            ir->vertex(-0.828f, 0.945f);
            ir->color(0.f, 1.f, 0.f);
            ir->pointSize(3.0f);
            ir->vertex(-0.844f, 0.734f);
            ir->color(0.f, 0.f, 1.f);
            ir->pointSize(4.0f);
            ir->vertex(-0.648f, 0.797f);
            ir->color(0.f, 0.f, 0.f);
            ir->pointSize(3.0f);
            ir->vertex(-0.621f, 0.941f);
        }
        ir->endPrimitive();
    }

    void drawLines()
    {
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        {
            ir->color(1.f, 0.f, 0.f);
            ir->vertex(-0.332f, 0.961f);
            ir->vertex(-0.07f, 0.711f);
            ir->color(0.f, 0.f, 1.f);
            ir->vertex(-0.289f, 0.767f);
            ir->vertex(0.047f, 0.953f);
            ir->color(1.f, 1.f, 1.f);
            ir->vertex(0.039f, 0.781f);
            ir->vertex(0.109f, 0.906f);
        }
        ir->endPrimitive();
    }

    void drawLineStrip()
    {
        ir->color(1.f, 1.f, 0.f);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
        {
            ir->vertex(0.266f, 0.754f);
            ir->vertex(0.379f, 0.953f);
            ir->vertex(0.473f, 0.777f);
            ir->vertex(0.637f, 0.949f);
            ir->vertex(0.813f, 0.785f);
            ir->vertex(0.488f, 0.929f);
            ir->vertex(0.598f, 0.773f);
        }
        ir->endPrimitive();
    }

    void drawLineLoop()
    {
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
        {
            ir->color(1.f, 0.f, 0.f);
            ir->vertex(-0.875f, 0.359f);
            ir->color(1.f, 1.f, 0.f);
            ir->vertex(-0.879f, 0.512f);
            ir->color(0.f, 1.f, 0.f);
            ir->vertex(-0.589f, 0.480f);
            ir->color(0.f, 0.f, 1.f);
            ir->vertex(-0.453f, 0.379f);
            ir->color(0.f, 1.f, 1.f);
            ir->vertex(-0.695f, 0.234f);
            ir->color(1.f, 0.f, 1.f);
            ir->vertex(-0.719f, 0.398f);
        }
        ir->endPrimitive(true);
    }

    void drawPolygon()
    {   // Polygons are not present in Vulkan, have to be emulated by triangle fan
        ir->setRasterizationState(negateViewport ? magma::renderstates::fillCullBackCW
                                                 : magma::renderstates::fillCullBackCCW);
        ir->color(0.f, 0.5f, 0.5f);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
        {
            ir->vertex(-0.277, 0.277f);
            ir->vertex(-0.211f, 0.477f);
            ir->vertex(-0.063f, 0.477f);
            ir->vertex(0.094f, 0.277f);
            ir->vertex(-0.141f, 0.204f);
        }
        ir->endPrimitive();
    }

    void drawQuads()
    {   // Quads are not present in Vulkan, have to be emulated by triangle strips
        ir->setRasterizationState(negateViewport ? magma::renderstates::lineCullBackCCW
                                                 : magma::renderstates::lineCullBackCW);
        ir->color(1.f, 0.f, 0.f);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        {
            ir->vertex(0.301f, 0.476f);
            ir->vertex(0.438f, 0.273f);
            ir->vertex(0.434f, 0.543f);
            ir->vertex(0.586f, 0.406f);
        }
        ir->endPrimitive();
        ir->color(0.f, 1.f, 0.f);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        {
            ir->vertex(0.66f, 0.27f);
            ir->vertex(0.878f, 0.27f);
            ir->vertex(0.66f, 0.476f);
            ir->vertex(0.949f, 0.476f);
        }
        ir->endPrimitive();
    }

    void drawQuadStrip()
    {   // Quad strip not present in Vulkan, have to be emulated by triangle strip
        ir->setRasterizationState(negateViewport ? magma::renderstates::lineCullBackCW
                                                 : magma::renderstates::lineCullBackCCW);
        ir->color(0.f, 0.f, 0.5f);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        {
            ir->vertex(-0.746f, -0.203f);
            ir->vertex(-0.746f, -0.066f);
            ir->vertex(-0.613f, -0.203f);
            ir->vertex(-0.609f, 0.f);
            ir->vertex(-0.472f, -0.254f);
            ir->vertex(-0.398f, 0.f);
            ir->vertex(-0.265f, -0.2f);
            ir->vertex(-0.265f, -0.129f);
        }
        ir->endPrimitive();
    }

    void drawTriangles()
    {
        ir->setRasterizationState(negateViewport ? magma::renderstates::fillCullBackCCW
                                                 : magma::renderstates::fillCullBackCW);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        {
            ir->color(1.f, 0.f, 0.f);
            ir->vertex(0.227f, 0.f);
            ir->color(0.f, 1.f, 0.f);
            ir->vertex(0.3f, -0.269f);
            ir->color(0.f, 0.f, 1.f);
            ir->vertex(0.441f, -0.07f);

            ir->color(0.5f, 0.f, 0.f);
            ir->vertex(0.441f, -0.203f);
            ir->vertex(0.730f, -0.203f);
            ir->vertex(0.586f, 0.004f);
        }
        ir->endPrimitive();
        ir->setRasterizationState(magma::renderstates::lineCullBackCCW);
        ir->color(0.f, 0.f, 1.f);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        {
            ir->vertex(0.73f, -0.275f);
            ir->vertex(0.949f, -0.275f);
            ir->vertex(0.875f, 0.f);
        }
        ir->endPrimitive();
    }

    void drawTriangleStrip()
    {
        ir->setRasterizationState(negateViewport ? magma::renderstates::lineCullBackCW
                                                 : magma::renderstates::lineCullBackCCW);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        {
            ir->color(1.f, 0.f, 0.f);
            ir->vertex(-0.938f, -0.75f);
            ir->vertex(-0.793f, -0.48f);
            ir->vertex(-0.645f, -0.746f);
            ir->color(0.f, 0.f, 1.f);
            ir->vertex(-0.43f, -0.54f);
            ir->vertex(-0.211f, -0.813f);
            ir->vertex(-0.137f, -0.54f);
        }
        ir->endPrimitive();
    }

    void drawTriangleFan()
    {
        ir->setLineWidth(3.f);
        ir->setRasterizationState(negateViewport ? magma::renderstates::lineCullBackCW
                                                 : magma::renderstates::lineCullBackCCW);
        ir->color(0.f, 0.f, 0.f);
        ir->beginPrimitive(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
        {
            ir->vertex(0.586f, -0.75f);
            ir->vertex(0.297f, -0.676f);
            ir->vertex(0.375f, -0.609f);
            ir->vertex(0.586f, -0.543f);
            ir->vertex(0.805f, -0.539f);
            ir->vertex(0.949f, -0.75f);
        }
        ir->endPrimitive();
    }

    void recordCommandBuffer(uint32_t index)
    {
        std::shared_ptr<magma::CommandBuffer> cmdBuffer = commandBuffers[index];
        cmdBuffer->begin();
        {
            cmdBuffer->beginRenderPass(renderPass, framebuffers[index], {magma::clears::grayColor});
            {
                cmdBuffer->setViewport(0, 0, width, negateViewport ? -height : height);
                cmdBuffer->setScissor(0, 0, width, height);
                ir->commitPrimitives(cmdBuffer, false);
            }
            cmdBuffer->endRenderPass();
        }
        cmdBuffer->end();
    }
};

std::unique_ptr<IApplication> appFactory(const AppEntry& entry)
{
    return std::unique_ptr<ImmediateModeApp>(new ImmediateModeApp(entry));
}

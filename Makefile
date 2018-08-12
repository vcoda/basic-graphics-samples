CC=g++
GLSLC=$(VULKAN_SDK)/bin/glslangValidator

PLATFORM=VK_USE_PLATFORM_XCB_KHR
INCLUDE_DIR=-I$(VULKAN_SDK)/include -Ithird-party -Ithird-party/rapid
BUILD=build

BASE_CFLAGS=-std=c++14 -MD -D$(PLATFORM) $(INCLUDE_DIR)
DEBUG_CFLAGS=$(BASE_CFLAGS) -g -D_DEBUG
RELEASE_CFLAGS=$(BASE_CFLAGS)
LDFLAGS=-L$(VULKAN_SDK)/lib -lxcb -lvulkan

# Sample app dir/exe names

TARGET01=01-clear
TARGET02=02-simple-triangle
TARGET03=03-vertex-buffer
TARGET04=04-vertex-transform
TARGET05=05-mesh
TARGET06=06-texture
TARGET07=07-texture-array
TARGET08=08-cubemap
TARGET09=09-alpha-blend
TARGET10=10-render-to-texture
TARGET11=11-occlusion-query
TARGET12=12-pushconstants
TARGET13=13-specialization
TARGET14=14-particles
TARGET15=15-compute
TARGET16=16-immediate

# Magma object files

MAGMA=/third-party/magma
MAGMA_OBJS= \
	$(BUILD)$(MAGMA)/allocator/allocator.o \
	\
	$(BUILD)$(MAGMA)/barriers/bufferMemoryBarrier.o \
	$(BUILD)$(MAGMA)/barriers/globalMemoryBarrier.o \
	$(BUILD)$(MAGMA)/barriers/imageMemoryBarrier.o \
	\
	$(BUILD)$(MAGMA)/descriptors/attachment.o \
	$(BUILD)$(MAGMA)/descriptors/subpass.o \
	\
	$(BUILD)$(MAGMA)/misc/clearValue.o \
	$(BUILD)$(MAGMA)/misc/deviceQueueDesc.o \
	$(BUILD)$(MAGMA)/misc/exception.o \
	$(BUILD)$(MAGMA)/misc/extensions.o \
	$(BUILD)$(MAGMA)/misc/format.o \
	$(BUILD)$(MAGMA)/misc/scopedDebugMarker.o \
	$(BUILD)$(MAGMA)/misc/stringize.o \
	\
	$(BUILD)$(MAGMA)/objects/buffer.o \
	$(BUILD)$(MAGMA)/objects/bufferView.o \
	$(BUILD)$(MAGMA)/objects/commandBuffer.o \
	$(BUILD)$(MAGMA)/objects/commandPool.o \
	$(BUILD)$(MAGMA)/objects/debugMarker.o \
	$(BUILD)$(MAGMA)/objects/debugReportCallback.o \
	$(BUILD)$(MAGMA)/objects/descriptorPool.o \
	$(BUILD)$(MAGMA)/objects/descriptorSet.o \
	$(BUILD)$(MAGMA)/objects/descriptorSetLayout.o \
	$(BUILD)$(MAGMA)/objects/device.o \
	$(BUILD)$(MAGMA)/objects/deviceMemory.o \
	$(BUILD)$(MAGMA)/objects/display.o \
	$(BUILD)$(MAGMA)/objects/displayMode.o \
	$(BUILD)$(MAGMA)/objects/dstTransferBuffer.o \
	$(BUILD)$(MAGMA)/objects/event.o \
	$(BUILD)$(MAGMA)/objects/fence.o \
	$(BUILD)$(MAGMA)/objects/framebuffer.o \
	$(BUILD)$(MAGMA)/objects/image.o \
	$(BUILD)$(MAGMA)/objects/image1D.o \
	$(BUILD)$(MAGMA)/objects/image1DArray.o \
	$(BUILD)$(MAGMA)/objects/image2D.o \
	$(BUILD)$(MAGMA)/objects/image2DArray.o \
	$(BUILD)$(MAGMA)/objects/image3D.o \
	$(BUILD)$(MAGMA)/objects/imageCube.o \
	$(BUILD)$(MAGMA)/objects/imageView.o \
	$(BUILD)$(MAGMA)/objects/indexBuffer.o \
	$(BUILD)$(MAGMA)/objects/indirectBuffer.o \
	$(BUILD)$(MAGMA)/objects/instance.o \
	$(BUILD)$(MAGMA)/objects/object.o \
	$(BUILD)$(MAGMA)/objects/physicalDevice.o \
	$(BUILD)$(MAGMA)/objects/pipeline.o \
	$(BUILD)$(MAGMA)/objects/pipelineCache.o \
	$(BUILD)$(MAGMA)/objects/pipelineLayout.o \
	$(BUILD)$(MAGMA)/objects/queryPool.o \
	$(BUILD)$(MAGMA)/objects/queue.o \
	$(BUILD)$(MAGMA)/objects/renderPass.o \
	$(BUILD)$(MAGMA)/objects/sampler.o \
	$(BUILD)$(MAGMA)/objects/semaphore.o \
	$(BUILD)$(MAGMA)/objects/shaderModule.o \
	$(BUILD)$(MAGMA)/objects/srcTransferBuffer.o \
	$(BUILD)$(MAGMA)/objects/storageBuffer.o \
	$(BUILD)$(MAGMA)/objects/surface.o \
	$(BUILD)$(MAGMA)/objects/swapchain.o \
	$(BUILD)$(MAGMA)/objects/uniformTexelBuffer.o \
	$(BUILD)$(MAGMA)/objects/vertexBuffer.o \
	\
	$(BUILD)$(MAGMA)/states/colorBlendState.o \
	$(BUILD)$(MAGMA)/states/depthStencilState.o \
	$(BUILD)$(MAGMA)/states/inputAssemblyState.o \
	$(BUILD)$(MAGMA)/states/multisampleState.o \
	$(BUILD)$(MAGMA)/states/rasterizationState.o \
	$(BUILD)$(MAGMA)/states/samplerState.o \
	$(BUILD)$(MAGMA)/states/tesselationState.o \
	$(BUILD)$(MAGMA)/states/vertexInputState.o \
	$(BUILD)$(MAGMA)/states/viewportState.o \
	\
	$(BUILD)$(MAGMA)/utilities/immediateRender.o

# Framework object files

FRAMEWORK=/framework
FRAMEWORK_OBJS= \
	$(BUILD)$(FRAMEWORK)/bezierMesh.o \
	$(BUILD)$(FRAMEWORK)/cubeMesh.o \
	$(BUILD)$(FRAMEWORK)/knotMesh.o \
	$(BUILD)$(FRAMEWORK)/main.o \
	$(BUILD)$(FRAMEWORK)/linearAllocator.o \
	$(BUILD)$(FRAMEWORK)/planeMesh.o \
	$(BUILD)$(FRAMEWORK)/shader.o \
	$(BUILD)$(FRAMEWORK)/utilities.o \
	$(BUILD)$(FRAMEWORK)/vulkanApp.o \
	$(BUILD)$(FRAMEWORK)/xcbApp.o

OBJS = $(MAGMA_OBJS) $(FRAMEWORK_OBJS)
DEPS := $(OBJS:.o=.d)

-include $(DEPS)

$(BUILD)/%.o: %.cpp
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

%.o: %.vert
	@echo Compiling vertex shader
	$(GLSLC) -V $*.vert -o $*.o

%.o: %.frag
	@echo Compiling fragment shader
	$(GLSLC) -V $*.frag -o $*.o

# Sample app dependencies and linker arguments

$(TARGET01)/$(TARGET01): $(BUILD)/$(TARGET01)/$(TARGET01).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET02)/$(TARGET02): $(BUILD)/$(TARGET02)/$(TARGET02).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET03)/$(TARGET03): $(BUILD)/$(TARGET03)/$(TARGET03).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET04)/$(TARGET04): $(BUILD)/$(TARGET04)/$(TARGET04).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET05)/$(TARGET05): $(BUILD)/$(TARGET05)/$(TARGET05).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET06)/$(TARGET06): $(BUILD)/$(TARGET06)/$(TARGET06).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET07)/$(TARGET07): $(BUILD)/$(TARGET07)/$(TARGET07).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET08)/$(TARGET08): $(BUILD)/$(TARGET08)/$(TARGET08).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET09)/$(TARGET09): $(BUILD)/$(TARGET09)/$(TARGET09).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET10)/$(TARGET10): $(BUILD)/$(TARGET10)/$(TARGET10).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET11)/$(TARGET11): $(BUILD)/$(TARGET11)/$(TARGET11).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET12)/$(TARGET12): $(BUILD)/$(TARGET12)/$(TARGET12).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET13)/$(TARGET13): $(BUILD)/$(TARGET13)/$(TARGET13).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET14)/$(TARGET14): $(BUILD)/$(TARGET14)/$(TARGET14).o $(OBJS) $(BUILD)/$(TARGET14)/particlesystem.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET15)/$(TARGET15): $(BUILD)/$(TARGET15)/$(TARGET15).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET16)/$(TARGET16): $(BUILD)/$(TARGET16)/$(TARGET16).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Create build directories

mkbuilddir:
	@mkdir -p $(BUILD)
	@mkdir -p $(BUILD)$(MAGMA)/allocator
	@mkdir -p $(BUILD)$(MAGMA)/barriers
	@mkdir -p $(BUILD)$(MAGMA)/descriptors
	@mkdir -p $(BUILD)$(MAGMA)/helpers
	@mkdir -p $(BUILD)$(MAGMA)/misc
	@mkdir -p $(BUILD)$(MAGMA)/objects
	@mkdir -p $(BUILD)$(MAGMA)/states
	@mkdir -p $(BUILD)$(MAGMA)/utilities
	@mkdir -p $(BUILD)$(FRAMEWORK)
	@mkdir -p $(BUILD)/$(TARGET01)
	@mkdir -p $(BUILD)/$(TARGET02)
	@mkdir -p $(BUILD)/$(TARGET03)
	@mkdir -p $(BUILD)/$(TARGET04)
	@mkdir -p $(BUILD)/$(TARGET05)
	@mkdir -p $(BUILD)/$(TARGET06)
	@mkdir -p $(BUILD)/$(TARGET07)
	@mkdir -p $(BUILD)/$(TARGET08)
	@mkdir -p $(BUILD)/$(TARGET09)
	@mkdir -p $(BUILD)/$(TARGET10)
	@mkdir -p $(BUILD)/$(TARGET11)
	@mkdir -p $(BUILD)/$(TARGET12)
	@mkdir -p $(BUILD)/$(TARGET13)
	@mkdir -p $(BUILD)/$(TARGET14)
	@mkdir -p $(BUILD)/$(TARGET15)
	@mkdir -p $(BUILD)/$(TARGET16)

# Sample app shaders

shaders-02: $(TARGET02)/position.o		$(TARGET02)/fill.o
shaders-03: $(TARGET03)/passthrough.o	$(TARGET03)/fill.o
shaders-04: $(TARGET04)/transform.o		$(TARGET04)/frontFace.o
shaders-05: $(TARGET05)/transform.o		$(TARGET05)/normal.o
shaders-06: $(TARGET06)/passthrough.o	$(TARGET06)/multitexture.o
shaders-07: $(TARGET07)/transform.o		$(TARGET07)/textureArray.o
shaders-08: $(TARGET08)/transform.o		$(TARGET08)/envmap.o
shaders-09: $(TARGET09)/transform.o		$(TARGET09)/texture.o
shaders-10: $(TARGET10)/triangle.o		$(TARGET10)/fill.o $(TARGET10)/quad.o $(TARGET10)/texture.o
shaders-11: $(TARGET11)/transform.o		$(TARGET11)/fill.o
shaders-12: $(TARGET12)/passthrough.o	$(TARGET12)/fill.o
shaders-13: $(TARGET13)/transform.o		$(TARGET13)/specialized.o
shaders-14: $(TARGET14)/pointSize.o		$(TARGET14)/particle.o $(TARGET14)/particleNeg.o

$(TARGET15)/sum.o: $(TARGET15)/arithmetic.comp
	$(GLSLC) -V $(TARGET15)/arithmetic.comp -e sum --source-entrypoint main -o $(TARGET15)/sum.o

$(TARGET15)/mul.o: $(TARGET15)/arithmetic.comp
	$(GLSLC) -V $(TARGET15)/arithmetic.comp -e mul --source-entrypoint main -o $(TARGET15)/mul.o

$(TARGET15)/power.o: $(TARGET15)/arithmetic.comp
	$(GLSLC) -V $(TARGET15)/arithmetic.comp -e power --source-entrypoint main -o $(TARGET15)/power.o

shaders-15: $(TARGET15)/sum.o $(TARGET15)/mul.o $(TARGET15)/power.o

# Sample app builds: make output directory, build executable, compile shaders

01-clear:				mkbuilddir $(TARGET01)/$(TARGET01)
02-simple-triangle:		mkbuilddir $(TARGET02)/$(TARGET02) shaders-02
03-vertex-buffer:		mkbuilddir $(TARGET03)/$(TARGET03) shaders-03
04-vertex-transform:	mkbuilddir $(TARGET04)/$(TARGET04) shaders-04
05-mesh:				mkbuilddir $(TARGET05)/$(TARGET05) shaders-05
06-texture:				mkbuilddir $(TARGET06)/$(TARGET06) shaders-06
07-texture-array:		mkbuilddir $(TARGET07)/$(TARGET07) shaders-07
08-cubemap:				mkbuilddir $(TARGET08)/$(TARGET08) shaders-08
09-alpha-blend:			mkbuilddir $(TARGET09)/$(TARGET09) shaders-09
10-render-to-texture:	mkbuilddir $(TARGET10)/$(TARGET10) shaders-10
11-occlusion-query:		mkbuilddir $(TARGET11)/$(TARGET11) shaders-11
12-pushconstants:		mkbuilddir $(TARGET12)/$(TARGET12) shaders-12
13-specialization:		mkbuilddir $(TARGET13)/$(TARGET13) shaders-13
14-particles:			mkbuilddir $(TARGET14)/$(TARGET14) shaders-14
15-compute:				mkbuilddir $(TARGET15)/$(TARGET15) shaders-15
16-immediate:			mkbuilddir $(TARGET16)/$(TARGET16)

# Build all samples

all:$(TARGET01) \
	$(TARGET02) \
	$(TARGET03) \
	$(TARGET04) \
	$(TARGET05) \
	$(TARGET06) \
	$(TARGET07) \
	$(TARGET08) \
	$(TARGET09) \
	$(TARGET10) \
	$(TARGET11) \
	$(TARGET12) \
	$(TARGET13) \
	$(TARGET14) \
	$(TARGET15) \
	$(TARGET16)

# Remove build stuff

clean:
	@find . -name '*.o' -delete
	@rm -rf $(DEPS) \
	$(TARGET01)/$(TARGET01) \
	$(TARGET02)/$(TARGET02) \
	$(TARGET03)/$(TARGET03) \
	$(TARGET04)/$(TARGET04) \
	$(TARGET05)/$(TARGET05) \
	$(TARGET06)/$(TARGET06) \
	$(TARGET07)/$(TARGET07) \
	$(TARGET08)/$(TARGET08) \
	$(TARGET09)/$(TARGET09) \
	$(TARGET10)/$(TARGET10) \
	$(TARGET11)/$(TARGET11) \
	$(TARGET12)/$(TARGET12) \
	$(TARGET13)/$(TARGET13) \
	$(TARGET14)/$(TARGET14) \
	$(TARGET15)/$(TARGET15) \
	$(TARGET16)/$(TARGET16)
	@rm -rf $(BUILD)

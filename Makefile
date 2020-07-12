CC=g++
GLSLC=$(VULKAN_SDK)/bin/glslangValidator

PLATFORM=VK_USE_PLATFORM_XCB_KHR
CONSTEXPR_DEPTH_FLAGS=-ftemplate-depth=2048 -fconstexpr-depth=2048
INCLUDE_DIR=-I$(VULKAN_SDK)/include -Ithird-party -Ithird-party/rapid
LIBRARY_DIR=-L$(VULKAN_SDK)/lib -Lthird-party/magma -Lthird-party/quadric

BASE_CFLAGS=-std=c++14 -m64 -msse4 -pthread -MD -D$(PLATFORM) $(CONSTEXPR_DEPTH_FLAGS) $(INCLUDE_DIR)
DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CFLAGS=$(BASE_CFLAGS) -O0 -g -D_DEBUG
	MAGMA_LIB=magmad
	QUADRIC_LIB=quadricd
else
	CFLAGS=$(BASE_CFLAGS) -O3 -DNDEBUG
	MAGMA_LIB=magma
	QUADRIC_LIB=quadric
endif
LDFLAGS=$(LIBRARY_DIR) -lpthread -lxcb -lvulkan -l$(QUADRIC_LIB) -l$(MAGMA_LIB)

BUILD=build
MAGMA=third-party/magma
QUADRIC=third-party/quadric
FRAMEWORK=framework

# Sample app dir/exe names

TARGET01=01-clear
TARGET02=02-triangle
TARGET03=03-vertex-buffer
TARGET04=04-vertex-transform
TARGET05=05-mesh
TARGET06=06-texture
TARGET07=07-texture-array
TARGET08=08-texture-cube
TARGET09=09-texture-volume
TARGET10a=10.a-render-to-texture
TARGET10b=10.b-render-to-msaa-texture
TARGET11=11-occlusion-query
TARGET12=12-alpha-blend
TARGET13=13-specialization
TARGET14=14-pushconstants
TARGET15=15-particles
TARGET16=16-immediate-mode
TARGET18=18-compute

# Framework object files

FRAMEWORK_OBJS= \
	$(BUILD)/$(FRAMEWORK)/graphicsPipeline.o \
	$(BUILD)/$(FRAMEWORK)/main.o \
	$(BUILD)/$(FRAMEWORK)/linearAllocator.o \
	$(BUILD)/$(FRAMEWORK)/utilities.o \
	$(BUILD)/$(FRAMEWORK)/vulkanApp.o \
	$(BUILD)/$(FRAMEWORK)/xcbApp.o

OBJS = $(FRAMEWORK_OBJS)
DEPS := $(OBJS:.o=.d)

-include $(DEPS)

$(BUILD)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

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

$(TARGET10a)/$(TARGET10a): $(BUILD)/$(TARGET10a)/$(TARGET10a).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET10b)/$(TARGET10b): $(BUILD)/$(TARGET10b)/$(TARGET10b).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET11)/$(TARGET11): $(BUILD)/$(TARGET11)/$(TARGET11).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET12)/$(TARGET12): $(BUILD)/$(TARGET12)/$(TARGET12).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET13)/$(TARGET13): $(BUILD)/$(TARGET13)/$(TARGET13).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET14)/$(TARGET14): $(BUILD)/$(TARGET14)/$(TARGET14).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET15)/$(TARGET15): $(BUILD)/$(TARGET15)/$(TARGET15).o $(OBJS) $(BUILD)/$(TARGET15)/particlesystem.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET16)/$(TARGET16): $(BUILD)/$(TARGET16)/$(TARGET16).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET18)/$(TARGET18): $(BUILD)/$(TARGET18)/$(TARGET18).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Create build directories

builddir:
	@mkdir -p $(BUILD)
	@mkdir -p $(BUILD)/$(MAGMA)/allocator
	@mkdir -p $(BUILD)/$(MAGMA)/auxiliary
	@mkdir -p $(BUILD)/$(MAGMA)/barriers
	@mkdir -p $(BUILD)/$(MAGMA)/descriptors
	@mkdir -p $(BUILD)/$(MAGMA)/exceptions
	@mkdir -p $(BUILD)/$(MAGMA)/extensions
	@mkdir -p $(BUILD)/$(MAGMA)/helpers
	@mkdir -p $(BUILD)/$(MAGMA)/misc
	@mkdir -p $(BUILD)/$(MAGMA)/objects
	@mkdir -p $(BUILD)/$(MAGMA)/shaders
	@mkdir -p $(BUILD)/$(MAGMA)/states
	@mkdir -p $(BUILD)/$(QUADRIC)
	@mkdir -p $(BUILD)/$(FRAMEWORK)
	@mkdir -p $(BUILD)/$(TARGET01)
	@mkdir -p $(BUILD)/$(TARGET02)
	@mkdir -p $(BUILD)/$(TARGET03)
	@mkdir -p $(BUILD)/$(TARGET04)
	@mkdir -p $(BUILD)/$(TARGET05)
	@mkdir -p $(BUILD)/$(TARGET06)
	@mkdir -p $(BUILD)/$(TARGET07)
	@mkdir -p $(BUILD)/$(TARGET08)
	@mkdir -p $(BUILD)/$(TARGET09)
	@mkdir -p $(BUILD)/$(TARGET10a)
	@mkdir -p $(BUILD)/$(TARGET10b)
	@mkdir -p $(BUILD)/$(TARGET11)
	@mkdir -p $(BUILD)/$(TARGET12)
	@mkdir -p $(BUILD)/$(TARGET13)
	@mkdir -p $(BUILD)/$(TARGET14)
	@mkdir -p $(BUILD)/$(TARGET15)
	@mkdir -p $(BUILD)/$(TARGET16)
	@mkdir -p $(BUILD)/$(TARGET18)

magma:
	$(MAKE) -C $(MAGMA) magma

quadric:
	$(MAKE) -C $(QUADRIC) quadric

# Sample app shaders

shaders-02: $(TARGET02)/position.o		$(TARGET02)/fill.o
shaders-03: $(TARGET03)/passthrough.o	$(TARGET03)/fill.o
shaders-04: $(TARGET04)/transform.o		$(TARGET04)/frontFace.o
shaders-05: $(TARGET05)/transform.o		$(TARGET05)/normal.o
shaders-06: $(TARGET06)/passthrough.o	$(TARGET06)/multitexture.o
shaders-07: $(TARGET07)/transform.o		$(TARGET07)/textureArray.o
shaders-08: $(TARGET08)/transform.o		$(TARGET08)/envmap.o
shaders-09: $(TARGET09)/quad.o			$(TARGET09)/raycast.o
shaders-10a: $(TARGET10a)/pasthrough.o	$(TARGET10a)/triangle.o		$(TARGET10a)/fill.o $(TARGET10a)/tex.o
shaders-10b: $(TARGET10b)/passthrough.o	$(TARGET10b)/triangle.o		$(TARGET10b)/fill.o $(TARGET10b)/tex.o
shaders-11: $(TARGET11)/transform.o		$(TARGET11)/fill.o
shaders-12: $(TARGET12)/transform.o		$(TARGET12)/texture.o
shaders-13: $(TARGET13)/transform.o		$(TARGET13)/specialized.o
shaders-14: $(TARGET14)/passthrough.o	$(TARGET14)/fill.o
shaders-15: $(TARGET15)/pointSize.o		$(TARGET15)/particle.o		$(TARGET15)/particleNeg.o

$(TARGET18)/sum.o: $(TARGET18)/arithmetic.comp
	$(GLSLC) -V $(TARGET18)/arithmetic.comp -e sum --source-entrypoint main -o $(TARGET18)/sum.o

$(TARGET18)/mul.o: $(TARGET18)/arithmetic.comp
	$(GLSLC) -V $(TARGET18)/arithmetic.comp -e mul --source-entrypoint main -o $(TARGET18)/mul.o

$(TARGET18)/power.o: $(TARGET18)/arithmetic.comp
	$(GLSLC) -V $(TARGET18)/arithmetic.comp -e power --source-entrypoint main -o $(TARGET18)/power.o

shaders-18: $(TARGET18)/sum.o $(TARGET18)/mul.o $(TARGET18)/power.o

# Sample app builds: make output directory, build executable, compile shaders

01-clear:					builddir magma $(TARGET01)/$(TARGET01)
02-triangle:				builddir magma $(TARGET02)/$(TARGET02) shaders-02
03-vertex-buffer:			builddir magma $(TARGET03)/$(TARGET03) shaders-03
04-vertex-transform:		builddir magma $(TARGET04)/$(TARGET04) shaders-04
05-mesh:					builddir magma quadric $(TARGET05)/$(TARGET05) shaders-05
06-texture:					builddir magma $(TARGET06)/$(TARGET06) shaders-06
07-texture-array:			builddir magma quadric $(TARGET07)/$(TARGET07) shaders-07
08-texture-cube:			builddir magma quadric $(TARGET08)/$(TARGET08) shaders-08
09-texture-volume:			builddir magma $(TARGET09)/$(TARGET09) shaders-09
10a-render-to-texture:		builddir magma $(TARGET10a)/$(TARGET10a) shaders-10a
10b-render-to-msaa-texture:	builddir magma $(TARGET10b)/$(TARGET10b) shaders-10b
11-occlusion-query:			builddir magma quadric $(TARGET11)/$(TARGET11) shaders-11
12-alpha-blend:				builddir magma quadric $(TARGET12)/$(TARGET12) shaders-12
13-specialization:			builddir magma quadric $(TARGET13)/$(TARGET13) shaders-13
14-pushconstants:			builddir magma $(TARGET14)/$(TARGET14) shaders-14
15-particles:				builddir magma $(TARGET15)/$(TARGET15) shaders-15
16-immediate-mode:			builddir magma $(TARGET16)/$(TARGET16)
18-compute:					builddir magma $(TARGET18)/$(TARGET18) shaders-18

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
	$(TARGET10a) \
	$(TARGET10b) \
	$(TARGET11) \
	$(TARGET12) \
	$(TARGET13) \
	$(TARGET14) \
	$(TARGET15) \
	$(TARGET16) \
	$(TARGET18)

# Remove build stuff

clean:
	$(MAKE) -C $(MAGMA) clean
	$(MAKE) -C $(QUADRIC) clean
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
	$(TARGET10a)/$(TARGET10a) \
	$(TARGET10b)/$(TARGET10b) \
	$(TARGET11)/$(TARGET11) \
	$(TARGET12)/$(TARGET12) \
	$(TARGET13)/$(TARGET13) \
	$(TARGET14)/$(TARGET14) \
	$(TARGET15)/$(TARGET15) \
	$(TARGET16)/$(TARGET16) \
	$(TARGET18)/$(TARGET18)
	@rm -rf $(BUILD)

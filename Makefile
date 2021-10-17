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
LDFLAGS=$(LIBRARY_DIR) -lpthread -lxcb -lvulkan -l$(MAGMA_LIB)
LDFLAGS_Q=$(LIBRARY_DIR) -lpthread -lxcb -lvulkan -l$(QUADRIC_LIB) -l$(MAGMA_LIB)

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
	$(CC) -o $@ $^ $(LDFLAGS_Q)

$(TARGET06)/$(TARGET06): $(BUILD)/$(TARGET06)/$(TARGET06).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET07)/$(TARGET07): $(BUILD)/$(TARGET07)/$(TARGET07).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS_Q)

$(TARGET08)/$(TARGET08): $(BUILD)/$(TARGET08)/$(TARGET08).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS_Q)

$(TARGET09)/$(TARGET09): $(BUILD)/$(TARGET09)/$(TARGET09).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET10a)/$(TARGET10a): $(BUILD)/$(TARGET10a)/$(TARGET10a).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET10b)/$(TARGET10b): $(BUILD)/$(TARGET10b)/$(TARGET10b).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET11)/$(TARGET11): $(BUILD)/$(TARGET11)/$(TARGET11).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS_Q)

$(TARGET12)/$(TARGET12): $(BUILD)/$(TARGET12)/$(TARGET12).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS_Q)

$(TARGET13)/$(TARGET13): $(BUILD)/$(TARGET13)/$(TARGET13).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS_Q)

$(TARGET14)/$(TARGET14): $(BUILD)/$(TARGET14)/$(TARGET14).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET15)/$(TARGET15): $(BUILD)/$(TARGET15)/$(TARGET15).o $(OBJS) $(BUILD)/$(TARGET15)/particlesystem.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET16)/$(TARGET16): $(BUILD)/$(TARGET16)/$(TARGET16).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET18)/$(TARGET18): $(BUILD)/$(TARGET18)/$(TARGET18).o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Make build directories

mkdir-magma:
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

mkdir-quadric:
	@mkdir -p $(BUILD)/$(QUADRIC)

mkdir-framework:
	@mkdir -p $(BUILD)/$(FRAMEWORK)

mkdir-01:
	@mkdir -p $(BUILD)/$(TARGET01)

mkdir-02:
	@mkdir -p $(BUILD)/$(TARGET02)

mkdir-03:
	@mkdir -p $(BUILD)/$(TARGET03)

mkdir-04:
	@mkdir -p $(BUILD)/$(TARGET04)

mkdir-05:
	@mkdir -p $(BUILD)/$(TARGET05)

mkdir-06:
	@mkdir -p $(BUILD)/$(TARGET06)

mkdir-07:
	@mkdir -p $(BUILD)/$(TARGET07)

mkdir-08:
	@mkdir -p $(BUILD)/$(TARGET08)

mkdir-09:
	@mkdir -p $(BUILD)/$(TARGET09)

mkdir-10a:
	@mkdir -p $(BUILD)/$(TARGET10a)

mkdir-10b:
	@mkdir -p $(BUILD)/$(TARGET10b)

mkdir-11:
	@mkdir -p $(BUILD)/$(TARGET11)

mkdir-12:
	@mkdir -p $(BUILD)/$(TARGET12)

mkdir-13:
	@mkdir -p $(BUILD)/$(TARGET13)

mkdir-14:
	@mkdir -p $(BUILD)/$(TARGET14)

mkdir-15:
	@mkdir -p $(BUILD)/$(TARGET15)

mkdir-16:
	@mkdir -p $(BUILD)/$(TARGET16)

mkdir-18:
	@mkdir -p $(BUILD)/$(TARGET18)

# Build third-party libraries

magma:
	 $(MAKE) -C $(MAGMA) magma

quadric:
	 $(MAKE) -C $(QUADRIC) quadric

# Build sample apps
	
01-clear: mkdir-magma \
	mkdir-framework \
	mkdir-01 \
	magma \
	$(TARGET01)/$(TARGET01)

02-triangle: mkdir-magma \
	mkdir-framework \
	mkdir-02 \
	magma \
	$(TARGET02)/$(TARGET02) \
	$(TARGET02)/position.o \
	$(TARGET02)/fill.o

03-vertex-buffer: mkdir-magma \
	mkdir-framework \
	mkdir-03 \
	magma \
	$(TARGET03)/$(TARGET03) \
	$(TARGET03)/passthrough.o \
	$(TARGET03)/fill.o

04-vertex-transform: mkdir-magma \
	mkdir-framework \
	mkdir-04 \
	magma \
	$(TARGET04)/$(TARGET04) \
	$(TARGET04)/transform.o \
	$(TARGET04)/frontFace.o

05-mesh: mkdir-magma \
	mkdir-quadric \
	mkdir-framework \
	mkdir-05 \
	magma \
	quadric \
	$(TARGET05)/$(TARGET05) \
	$(TARGET05)/transform.o \
	$(TARGET05)/normal.o

06-texture: mkdir-magma \
	mkdir-framework \
	mkdir-06 \
	magma \
	$(TARGET06)/$(TARGET06) \
	$(TARGET06)/passthrough.o \
	$(TARGET06)/multitexture.o

07-texture-array: mkdir-magma \
	mkdir-quadric \
	mkdir-framework \
	mkdir-07 \
	magma \
	quadric \
	$(TARGET07)/$(TARGET07) \
	$(TARGET07)/transform.o \
	$(TARGET07)/textureArray.o

08-texture-cube: mkdir-magma \
	mkdir-quadric \
	mkdir-framework \
	mkdir-08 \
	magma \
	quadric \
	$(TARGET08)/$(TARGET08) \
	$(TARGET08)/transform.o \
	$(TARGET08)/envmap.o

09-texture-volume: mkdir-magma \
	mkdir-framework \
	mkdir-09 \
	magma \
	$(TARGET09)/$(TARGET09) \
	$(TARGET09)/quad.o \
	$(TARGET09)/raycast.o

10.a-render-to-texture: mkdir-magma \
	mkdir-framework \
	mkdir-10a \
	magma \
	$(TARGET10a)/$(TARGET10a) \
	$(TARGET10a)/passthrough.o \
	$(TARGET10a)/triangle.o \
	$(TARGET10a)/fill.o \
	$(TARGET10a)/tex.o

10.b-render-to-msaa-texture: mkdir-magma \
	mkdir-framework \
	mkdir-10b \
	magma \
	$(TARGET10b)/$(TARGET10b) \
	$(TARGET10b)/passthrough.o \
	$(TARGET10b)/triangle.o \
	$(TARGET10b)/fill.o \
	$(TARGET10b)/tex.o

11-occlusion-query:	mkdir-magma \
	mkdir-quadric \
	mkdir-framework \
	mkdir-11 \
	magma \
	quadric \
	$(TARGET11)/$(TARGET11) \
	$(TARGET11)/transform.o \
	$(TARGET11)/fill.o

12-alpha-blend: mkdir-magma \
	mkdir-quadric \
	mkdir-framework \
	mkdir-12 \
	magma \
	quadric \
	$(TARGET12)/$(TARGET12) \
	$(TARGET12)/transform.o \
	$(TARGET12)/texture.o

13-specialization: mkdir-magma \
	mkdir-quadric \
	mkdir-framework \
	mkdir-13 \
	magma \
	quadric \
	$(TARGET13)/$(TARGET13) \
	$(TARGET13)/transform.o \
	$(TARGET13)/specialized.o

14-pushconstants: mkdir-magma \
	mkdir-framework \
	mkdir-14 \
	magma \
	$(TARGET14)/$(TARGET14) \
	$(TARGET14)/passthrough.o \
	$(TARGET14)/fill.o

15-particles: mkdir-magma \
	mkdir-framework \
	mkdir-15 \
	magma \
	$(TARGET15)/$(TARGET15) \
	$(TARGET15)/pointSize.o \
	$(TARGET15)/particle.o \
	$(TARGET15)/particleNeg.o

16-immediate-mode: mkdir-magma \
    mkdir-framework \
	mkdir-16 \
	magma \
	$(TARGET16)/$(TARGET16)

$(TARGET18)/sum.o: $(TARGET18)/arithmetic.comp
	$(GLSLC) -V $(TARGET18)/arithmetic.comp -e sum --source-entrypoint main -o $(TARGET18)/sum.o
$(TARGET18)/mul.o: $(TARGET18)/arithmetic.comp
	$(GLSLC) -V $(TARGET18)/arithmetic.comp -e mul --source-entrypoint main -o $(TARGET18)/mul.o
$(TARGET18)/power.o: $(TARGET18)/arithmetic.comp
	$(GLSLC) -V $(TARGET18)/arithmetic.comp -e power --source-entrypoint main -o $(TARGET18)/power.o

18-compute: mkdir-magma \
	mkdir-framework \
	mkdir-18 \
	magma \
	$(TARGET18)/$(TARGET18) \
	$(TARGET18)/sum.o \
	$(TARGET18)/mul.o \
	$(TARGET18)/power.o

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

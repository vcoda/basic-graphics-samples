CC=g++
GLSLC=$(VULKAN_SDK)/bin/glslangValidator

PLATFORM=VK_USE_PLATFORM_XCB_KHR
THIRD_PARTY=../third-party
INCLUDE_DIR=-I$(VULKAN_SDK)/include -I$(THIRD_PARTY) -I$(THIRD_PARTY)/rapid
LIB_DIR=-L$(VULKAN_SDK)/lib -L$(THIRD_PARTY)/magma -L$(THIRD_PARTY)/quadric

BASE_CFLAGS=-std=c++17 -m64 -msse4 -pthread -MD -D$(PLATFORM) $(INCLUDE_DIR)
DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CFLAGS=$(BASE_CFLAGS) -O0 -g -D_DEBUG
	MAGMA=magmad
	QUADRIC=quadricd
else
	CFLAGS=$(BASE_CFLAGS) -O3 -DNDEBUG
	MAGMA=magma
	QUADRIC=quadric
endif
LDFLAGS=$(LIB_DIR) -l$(MAGMA) -lpthread -lxcb -lvulkan

FRAMEWORK=../framework
FRAMEWORK_OBJS= \
	$(FRAMEWORK)/graphicsPipeline.o \
	$(FRAMEWORK)/main.o \
	$(FRAMEWORK)/utilities.o \
	$(FRAMEWORK)/vulkanApp.o \
	$(FRAMEWORK)/xcbApp.o

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.vert
	$(GLSLC) -V $*.vert -o $*.o

%.o: %.frag
	$(GLSLC) -V $*.frag -o $*.o


include ../Makeshared.mk

ifeq ($(DEBUG), 1)
	QUADRIC=quadricd
else
	QUADRIC=quadric
endif

LDFLAGS2=$(LIB_DIR) -lpthread -lxcb -lvulkan -l$(QUADRIC) -l$(MAGMA)

default:
	07-texture-array transform.o textureArray.o

07-texture-array:
	07-texture-array.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 07-texture-array


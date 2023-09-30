include ../Makeshared.mk

ifeq ($(DEBUG), 1)
	QUADRIC=quadricd
else
	QUADRIC=quadric
endif

LDFLAGS2=$(LIB_DIR) -lpthread -lxcb -lvulkan -l$(QUADRIC) -l$(MAGMA)

default:
	08-texture-cube transform.o envmap.o

08-texture-cube:
	08-texture-cube.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 08-texture-cube


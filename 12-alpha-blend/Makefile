include ../Makeshared.mk

ifeq ($(DEBUG), 1)
	QUADRIC=quadricd
else
	QUADRIC=quadric
endif

LDFLAGS2=$(LIB_DIR) -lpthread -lxcb -lvulkan -l$(QUADRIC) -l$(MAGMA)

default:
	12-alpha-blend transform.o texture.o

12-alpha-blend:
	12-alpha-blend.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 12-alpha-blend


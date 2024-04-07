include ../Makeshared.mk

LDFLAGS2=-l$(QUADRIC) $(LDFLAGS)

default: 07-texture-array shaders

07-texture-array: 07-texture-array.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

shaders: transform.o textureArray.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 07-texture-array



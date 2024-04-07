include ../Makeshared.mk

default: 09-texture-volume shaders

09-texture-volume: 09-texture-volume.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

shaders: quad.o raycast.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 09-texture-volume


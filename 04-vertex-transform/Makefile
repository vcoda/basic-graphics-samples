include ../Makeshared.mk

default: 04-vertex-transform shaders

04-vertex-transform: 04-vertex-transform.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

shaders: transform.o frontFace.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 04-vertex-transform

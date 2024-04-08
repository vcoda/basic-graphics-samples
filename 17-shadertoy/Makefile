include ../Makeshared.mk

default: 17-shadertoy shaders

17-shadertoy: 17-shadertoy.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lshaderc_combined

shaders: quad.o shader.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 17-shadertoy

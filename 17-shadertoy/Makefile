include ../Makeshared.mk

default:
	17-shadertoy quad.o shader.o

17-shadertoy:
	17-shadertoy.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) -lshaderc_combined

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 17-shadertoy


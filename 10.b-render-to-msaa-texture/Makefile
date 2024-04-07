include ../Makeshared.mk

default: 10b-render-to-msaa-texture shaders

10b-render-to-msaa-texture: 10.b-render-to-msaa-texture.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

shaders: passthrough.o triangle.o fill.o tex.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 10b-render-to-msaa-texture


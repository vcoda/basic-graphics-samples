include ../Makeshared.mk

default: 10a-render-to-texture shaders

10a-render-to-texture: 10.a-render-to-texture.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

shaders: passthrough.o triangle.o fill.o tex.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 10a-render-to-texture

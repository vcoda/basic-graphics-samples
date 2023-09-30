include ../Makeshared.mk

default:
	10b-render-to-msaa-texture passthrough.o triangle.o fill.o tex.o

10b-render-to-msaa-texture:
	10.b-render-to-msaa-texture.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 10b-render-to-msaa-texture


include ../Makeshared.mk

default: 03-vertex-buffer shaders

03-vertex-buffer: 03-vertex-buffer.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

shaders: passthrough.o fill.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 03-vertex-buffer

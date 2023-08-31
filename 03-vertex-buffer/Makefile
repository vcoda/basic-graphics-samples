include ../makeconf

default: 03-vertex-buffer passthrough.o fill.o

03-vertex-buffer: 03-vertex-buffer.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 03-vertex-buffer


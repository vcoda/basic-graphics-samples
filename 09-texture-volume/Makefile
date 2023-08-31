include ../makeconf

default: 09-texture-volume quad.o raycast.o

09-texture-volume: 09-texture-volume.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 09-texture-volume


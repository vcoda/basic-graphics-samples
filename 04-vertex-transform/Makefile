include ../makeconf

default: 04-vertex-transform transform.o frontFace.o

04-vertex-transform: 04-vertex-transform.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 04-vertex-transform


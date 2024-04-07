include ../Makeshared.mk

default: 14-pushconstants shaders

14-pushconstants: 14-pushconstants.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

shaders: passthrough.o fill.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 14-pushconstants


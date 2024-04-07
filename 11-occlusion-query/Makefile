include ../Makeshared.mk

LDFLAGS2=-l$(QUADRIC) $(LDFLAGS)

default: 11-occlusion-query shaders

11-occlusion-query: 11-occlusion-query.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

shaders: transform.o fill.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 11-occlusion-query

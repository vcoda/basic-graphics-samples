include ../Makeshared.mk

LDFLAGS2=-l$(QUADRIC) $(LDFLAGS)

default: 19-instancing shaders

05-mesh: 19-instancing.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

shaders: building.o diffuse.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 19-instancing

include ../Makeshared.mk

LDFLAGS2=-l$(QUADRIC) $(LDFLAGS)

default: 05-mesh shaders

05-mesh: 05-mesh.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

shaders: transform.o normal.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 05-mesh

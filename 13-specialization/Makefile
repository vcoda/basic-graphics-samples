include ../Makeshared.mk

LDFLAGS2=-l$(QUADRIC) $(LDFLAGS)

default: 13-specialization shaders

13-specialization: 13-specialization.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

shaders: transform.o specialized.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 13-specialization


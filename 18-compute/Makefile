include ../Makeshared.mk

default:
	18-compute sum.o mul.o power.o

18-compute:
	18-compute.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

sum.o: arithmetic.comp
	$(GLSLC) -V arithmetic.comp -e sum --source-entrypoint main -o sum.o
mul.o: arithmetic.comp
	$(GLSLC) -V arithmetic.comp -e mul --source-entrypoint main -o mul.o
power.o: arithmetic.comp
	$(GLSLC) -V arithmetic.comp -e power --source-entrypoint main -o power.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 18-compute


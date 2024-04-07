include ../Makeshared.mk

default: 15-particles shaders

15-particles: 15-particles.o particlesystem.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

shaders: pointSize.o particle.o particleNeg.o

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 15-particles


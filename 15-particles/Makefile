include ../makeconf

default: 15-particles pointSize.o particle.o particleNeg.o

15-particles: 15-particles.o particlesystem.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 15-particles


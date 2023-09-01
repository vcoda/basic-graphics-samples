include ../makeconf

ifeq ($(DEBUG), 1)
	QUADRIC=quadricd
else
	QUADRIC=quadric
endif

LDFLAGS2=$(LIB_DIR) -lpthread -lxcb -lvulkan -l$(QUADRIC) -l$(MAGMA)

default: 13-specialization transform.o specialized.o

13-specialization: 13-specialization.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 13-specialization


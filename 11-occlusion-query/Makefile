include ../makeconf

ifeq ($(DEBUG), 1)
	QUADRIC=quadricd
else
	QUADRIC=quadric
endif

LDFLAGS2=$(LIB_DIR) -lpthread -lxcb -lvulkan -l$(QUADRIC) -l$(MAGMA)

default: 11-occlusion-query transform.o fill.o

11-occlusion-query: 11-occlusion-query.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS2)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 11-occlusion-query


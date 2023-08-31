include ../makeconf

16-immediate-mode: 16-immediate-mode.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 16-immediate-mode


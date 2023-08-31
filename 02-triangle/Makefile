include ../makeconf

default: 02-triangle position.o fill.o

02-triangle: 02-triangle.o $(FRAMEWORK_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@find . -iregex '.*\.\(d\|o\)' -delete
	@find $(FRAMEWORK) -iregex '.*\.\(d\|o\)' -delete
	@rm -rf 02-triangle

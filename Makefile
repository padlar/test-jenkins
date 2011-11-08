CFLAGS=-Wall -g3
all: androgenizer

androgenizer: main.o options.o emit.o common.h emit.h options.h library.h library.o option_entries.h
	$(CC) $(CFLAGS) main.o options.o emit.o library.o -o androgenizer

clean:
	rm -f main.o options.o emit.o library.o androgenizer

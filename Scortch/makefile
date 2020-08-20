CC=gcc
CFLAGS=-I.
DEPS = sort.h
OBJ = main.o sort.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

scortch: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f ./*.o

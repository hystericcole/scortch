CC=gcc
CFLAGS=-I. -O3
DEPS = sort.h
OBJ = main.o sort.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

scortch: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f ./*.o

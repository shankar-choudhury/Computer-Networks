CC=gcc
CFLAGS=-Wall -Werror -g
TARGET=proj1

all: $(TARGET)

$(TARGET): proj1.o
	$(CC) $(CFLAGS) -o $@ $<

proj1.o: proj1.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o

distclean: clean
	rm -f $(TARGET)

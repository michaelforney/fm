.POSIX:
.PHONY: all clean

CFLAGS+=-pedantic -Wall
LDLIBS=-lm

OBJ=\
	main.o\
	net.o\
	util.o

all: fm

clean:
	rm -f fm $(OBJ)

$(OBJ): fm.h

fm: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)


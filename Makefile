CC = cc

CFLAGS = \
	-I$(shell brew --prefix raylib)/include \
	-g \
	-O3 \
	-fsanitize=address,undefined \
	-fno-omit-frame-pointer

LDFLAGS = \
	-L$(shell brew --prefix raylib)/lib \
	-lraylib \
	-fsanitize=address,undefined

sim: main.c
	$(CC) $(CFLAGS) main.c -o sim $(LDFLAGS)

clean:
	rm -f sim

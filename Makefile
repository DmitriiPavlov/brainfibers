CC = cc
CFLAGS = -I$(shell brew --prefix raylib)/include
LDFLAGS = -L$(shell brew --prefix raylib)/lib -lraylib
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .

sim: main.c
	$(CC) $(CFLAGS) main.c -o sim $(LDFLAGS)

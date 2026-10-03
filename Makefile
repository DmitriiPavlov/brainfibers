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

PROFILE_CC = gcc-16

PROFILE_CFLAGS = \
	-I$(shell brew --prefix raylib)/include \
	-g \
	-O2 \
	-pg

PROFILE_LDFLAGS = \
	-L$(shell brew --prefix raylib)/lib \
	-lraylib \
	-pg

sim: main.c
	$(CC) $(CFLAGS) main.c -o sim $(LDFLAGS)

sim_profile: main.c
	$(PROFILE_CC) $(PROFILE_CFLAGS) main.c -o sim_profile $(PROFILE_LDFLAGS)

clean:
	rm -f sim sim_profile gmon.out profile.txt

CFLAGS = -Wall -Wextra -Werror -O3
TARGETS = lab12sesN3249 lines_count.so

all: lib.so
	gcc $(CFLAGS) -o lab12sesN3249 lab12sesN3249.c -ldl

lib.so:
	gcc $(CFLAGS) -shared -fPIC -o libsesN3249.so libsesN3249.c -ldl -lm

clean:
	rm -rf *.o *.so $(TARGETS)

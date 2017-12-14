JANSSON_FLAGS=$(shell pkg-config --cflags --libs jansson)
LIBGNURX_FLAGS=$(shell pkg-config --cflags --libs gnurx)
GTK_FLAGS=$(shell pkg-config --cflags --libs gtk+-3.0)
GCC_RELEASE_OPTIONS=-Wall -Wextra -Wpedantic -std=gnu11 -O3 -m64
GCC_DEBUG_OPTIONS=-Wall -Wextra -Wpedantic -std=gnu11 -O0 -g3 -m64

release : main.c
	gcc $(GCC_RELEASE_OPTIONS) main.c -o main $(JANSSON_FLAGS) $(LIBGNURX_FLAGS) $(GTK_FLAGS)
	strip main

debug : main.c
	gcc $(GCC_DEBUG_OPTIONS) main.c -o main $(JANSSON_FLAGS) $(LIBGNURX_FLAGS) $(GTK_FLAGS)

JANSSON_FLAGS=$(shell pkg-config --cflags --libs jansson)
LIBGNURX_FLAGS=$(shell pkg-config --cflags --libs gnurx)
GTK_FLAGS=$(shell pkg-config --cflags --libs gtk+-3.0)
GCC_OPTINS=-Wall -Wextra -Wpedantic -std=gnu11 -O3 -m64 -lpthread

main : main.c
	gcc $(GCC_OPTINS) main.c -o main $(JANSSON_FLAGS) $(LIBGNURX_FLAGS) $(GTK_FLAGS)

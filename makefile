GTK_FLAGS=$(shell pkg-config --cflags --libs gtk+-3.0)
OPTIONS=-Wall -Wextra -Wpedantic -std=gnu11 -O3 -m64 -lm -pthread

imageview : imageview.c
	gcc $(OPTIONS) -o imageview imageview.c $(GTK_FLAGS)

clean :
	-rm imageview

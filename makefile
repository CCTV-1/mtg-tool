GTK_FLAGS=$(shell pkg-config --cflags --libs gtk+-3.0)

imageview : imageview.c
	gcc -Wall -Wextra -Wpedantic -std=gnu14 -O3 -m64 -o imageview imageview.c $(GTK_FLAGS)

clean :
	-rm imageview

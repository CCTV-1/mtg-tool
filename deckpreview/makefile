CURL_FLAGS=$(shell pkg-config --cflags --libs libcurl)
JANSSON_FLAGS=$(shell pkg-config --cflags --libs jansson)
GTK_FLAGS=$(shell pkg-config --cflags --libs gtk+-3.0)
GCC_RELEASE_OPTIONS=-Wall -Wextra -Wpedantic -Wno-format -std=gnu11 -O3 -m64
GCC_DEBUG_OPTIONS=-Wall -Wextra -Wpedantic -Wno-format -std=gnu11 -O0 -pg -g3 -m64

release : main.c deckprase.c
	gcc $(GCC_RELEASE_OPTIONS) main.c deckprase.c previewconfig.c -o main $(JANSSON_FLAGS) $(GTK_FLAGS) $(CURL_FLAGS)

debug : main.c deckprase.c
	gcc $(GCC_DEBUG_OPTIONS) main.c -o main deckprase.c previewconfig.c $(JANSSON_FLAGS) $(GTK_FLAGS) $(CURL_FLAGS)

clean :
	-rm main
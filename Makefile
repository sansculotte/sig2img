CC=clang
CFLAGS=-Wall $(shell pkg-config --cflags libpng sndfile)
LIBS=$(shell pkg-config --libs libpng sndfile)
EXECUTEABLE=sig2img

sig2img: sig2img.c
	$(CC) $(CFLAGS) sig2img.c $(LIBS) -o$(EXECUTEABLE)

debug: sig2img.c
	$(CC) $(CFLAGS) -g sig2img.c $(LIBS) -o$(EXECUTEABLE)

clean:
	rm sig2img

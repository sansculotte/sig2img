CC=clang
CFLAGS=-Wall
LIBS=-lpng -lsndfile -lm
EXECUTEABLE=sig2img

sig2img: sig2img.c
	$(CC) $(CFLAGS) sig2img.c $(LIBS) -o$(EXECUTEABLE)

debug: sig2img.c
	$(CC) $(CFLAGS) -g sig2img.c $(LIBS) -o$(EXECUTEABLE)

clean:
	rm sig2img

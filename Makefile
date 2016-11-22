CC=clang
CFLAGS=-Wall
LIBS=-lpng -lsndfile -lm

sig2img: sig2img.c
	$(CC) $(CFLAGS) sig2img.c $(LIBS) -osig2img

debug: sig2img.c
	$(CC) $(CFLAGS) -g sig2img.c $(LIBS) -osig2img

clean:
	rm sig2img

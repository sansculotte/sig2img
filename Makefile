CC=gcc 
CFLAGS=-Wall
LIBS=-lpng -lsndfile

sig2img:
	$(CC) $(CFLAGS)  sig2img.c $(LIBS) -osig2img

clean:
	rm sig2img 

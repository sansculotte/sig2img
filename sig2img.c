// vim: softtabstop=4 :shiftwidth=4 :expandtab
/*******************************************************************************
 *
 * sig2img
 * write an audiobuffer as a sequence of image files
 * can be usefull for an audio-based texture in a 3D
 * application.
 * 062011 u@sansculotte.net
 *
 * todo:
 * - pixelbuffer > audiobuffer: shift
 * - audiobuffer > pixelbuffer: squeeze? truncate?
 * - option to seperate channels
 * - distort an input image with soundbuffer
 * - float fps
 * - test compile and run on different systems
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sndfile.h>
#include <png.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#define MINFPS 1
#define MAXFPS 1024
#define MAXWIDTH 8196
#define MAXHEIGHT 8196
#define MINWIDTH 1
#define MINHEIGHT 1
#define MINBUFSIZE 64
#define MAXBUFSIZE 2^32
#define BYTES_PER_PIXEL 1


typedef struct  {
   uint16_t width;
   uint16_t height;
   uint32_t audio_frames;
   uint16_t channels;
   uint16_t bit_depth;
} s_dimension;

void frame_loop(size_t frame, char* output_dir, SNDFILE* audio_file, s_dimension d);
void set_filename(char* file_name, char* dir, size_t index);
int min(int val1, int val2);


/**
 * entry point
 */
int main (int argc, char *argv[]) {

   uint16_t width, height, fps = 25, bit_depth = 8;
   uint32_t audio_buffer_size, pixel_buffer_size;
   char *audio_path;
   char *output_dir = "";

   size_t audio_frames, video_frames;

   SNDFILE *audio_file;
   SF_INFO info;

   if (argc<5) {
      printf("Usage: %s <audio file> <width> <height> <fps> [<ouput dir>]\n", argv[0]);
      return(EXIT_FAILURE);
   }

   audio_path = argv[1];

   if (argc>=6) {
      if (strlen(argv[5]) > 255) {
          puts("output_dir too long\n");
          return(EXIT_FAILURE);
      }
      strcpy(output_dir, argv[5]);
      if(access(output_dir, W_OK) != 0) {
         printf("output_dir '%s' is not accessible\n", output_dir);
         return(EXIT_FAILURE);
      }
      printf("strlen(output_dir) = %lu", strlen(output_dir));
   }

   if (argc>=4) {
      fps = atoi(argv[4]);
   }

   width  = atoi(argv[2]);
   height = atoi(argv[3]);

   if (width<MINWIDTH || width>MAXWIDTH || height<MINHEIGHT || height>MAXHEIGHT) {
      printf("width must be beetween %d and %d, height between %d %d\n", MINWIDTH, MAXWIDTH, MINHEIGHT, MAXHEIGHT);
      return(EXIT_FAILURE);
   }

   if (fps<MINFPS || fps>MAXFPS) {
      printf("frames per second must be between %d and %d\n", MINFPS, MAXFPS );
      return(EXIT_FAILURE);
   }

   memset(&info, 0, sizeof(info));

   // open soundfile
   audio_file = sf_open(audio_path, SFM_READ, &info);
   if (audio_file == NULL) {
       printf("failed to open file '%s' : \n%s\n", audio_path, sf_strerror (NULL));
       return(EXIT_FAILURE);
   }

   if (info.channels < 1) {
      puts("no audio channels.");
      return(EXIT_FAILURE);
   }

   // allocate buffer
   audio_frames = floor(info.samplerate / fps);
   audio_buffer_size = info.channels * audio_frames;
   pixel_buffer_size = width * height * BYTES_PER_PIXEL;

   if (audio_buffer_size > pixel_buffer_size) {
      printf(
         "audiobuffer (%d) greater than pixel buffer (%d). can't handle this atm.\n",
         audio_buffer_size,
         pixel_buffer_size
      );
      printf("samplerate: %d, channels: %d\n", info.samplerate, info.channels);
      return(EXIT_FAILURE);
   }


   video_frames = ceil(fps * info.frames / (int) info.samplerate);

   //printf("frames: %lu\n", info.frames);
   //printf("video_frames: %u, audioframes: %u, samplerate: %u\n", v_frames, info.frames, (int) info.samplerate);

   if (video_frames < 1) {
      puts("not enough samples for a video frame");
      return(EXIT_FAILURE);
   }

   s_dimension dimension = {
       width,
       height,
       audio_frames,
       (short)info.channels,
       bit_depth
   };

   for (size_t frame=0; frame<video_frames; frame++) {
      frame_loop(frame, output_dir, audio_file, dimension);
   }

   sf_close(audio_file);
   return(0);

}

/*
 * write audio block to png file
*/
void frame_loop(size_t frame, char* output_dir, SNDFILE* audio_file, s_dimension d) {

    size_t x, y;
    size_t area = d.width * d.height;
    size_t wsec = ceil(d.width / d.channels);
    sf_count_t req, cnt;
    sf_count_t copy_frames = ceil(area / d.channels);
    png_uint_32 i;
    png_infop info_ptr;
    png_structp png_ptr;
    png_color_8 sig_bit;
    png_byte  pixelbuffer[d.height][d.width * BYTES_PER_PIXEL];
    png_bytep row_pointers[d.height];
    short int audiobuffer[d.channels * area];

    char file_name[11 + strlen(output_dir)];

    // open file for writing
    set_filename(file_name, output_dir, frame);
    FILE *fp = fopen(file_name, "wb");
    if (!fp) {
        exit(ERROR);
    }
    printf("%s\n", file_name);

    // clear pixelbuffer
    memset(&pixelbuffer, 0, area * BYTES_PER_PIXEL);
    // assign row pointers
    for (i = 0; i < d.height; i++) {
        row_pointers[i] = &(pixelbuffer[i][0]);
    }

    req = (size_t)sf_seek(audio_file, d.audio_frames * frame, SEEK_SET);
    if (req == -1) {
        puts("[!] audiofile seek error");
        // skip frame
        return;
    }
    cnt = sf_readf_short(audio_file, audiobuffer, copy_frames);

    // copy block
    for (x=0; x<cnt; x++) {
        //pixelbuffer[ (int) floor(x / width) ][x % width] = (char) audiobuffer[x * channels];
        for(y=0; y<d.channels; y++) {
            size_t ix = min(floor(x / wsec), d.height-1);
            size_t iy = x % wsec + y * wsec;
            pixelbuffer[ix][iy] = abs((audiobuffer[x * d.channels + y]) / 128);
            //printf("%lu ", (audiobuffer[x * channels + y] + sizeof(short int)) / 2);
            //printf("%i ", abs((audiobuffer[x * channels + y]) / 128));
        }
        //printf("\n");
    }


    // generate png from pixelbuffer
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr) {
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        puts("png error");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    // write png
    png_init_io(png_ptr, fp);
    png_set_IHDR(
        png_ptr,
        info_ptr,
        d.width,
        d.height,
        d.bit_depth,
        PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    sig_bit.gray = d.bit_depth;
    png_set_sBIT(png_ptr, info_ptr, &sig_bit);

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

int min(int val1, int val2) {
   return val2 > val1 ? val1 : val2;
}

void set_filename(char* file_name, char* dir, size_t index) {

    size_t num = index + 1;

    if(strlen(dir) == 0) {
        sprintf(file_name, "f%06lu.png", num);
    } else if (strcmp(dir, "/") == 0) {
        sprintf(file_name, "/f%06lu.png", num);
    } else {
        sprintf(file_name, "%s/f%06lu.png", dir, num);
    }
}

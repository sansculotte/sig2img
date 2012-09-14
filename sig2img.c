/********************************************************************************
 * 
 * sig2img
 * write an audiobuffer as a sequence of image files
 * can be usefull as an audio-based texture in an 3D
 * application.
 * 062011 u@sansculotte.net
 * 
 * todo:
 * - put code-juice into modular functions
 * - what if audiobuffer > pixelbuffer? squeeze? truncate?
 * - distort an input image with soundbuffer
 * - float fps
 * - test compile and run on different systems
 */

#include <stdio.h>
#include <sndfile.h>
#include <png.h>
#include <math.h>

#define MINFPS 1
#define MAXFPS 1024
#define MAXWIDTH 8196
#define MAXHEIGHT 8196
#define MINWIDTH 40
#define MINHEIGHT 40 
#define MINBUFSIZE 64
#define MAXBUFSIZE 2^32
#define BYTES_PER_PIXEL 1

#define ERROR 1

/** entry point
 */
int main (int argc, char *argv[]) {

   int width, height, fps, audio_frames, audio_buffer_size, pixel_buffer_size;
   int frame;
   png_uint_32 i;
   char * audio_path, * output_dir="";

   int v_frames;

   SNDFILE *audio_file;
   SF_INFO info;

   if(argc<5) {
      printf("Usage: %s <audio file> <width> <height> <fps> [<ouput dir>]\n", argv[0]);
      return (ERROR);
   }

   audio_path = argv[1];

   if(argc>=6) {
      output_dir = argv[5];
      /*
      if(stat(output_dir, ) == -1) {
         printf("output_dir '%s' is anot accessible\n", output_dir);
         return (ERROR);
      }
      */
   }

   width = atoi(argv[2]);
   height = atoi(argv[3]);
   fps = atoi(argv[4]);

   if(width<MINWIDTH || width>MAXWIDTH || height<MINHEIGHT || height>MAXHEIGHT) {
      printf("width must be beetween %d and %d, height between %d %d\n", MINWIDTH, MAXWIDTH, MINHEIGHT, MAXHEIGHT );
      return (ERROR);
   }
   if (height > PNG_UINT_32_MAX/png_sizeof(png_bytep)) {
      puts ("Image is too tall to process in memory");
      return (ERROR);
   }

   if(fps<MINFPS || fps>MAXFPS) {
      printf("frames per second must be between %d and %d\n", MINFPS, MAXFPS );
      return (ERROR);
   }
   
	memset (&info, 0, sizeof (info)) ;

   // open soundfile
   audio_file = sf_open(audio_path, SFM_READ, &info); 
	if (audio_file == NULL) {
      printf ("failed to open file '%s' : \n%s\n", audio_path, sf_strerror (NULL)) ;
		return (ERROR) ;
	} 

   if(info.channels<1) {
      puts("no audio channels.");
      return (ERROR);
   }

   // allocate buffer
   audio_frames = floor(info.samplerate / fps);
   audio_buffer_size = info.channels * audio_frames;
   pixel_buffer_size = width*height*BYTES_PER_PIXEL;

   if (audio_buffer_size>pixel_buffer_size) {
      puts("audiobuffer greater than pixel buffer. can't handle this atm.");
      return (ERROR);
   }

   short int audiobuffer[audio_buffer_size]; 
   png_byte pixelbuffer[height][width*BYTES_PER_PIXEL]; //int?
   png_bytep row_pointers[height];

   // clear pixelbuffer
   memset(&pixelbuffer, 0, pixel_buffer_size); 

   // assign row pointers
   for (i = 0; i < height; i++) {
      //row_pointers[i] = pixelbuffer + i*width*BYTES_PER_PIXEL;
      row_pointers[i] = &(pixelbuffer[i][0]);
   }

   printf("row_pointers: %lu \npixelbuffer: %lu pixelrow: %lu pixel: %lu\n", sizeof(row_pointers) / sizeof(row_pointers[0]), sizeof(pixelbuffer), sizeof(pixelbuffer[0]), sizeof(pixelbuffer[0][0]));
   printf("%s", row_pointers[119]);
   //output_rows(width, height, row_pointers);

   v_frames = ceil(fps * (info.frames / (int) info.samplerate));

   if(v_frames < 1) {
      puts("not enough samples for a video frame");
      return(0);
   }

   char file_name[11+strlen(output_dir)]; 
   sf_count_t cnt;
   int bit_depth = 8;

   // frame loop
   for(frame=0; frame<v_frames; frame++) {

      png_infop info_ptr;
      png_structp png_ptr;
      int req;
      int x, y, wsec;
      png_color_8 sig_bit;

      if(strlen(output_dir)==0) {
         sprintf(file_name, "f%06i.png",  frame+1);
      } else {
         sprintf(file_name, "%s/f%06i.png", output_dir, frame+1);
      }
      printf("%s\n", file_name);

      // open file for writing
      FILE *fp = fopen(file_name, "wb");
      if (!fp)
         return (ERROR);

      req = (int) sf_seek(audio_file, audio_frames * frame, SEEK_SET);
//printf("req: %i\n", req);
      cnt = sf_readf_short(audio_file, audiobuffer, (sf_count_t) audio_frames);
//printf("cnt: %i\n", (int) cnt);

//printf("pixel_buffer_size: %d, audio_buffer_size: %d, pixel_buffer_size-audio_buffer_size: %d\n", pixel_buffer_size, audio_buffer_size, pixel_buffer_size-audio_buffer_size);
      // scroll pixelbuffer
      //memmove(pixelbuffer+audio_buffer_size, pixelbuffer, pixel_buffer_size-audio_buffer_size ); 
      for(x=(int)height-ceil((audio_frames*info.channels)/width); x>0; x--) {
         memcpy(row_pointers[x+(int)floor((audio_frames*info.channels)/width)-1], row_pointers[x-1], width); 
         //row_pointers[x+(int)ceil(audio_buffer_size*sizeof(short int)/width)] = row_pointers[x]; 
      }

      // insert new block
      //memcpy(pixelbuffer, audiobuffer, audio_buffer_size*sizeof(short int));
      wsec = (int) ceil(width/info.channels);
      for(x=0; x<cnt; x++) {
         //pixelbuffer[ (int) floor(x / width) ][x % width] = (char) audiobuffer[x * info.channels];
         for(y=0; y<info.channels; y++) {
            pixelbuffer[ (int) floor(x / wsec) ][x % wsec + y * wsec] = abs((audiobuffer[x * info.channels + y]) / 128);
            //printf("%lu ", (audiobuffer[x * info.channels + y] + sizeof(short int)) / 2);
            //printf("%i ", abs((audiobuffer[x * info.channels + y]) / 128));
         }
         //printf("\n"); 
      } 
      // generate png from pixelbuffer
      png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

      if (!png_ptr) {
         fclose(fp);
         return (ERROR);
      }

      info_ptr = png_create_info_struct(png_ptr);

      if (!info_ptr) {
         png_destroy_write_struct(&png_ptr, NULL);
         fclose(fp);
         return (ERROR);
      }

      if (setjmp(png_jmpbuf(png_ptr))) {
         puts("png error");
         png_destroy_write_struct(&png_ptr, &info_ptr);
         fclose(fp);
         return (ERROR);
      }

      // write png
      png_init_io(png_ptr, fp);
      png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
      sig_bit.gray = bit_depth;
      png_set_sBIT(png_ptr, info_ptr, &sig_bit);

      png_write_info(png_ptr, info_ptr);
      png_write_image(png_ptr, row_pointers);
      png_write_end(png_ptr, info_ptr);

      png_destroy_write_struct(&png_ptr, &info_ptr);
      fclose(fp);

   }

	sf_close (audio_file);
   return(0);

}

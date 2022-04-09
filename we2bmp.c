#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include <string.h>
#include "./gopt.h"


#ifdef __unix__
//unix
//  #include <unistd.h>
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_image.h>

#elif defined _WIN32 || defined _WIN64
//windows
    #include <Windows.h>
#else
#error "unknown platform"
#endif


const char *filename_input;
const char *filename_output;


#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#define BUGFIX_VERSION 0

//Screen dimension constants
int SCREEN_WIDTH = 256;
int SCREEN_HEIGHT = 512;

int TILE_WIDTH = 8;
int TILE_HEIGHT = 8;


void *options;
int verbosity=0;
int byte_mode=0;
int rgb_mode=0;

unsigned char swapNibbles(unsigned char x)
{
        return ( (x & 0x0F)<<4 | (x & 0xF0)>>4 );
}



#ifdef __unix__
void close_sdl()
{

        //Quit SDL subsystems
        IMG_Quit();
        SDL_Quit();
}
#endif



int main(int argc, const char **argv)
{



        options = gopt_sort( &argc, argv, gopt_start(
                                     gopt_option( 'h', 0, gopt_shorts( 'h' ), gopt_longs( "help" )),
                                     gopt_option( 'z', 0, gopt_shorts( 'z' ), gopt_longs( "version" )),
                                     gopt_option( 'p', 0, gopt_shorts( 'p' ), gopt_longs( "preview" )),


                                     gopt_option( 'v', GOPT_REPEAT, gopt_shorts( 'v' ), gopt_longs( "verbose" )),


                                     gopt_option( 'm', GOPT_ARG, gopt_shorts( 'm' ), gopt_longs( "imagex" )),
                                     gopt_option( 'n', GOPT_ARG, gopt_shorts( 'n' ), gopt_longs( "imagey" )),

                                     gopt_option( 'x', GOPT_ARG, gopt_shorts( 'x' ), gopt_longs( "tilex" )),
                                     gopt_option( 'y', GOPT_ARG, gopt_shorts( 'y' ), gopt_longs( "tiley" )),
                                     gopt_option( 'r', GOPT_ARG, gopt_shorts( 'r' ), gopt_longs( "rgbmode" )),

                                     gopt_option( 'd', GOPT_ARG, gopt_shorts( 'd' ), gopt_longs( "dest" )),
                                     gopt_option( 's', GOPT_ARG, gopt_shorts( 's' ), gopt_longs( "source" )),

                                     gopt_option( 'o', GOPT_ARG, gopt_shorts( 'o' ), gopt_longs( "output" )),
                                     gopt_option( 'i', GOPT_ARG, gopt_shorts( 'i' ), gopt_longs( "input" )) ));





        if( gopt( options, 'h' ) ) {


                fprintf( stdout, "Syntax (extraction): we2bmp [options] -i input.bin -o output.bmp\n");
                fprintf( stdout, "Syntax (injection):  we2bmp [options] -s output.bmp -d input.bin\n\n");

                fprintf( stdout, "we2bmp - a ngc winning eleven bitmap reinjector\n" );
                fprintf( stdout, "by saturnu <tt@anpa.nl>\n\n" );

                printf("-Extraction mode-\n");
                printf("Input: (required)\n");
                fprintf( stdout, " -i, --input=filename.bin\twe binary bitmap\n" );
                printf("Output: (optional)\n");
                fprintf( stdout, " -o, --output=filename.bmp\toutput bitmap\n" );

                printf("\n-Injection mode-\n");
                printf("Destination: (required)\n");
                fprintf( stdout, " -d, --dest=filename.bin\twe binary bitmap\n" );
                printf("Source: (required)\n");
                fprintf( stdout, " -s, --source=filename.bmp\tsource bitmap\n" );

                printf("\nOptions:\n");

                fprintf( stdout, " -x, --tilex\t\ttile x length\n" );
                fprintf( stdout, " -y, --tiley\t\ttile y length\n" );
                fprintf( stdout, " -m, --imagex\t\timage x length\n" );
                fprintf( stdout, " -n, --imagey\t\timage y length\n" );
                fprintf( stdout, " -r, --rgbmode\t\t(0=444 default) 1=555 2=565\n" );


                printf("\nInformation:\n");
                #ifdef __unix__
                fprintf( stdout, " -p, --preview\t\tenable sdl preview\n" );
                #endif
                fprintf( stdout, " -h, --help\t\tdisplay this help and exit\n" );
                fprintf( stdout, " -v, --verbose\t\tverbose\n" );
                fprintf( stdout, " -z, --version\t\tversion info\n" );

                return 0;
        }


        if( gopt( options, 'z' ) ) {

                fprintf( stdout, "we2bmp version v%d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, BUGFIX_VERSION );
                return 0;
        }


        if( (gopt( options, 'i' ) && gopt( options, 's' )) ) {
                printf("mode conflict!\n");
                return 0;
        }


        if( !gopt( options, 'i' ) && !gopt( options, 's' ) ) {
                //needed every time
                printf("input/source file missing! use 'we2bmp -h' for help\n");
                return 0;
        }

        // if( !gopt( options, 'o' ) ){
        //needed every time
//    printf("output file missing! use 'we2bmp -h' for help\n");
//   return 0;
//   }


        if(gopt( options, 'v' ) < 4)
                verbosity = gopt( options, 'v' );
        else verbosity = 3;

        if( verbosity > 1 )
                fprintf( stderr, "being really verbose\n" );

        else if( verbosity )
                fprintf( stderr, "being verbose\n" );


        //injection mode
        if( gopt_arg( options, 'd', &filename_output ) && strcmp( filename_output, "-" ) ) {

                if( !gopt( options, 's' ) ) {
                        //needed every time
                        printf("source file missing! use 'we2bmp -h' for help\n");
                        return 0;
                }

                //first get some info out of the binary destination file to gather file offsets

                FILE *dest_file;
                unsigned char *dest_buffer;
                unsigned long dest_fileLen;

                dest_file = fopen(filename_output, "rb");


                if (!dest_file)
                {
                        fprintf(stderr, "Unable to open file\n");
                        return 0;
                }

                //Get file length
                fseek(dest_file, 0L, SEEK_END);
                dest_fileLen=ftell(dest_file);
                fseek(dest_file, 0L, SEEK_SET);

                //Allocate memory
                if( verbosity >1)
                        printf("dest_fileLen %d\n",dest_fileLen );
                dest_buffer=(unsigned char *)malloc(dest_fileLen);

                //  memset(buffer,0x00,dest_fileLen+1);

                if (!dest_buffer)
                {
                        fprintf(stderr, "Memory error!\n");
                        fclose(dest_file);
                        return 0;
                }

                //Read file contents into buffer
                //  fread(buffer, dest_fileLen, 1, file);

                if (fread(dest_buffer, sizeof(dest_buffer[0]), dest_fileLen, dest_file) != dest_fileLen) {
                        printf("read error\n");
                }

                //don't close file yet, there is data to be written
                //fclose(dest_file);

                unsigned short image_width  = dest_buffer[0x14+1] | dest_buffer[0x14+0] << 8;
                unsigned short image_height = dest_buffer[0x16+1] | dest_buffer[0x16+0] << 8;

                SCREEN_WIDTH = image_width;
                SCREEN_HEIGHT = image_height;



                const char *iwidth_str;

                if( gopt_arg( options, 'm', &iwidth_str ) && strcmp( iwidth_str, "-" ) ) {
                        SCREEN_WIDTH=atoi(iwidth_str);
                        if( verbosity >0)
                                printf("SCREEN_WIDTH %d (overwritten)\n", SCREEN_WIDTH);
                }
                else{
                        if( verbosity >0)
                                printf("SCREEN_WIDTH %d (default)\n", SCREEN_WIDTH);
                }

                const char *iheight_str;

                if( gopt_arg( options, 'n', &iheight_str ) && strcmp( iheight_str, "-" ) ) {
                        SCREEN_HEIGHT=atoi(iheight_str);
                        if( verbosity >0)
                                printf("SCREEN_HEIGHT %d (overwritten)\n", SCREEN_HEIGHT);
                }
                else{
                        if( verbosity >0)
                                printf("SCREEN_HEIGHT %d (default)\n", SCREEN_HEIGHT);
                }





                //check for byte or half byte image
                int diff = abs( (dest_fileLen - (SCREEN_HEIGHT*SCREEN_WIDTH)) );

                if (diff > dest_fileLen/2) {
                        if( verbosity >0)
                                printf("half byte mode\n");
                        TILE_WIDTH = 8;
                        TILE_HEIGHT = 8;

                }else{
                        if( verbosity >0)
                                printf("full byte mode\n");
                        TILE_WIDTH = 8;
                        TILE_HEIGHT = 4;

                        byte_mode=1;
                }


                const char *width_str;

                if( gopt_arg( options, 'x', &width_str ) && strcmp( width_str, "-" ) ) {
                        TILE_WIDTH=atoi(width_str);
                        if( verbosity >0)
                                printf("TILE_WIDTH %d (overwritten)\n", TILE_WIDTH);
                }
                else{
                        if( verbosity >0)
                                printf("TILE_WIDTH %d (default)\n", TILE_WIDTH);
                }

                const char *height_str;

                if( gopt_arg( options, 'y', &height_str ) && strcmp( height_str, "-" ) ) {
                        TILE_HEIGHT=atoi(height_str);
                        if( verbosity >0)
                                printf("TILE_HEIGHT %d (overwritten)\n", TILE_HEIGHT);
                }
                else{
                        if( verbosity >0)
                                printf("TILE_HEIGHT %d (default)\n", TILE_HEIGHT);
                }







                int injection_offset=0;
                int source_offset=0;
                if(byte_mode==0) {
                        source_offset=0x36+0x40; //0x36 header + 16 colors * 4 byte = 0x40 ->>> 118
                        injection_offset=0x60;
                }else{
                        source_offset=0x36+0x400;
                        injection_offset=0x240;
                }

                //now we have the image offsets
                //injection_offset source_offset

                //let's get the source file

                if( gopt_arg( options, 's', &filename_input ) && strcmp( filename_input, "-" ) ) {
                        if( verbosity >1)
                                printf("loading src file\n");
                }

                FILE *src_file;
                unsigned char *src_buffer;
                unsigned long src_fileLen;

                src_file = fopen(filename_input, "rb");


                if (!src_file)
                {
                        fprintf(stderr, "Unable to open file\n");
                        return 0;
                }

                //Get file length
                fseek(src_file, 0L, SEEK_END);
                src_fileLen=ftell(src_file);
                fseek(src_file, 0L, SEEK_SET);

                //Allocate memory
                if( verbosity >1)
                        printf("src_fileLen %d\n",src_fileLen );
                src_buffer=(unsigned char *)malloc(src_fileLen);

                //  memset(buffer,0x00,src_fileLen+1);

                if (!src_buffer)
                {
                        fprintf(stderr, "Memory error!\n");
                        fclose(src_file);
                        return 0;
                }

                //Read file contents into buffer
                //  fread(buffer, dest_fileLen, 1, src_file);

                if (fread(src_buffer, sizeof(src_buffer[0]), src_fileLen, src_file) != src_fileLen) {
                        printf("read error\n");
                }

                //buffer in memory is enough
                fclose(src_file);

                //TODO
                //de-mirror bitmap
                //mirror lines for bitmap format

                int output_buffer_size=0;

                if(byte_mode==0) {
                        output_buffer_size=(SCREEN_WIDTH*SCREEN_HEIGHT)/2;
                }
                else
                if(byte_mode==1) {
                        output_buffer_size=(SCREEN_WIDTH*SCREEN_HEIGHT);
                }




                unsigned char img_serial[output_buffer_size]; //use one byte per pixel - double space needed
                memset(img_serial,0x00,output_buffer_size);

                unsigned char img_tiles[output_buffer_size];   //use one byte per pixel - double space needed
                memset(img_tiles,0x00,output_buffer_size);


                int ya=0;
                int xa=0;
                int p_=0;


                for(ya=SCREEN_HEIGHT-1; ya>=0; ya--) { //264->0

                        if(byte_mode==0) {
                                for(xa=0; xa<SCREEN_WIDTH/2; xa++) {
                                        //swapped later somehow?
                                        //img_serial[p_]=swapNibbles(src_buffer[source_offset+ya*(SCREEN_WIDTH/2)+xa]);
                                        img_serial[p_]=src_buffer[source_offset+ya*(SCREEN_WIDTH/2)+xa];

                                        p_++;


                                }
                        }else if(byte_mode==1) {
                                for(xa=0; xa<SCREEN_WIDTH; xa++) {
                                        //img_serial_real_mirror_out[p_]=swapNibbles(img_serial_real_out[ya*SCREEN_WIDTH+xa]);
                                        img_serial[p_]=src_buffer[source_offset+ya*SCREEN_WIDTH+xa];

                                        p_++;


                                }

                        }

                }



//both demirror files are ok full byte and half byte



                int ty=0;
                int tx=0;
                int th=0;
                int tw=0;

                int g=0;
                //  p_=0;

//bring the bitmap to tile order


                if(byte_mode==0) {
                        SCREEN_WIDTH/=2;
                        TILE_WIDTH/=2;


                        for(ty=0; ty<SCREEN_HEIGHT/TILE_HEIGHT; ty++) {
                                for(tx=0; tx<SCREEN_WIDTH/TILE_WIDTH; tx++) {


                                        for(th=0; th<TILE_HEIGHT; th++) {
                                                for(tw=0; tw<TILE_WIDTH; tw++) {
                                                        img_tiles[g++]=img_serial[ty*TILE_HEIGHT*SCREEN_WIDTH + th*SCREEN_WIDTH + tx*TILE_WIDTH+tw];
                                                }
                                        }

                                }
                        }



                }else if(byte_mode==1) {

                        for(ty=0; ty<SCREEN_HEIGHT/TILE_HEIGHT; ty++) {
                                for(tx=0; tx<SCREEN_WIDTH/TILE_WIDTH; tx++) {


                                        for(th=0; th<TILE_HEIGHT; th++) {
                                                for(tw=0; tw<TILE_WIDTH; tw++) {
                                                        img_tiles[g++]=img_serial[ty*TILE_HEIGHT*SCREEN_WIDTH + th*SCREEN_WIDTH + tx*TILE_WIDTH+tw];
                                                }
                                        }

                                }
                        }

                }




//test
/*
                printf("byte_mode=%d\n",byte_mode);

                int t=0;
                for(t=0x00; t<0x600; t++){

                  printf("[%03x]%02x ",t,img_tiles[t]);
                }
 */

                //write to the destination file and close it




                fclose(dest_file);


                FILE *write_ptr;

                char str[256] = {0};
                snprintf(str, sizeof(str), "patched_%s",  basename( (char *)filename_output));

                write_ptr = fopen(str,"w+b");           // w for write, b for binary

                fwrite(dest_buffer,sizeof(dest_buffer[0]), injection_offset,write_ptr);
                fwrite(&img_tiles,sizeof(img_tiles[0]),sizeof(img_tiles),write_ptr);

                fclose(write_ptr);



        }





        //extraction mode
        if( gopt_arg( options, 'i', &filename_input ) && strcmp( filename_input, "-" ) ) {

                FILE *file;
                unsigned char *buffer;
                unsigned long fileLen;

                file = fopen(filename_input, "rb");


                if (!file)
                {
                        fprintf(stderr, "Unable to open file\n");
                        return 0;
                }

                //Get file length
                fseek(file, 0L, SEEK_END);
                fileLen=ftell(file);
                fseek(file, 0L, SEEK_SET);

                //Allocate memory
                if( verbosity >1)
                        printf("fileLen %d\n",fileLen );
                buffer=(unsigned char *)malloc(fileLen);

                //  memset(buffer,0x00,fileLen+1);

                if (!buffer)
                {
                        fprintf(stderr, "Memory error!\n");
                        fclose(file);
                        return 0;
                }

                //Read file contents into buffer
                //  fread(buffer, fileLen, 1, file);

                if (fread(buffer, sizeof(buffer[0]), fileLen, file) != fileLen) {
                        printf("read error\n");
                }


                fclose(file);


                /*
                   normal 256*256=65536 65536/2) 32768+118(header) = 32886 <- filesize

                   every half byte is an ind. pixel
                   bits [4][4] [4][4] [4][4]

                 */



/*
   about image size
   - 1. extract sizes from binary
   - 2. overwrite defaults
   - 3. check for parameter overwrites
 */

                unsigned short image_width  = buffer[0x14+1] | buffer[0x14+0] << 8;
                unsigned short image_height = buffer[0x16+1] | buffer[0x16+0] << 8;

                SCREEN_WIDTH = image_width;
                SCREEN_HEIGHT = image_height;



                const char *iwidth_str;

                if( gopt_arg( options, 'm', &iwidth_str ) && strcmp( iwidth_str, "-" ) ) {
                        SCREEN_WIDTH=atoi(iwidth_str);
                        if( verbosity >0)
                                printf("SCREEN_WIDTH %d (overwritten)\n", SCREEN_WIDTH);
                }
                else{
                        if( verbosity >0)
                                printf("SCREEN_WIDTH %d (default)\n", SCREEN_WIDTH);
                }

                const char *iheight_str;

                if( gopt_arg( options, 'n', &iheight_str ) && strcmp( iheight_str, "-" ) ) {
                        SCREEN_HEIGHT=atoi(iheight_str);
                        if( verbosity >0)
                                printf("SCREEN_HEIGHT %d (overwritten)\n", SCREEN_HEIGHT);
                }
                else{
                        if( verbosity >0)
                                printf("SCREEN_HEIGHT %d (default)\n", SCREEN_HEIGHT);
                }



                const char *bgr_str;

                if( gopt_arg( options, 'r', &bgr_str ) && strcmp( bgr_str, "-" ) ) {
                        rgb_mode=atoi(bgr_str);
                        if( verbosity >0)
                                printf("bgr_str %d (overwritten)\n", rgb_mode);
                }
                else{
                        if( verbosity >0)
                                printf("bgr_str %d (default)\n", rgb_mode);
                }







                //check for byte or half byte image
                int diff = abs( (fileLen - (SCREEN_HEIGHT*SCREEN_WIDTH)) );

                if (diff > fileLen/2) {
                        if( verbosity >0)
                                printf("half byte mode\n");
                        TILE_WIDTH = 8;
                        TILE_HEIGHT = 8;

                }else{
                        if( verbosity >0)
                                printf("full byte mode\n");
                        TILE_WIDTH = 8;
                        TILE_HEIGHT = 4;

                        byte_mode=1;
                }




                const char *width_str;

                if( gopt_arg( options, 'x', &width_str ) && strcmp( width_str, "-" ) ) {
                        TILE_WIDTH=atoi(width_str);
                        if( verbosity >0)
                                printf("TILE_WIDTH %d (overwritten)\n", TILE_WIDTH);
                }
                else{
                        if( verbosity >0)
                                printf("TILE_WIDTH %d (default)\n", TILE_WIDTH);
                }

                const char *height_str;

                if( gopt_arg( options, 'y', &height_str ) && strcmp( height_str, "-" ) ) {
                        TILE_HEIGHT=atoi(height_str);
                        if( verbosity >0)
                                printf("TILE_HEIGHT %d (overwritten)\n", TILE_HEIGHT);
                }
                else{
                        if( verbosity >0)
                                printf("TILE_HEIGHT %d (default)\n", TILE_HEIGHT);
                }



/*
                 typedef struct tagBITMAPFILEHEADER {    // bmfh
                     UINT    bfType;
                     DWORD   bfSize;
                     UINT    bfReserved1;
                     UINT    bfReserved2;
                     DWORD   bfOffBits;
                     } BITMAPFILEHEADER;

                 typedef struct tagBITMAPINFOHEADER {    // bmih
                     DWORD   biSize;
                     LONG    biWidth;
                     LONG    biHeight;
                     WORD    biPlanes;
                     WORD    biBitCount;
                     DWORD   biCompression;
                     DWORD   biSizeImage;
                     LONG    biXPelsPerMeter;
                     LONG    biYPelsPerMeter;
                     DWORD   biClrUsed;
                     DWORD   biClrImportant;
                     } BITMAPINFOHEADER;
 */



                //create bitmap header

                //0x36+0x40=0x76 -> 4bit
                //0x36+0x200=0x236 -> 8bit
                int header_sz=0;
                if(byte_mode==0) {
                        header_sz=0x36+0x40; //0x36 header + 16 colors * 4 byte = 0x40 ->>> 118
                }else{
                        header_sz=0x36+0x400;
                }

                unsigned char bitmap_header[header_sz]; //0x75 + '/0'
                memset(bitmap_header,0x00,header_sz);

                bitmap_header[0]='B'; //magic byte
                bitmap_header[1]='M';

                //  bitmap_header[2]=0x76; //02-05 fs 0x02-0x5 8076 -> 32886 -> filesize correct
                //  bitmap_header[3]=0x80;


                int image_filesize=SCREEN_WIDTH*SCREEN_HEIGHT;
                int image_size=image_filesize;

                if(byte_mode==0) {
                        image_filesize/=2;
                        image_filesize+=header_sz;
                }
                else if(byte_mode==1) {
                        image_filesize+=header_sz;
                }

                if( verbosity >1)
                        printf("ifs %d %0x\n",image_filesize,image_filesize);



                bitmap_header[2]=image_filesize;
                bitmap_header[3]=image_filesize>>8; //02-05 fs 0x02-0x5 8076 -> 32886 -> filesize correct
                bitmap_header[4]=image_filesize>>16;
                bitmap_header[5]=image_filesize>>24;

/*
                bitmap_header[2]=0x76;
                bitmap_header[3]=0x02; //02-05 fs 0x02-0x5 8076 -> 32886 -> filesize correct
                bitmap_header[4]=0x02;
 */

                //fixme
                bitmap_header[0xa]=header_sz; //offset image
                bitmap_header[0xb]=header_sz>>8;
                bitmap_header[0xc]=header_sz>>16;
                bitmap_header[0xd]=header_sz>>24;

                //0xe-0x11 -> 28 -> windows bmp header size
                bitmap_header[0xe]=0x28;


                //dimensions
                //0x12 width 0x0100 -> 256dec
                //   bitmap_header[0x12]=0x00;
                //   bitmap_header[0x13]=0x01;

                //0x16 height 0x0100 -> 256dec
                //   bitmap_header[0x16]=0x00;
                //   bitmap_header[0x17]=0x01;

                //dimensions


                //0x12 width 0x0100 -> 256dec
                bitmap_header[0x12]=buffer[0x14+1];
                bitmap_header[0x13]=buffer[0x14+0];

                //0x16 height 0x0100 -> 256dec
                bitmap_header[0x16]=buffer[0x16+1];
                bitmap_header[0x17]=buffer[0x16+0];


                //0x1a color planes always 1 used by old pcx std
                bitmap_header[0x1a]=0x01;


                //0x1c
                if(byte_mode==0)
                        bitmap_header[0x1c]=0x04; //color depth 4bit half byte color 16
                else if(byte_mode==1)
                        bitmap_header[0x1c]=0x08; //color depth 8bit one byte color 256

                //0xf (4) compression -> none
                bitmap_header[0x1e]=0x00;

                //0x22 image size -> 0x8000 -> 32768
//        bitmap_header[0x22]=0x00;
//        bitmap_header[0x23]=0x80;

                //0x22 image size -> 0x8000 -> 32768
                //  bitmap_header[0x22]=0x00;
                //  bitmap_header[0x23]=0x00;
                //  bitmap_header[0x24]=0x02;

                bitmap_header[0x22]=image_size;
                bitmap_header[0x23]=image_size>>8; //02-05 fs 0x02-0x5 8076 -> 32886 -> filesize correct
                bitmap_header[0x24]=image_size>>16;
                bitmap_header[0x25]=image_size>>24;

                //0x26 resolutions -> 0x0ec4
                //printer display settings - don't care add some garbage?
                bitmap_header[0x26]=0xc4;
                bitmap_header[0x27]=0x0e;
                bitmap_header[0x2a]=0xc4;
                bitmap_header[0x2b]=0x0e;

                bitmap_header[0x2e]=0x00; //if 4bit (0x1c=4/8) -> pallet size -> 0x00 -> full 16/256 colors
                                          //so we keep it zero
                bitmap_header[0x32]=0x00; //if 4bit (0x1c=4/8) -> value of used colors -> 0x00 all colors used


                //0x37 start color palette

                //copy platte from binary to bmp-header -> 0x10*4 (64 dec) or 0x200 - 512 dec bytes


                int table_size=0;

                if(byte_mode==0) {
                        //118-22 image start
                        table_size=0x40;
                        //16 colors - one word per color

                }
                else{
                        // 0x40->0x240 0x200=512 byte color table
                        table_size=0x400; //4byte table in bitmap 0x400, 2byte table in bin 0x200

                        //one word (two byte) per color
                }



                if( verbosity >0)
                        printf("table_size %d\n",table_size);

                int color_pos=0;
                int table_pos=0;
                for(table_pos=0; table_pos<table_size; table_pos+=4) { //0x76-0x36 -> 16 colors


                        if(rgb_mode==0) {
                                //rgb444 masks
                                unsigned short red_mask = 0xF00;
                                unsigned short green_mask = 0xF0;
                                unsigned short blue_mask = 0xF;


                                //rgb555 masks
                                /*
                                   unsigned short  red_mask = 0x7C00;
                                   unsigned short  green_mask = 0x3E0;
                                   unsigned short  blue_mask = 0x1F;
                                 */


                                unsigned short color = buffer[0x40+color_pos+1] | buffer[0x40+color_pos+0] << 8;
                                //  0000 RRRR  GGGG BBBB

                                //printf("color %x\n",color);
                                color_pos+=2; //next color
//printf("%x %x\n",0x40+color_pos,color);

                                //rgb444
                                unsigned char red_value = (color & red_mask) >> 8;
                                unsigned char green_value = (color & green_mask) >> 4;
                                unsigned char blue_value = (color & blue_mask);



                                // Expand to 8-bit values.
                                //rgb444
                                unsigned char red   = red_value << 4;
                                unsigned char green = green_value << 4;
                                unsigned char blue  = blue_value << 4;

                                /*
                                   char red   = red_value << 3;
                                   char green = green_value << 2; //565 = 2 //555 = 3
                                   char blue  = blue_value << 3;
                                 */

/*
                         typedef struct tagRGBQUAD {     / rgbq
                             BYTE    rgbBlue;
                             BYTE    rgbGreen;
                             BYTE    rgbRed;
                             BYTE    rgbReserved;
                             } RGBQUAD;
 */

                                //working 4bit

                                bitmap_header[table_pos+0x36+0]=red;
                                bitmap_header[table_pos+0x36+1]=green;
                                bitmap_header[table_pos+0x36+2]=blue;
                                bitmap_header[table_pos+0x36+3]=0;

/*
                        bitmap_header[table_pos+0x36+0]=blue;
                        bitmap_header[table_pos+0x36+1]=green;
                        bitmap_header[table_pos+0x36+2]=red;
                        bitmap_header[table_pos+0x36+3]=0;
 */

/*
                        bitmap_header[table_pos+0x36+0]=0;
                        bitmap_header[table_pos+0x36+1]=red;
                        bitmap_header[table_pos+0x36+2]=green;
                        bitmap_header[table_pos+0x36+3]=blue;
 */
//printf("red %02x green %02x blue %02x\n", red, green, blue);

//printf("%x %02x%02x%02x%02x\n",table_pos+0x36+3,bitmap_header[table_pos+0x36+0],bitmap_header[table_pos+0x36+1],bitmap_header[table_pos+0x36+2],bitmap_header[table_pos+0x36+3]);

                                //  unsigned int RGB888 = red << 16 | green << 8 | blue;

                                //  printf("RGB888 %04x\n",RGB888);
                        }  else
                        if(rgb_mode==1) {
                                unsigned short red_mask = 0x7C00; //565 F800 - 555 7C00
                                unsigned short green_mask = 0x3E0;//565 07E0 - 555 03E0
                                unsigned short blue_mask = 0x1F; //565 1F - 555 1F
                                unsigned short color = buffer[0x40+color_pos+1] | buffer[0x40+color_pos+0] << 8;
                                color_pos+=2;
                                //rgb444
                                unsigned char red_value = (color & red_mask) >> 10; //555 10 565 11
                                unsigned char green_value = (color & green_mask) >> 5;
                                unsigned char blue_value = (color & blue_mask);

                                // Expand to 8-bit values.
                                unsigned char red   = red_value << 3;
                                unsigned char green = green_value << 3; //565 = 2 //555 = 3
                                unsigned char blue  = blue_value << 3;

                                bitmap_header[table_pos+0x36+0]=red;
                                bitmap_header[table_pos+0x36+1]=green;
                                bitmap_header[table_pos+0x36+2]=blue;
                                bitmap_header[table_pos+0x36+3]=0;

                        } else
                        if(rgb_mode==2) {
                                unsigned short red_mask = 0xF800; //565 F800 - 555 7C00
                                unsigned short green_mask = 0x7E0;//565 07E0 - 555 03E0
                                unsigned short blue_mask = 0x1F; //565 1F - 555 1F
                                unsigned short color = buffer[0x40+color_pos+1] | buffer[0x40+color_pos+0] << 8;
                                color_pos+=2;
                                //rgb444
                                unsigned char red_value = (color & red_mask) >> 11; //555 10 565 11
                                unsigned char green_value = (color & green_mask) >> 5;
                                unsigned char blue_value = (color & blue_mask);

                                // Expand to 8-bit values.
                                unsigned char red   = red_value << 3;
                                unsigned char green = green_value << 2; //565 = 2 //555 = 3
                                unsigned char blue  = blue_value << 3;

                                bitmap_header[table_pos+0x36+0]=red;
                                bitmap_header[table_pos+0x36+1]=green;
                                bitmap_header[table_pos+0x36+2]=blue;
                                bitmap_header[table_pos+0x36+3]=0;

                        }
                }


                int output_buffer_size=0;

                if(byte_mode==0) {
                        output_buffer_size=(SCREEN_WIDTH*SCREEN_HEIGHT)/2;
                }
                else
                if(byte_mode==1) {
                        output_buffer_size=(SCREEN_WIDTH*SCREEN_HEIGHT);
                }




                unsigned char img_serial[(SCREEN_WIDTH*SCREEN_HEIGHT)]; //use one byte per pixel - double space needed
                unsigned char img_serial_real[(SCREEN_WIDTH*SCREEN_HEIGHT)];
                unsigned char img_serial_real_out[output_buffer_size]; //use half byte as pixel - for bmp
                memset(img_serial,0x00,(SCREEN_WIDTH*SCREEN_HEIGHT));
                memset(img_serial_real,0x00,(SCREEN_WIDTH*SCREEN_HEIGHT));
                memset(img_serial_real_out,0x00,output_buffer_size);

                int i;
                int p=0;



                int start_offset=0;
                if(byte_mode==0) {
                        //118-22 image start
                        start_offset=0x60;
                        //16 colors - one word per color

                }
                else{
                        // 0x40->0x240 0x200=512 byte color table
                        start_offset=0x240;

                        //one word (two byte) per color
                }




                for(i = start_offset; i < fileLen; i++) { //-22 binary file is 22 bytes shorter as the bitmap   ----- 0x60 looks right 8+1 black/b /pixel


                        unsigned char theByte = ((unsigned char *)buffer)[i];

                        //printf(" raw [x][x] %02x\n",theByte);

                        //if set pixel n+1
                        if(byte_mode==0) { //half byte mode
                                if ((theByte & 0x10) || (theByte & 0x20) || (theByte & 0x40) || (theByte & 0x80)) {
                                        img_serial_real[p]=theByte & ~0xF0;
                                        img_serial[p++]=0x01; //printf("B");

                                }
                                else{
                                        img_serial_real[p]=theByte & ~0xF0;
                                        p++; //printf("b");
                                }

                                //printf(" add [ ][x] %02x\n",img_serial_real[p-1]);

                                //if set pixel n
                                if ((theByte & 0x1) || (theByte & 0x2) || (theByte & 0x4) || (theByte & 0x8)) {
                                        img_serial_real[p]=(theByte &= ~0x0F) >>4; //ok first nibble
                                        img_serial[p++]=0x01; //printf("A");
                                }
                                else{
                                        img_serial_real[p]=(theByte &= ~0x0F) >>4;
                                        p++; //printf("a");
                                }
                        }//byte_mode 0
                        if(byte_mode==1) { //full byte mode
                                img_serial_real[p]=(theByte);

                                if(theByte!=0)
                                        img_serial[p]=0x1;

                                p++;
                        }

                        //printf(" add [x][ ] %02x\n",img_serial_real[p-1]);


                }


                char tile_buffer[(SCREEN_HEIGHT*SCREEN_WIDTH)/(TILE_HEIGHT*TILE_WIDTH)][TILE_HEIGHT*TILE_WIDTH];
                char tile_buffer_real[(SCREEN_HEIGHT*SCREEN_WIDTH)/(TILE_HEIGHT*TILE_WIDTH)][TILE_HEIGHT*TILE_WIDTH];


                memset(tile_buffer, 0, sizeof(tile_buffer[0][0]) * SCREEN_WIDTH * SCREEN_HEIGHT);
                memset(tile_buffer_real, 0, sizeof(tile_buffer_real[0][0]) * SCREEN_WIDTH * SCREEN_HEIGHT);



                int c=0, t=0, inner_tp=0;

                for(c=0; c < SCREEN_HEIGHT*SCREEN_WIDTH; c++) {


                        if(!(c%(TILE_HEIGHT*TILE_WIDTH)) ) {

                                if(c!=0) t++;
                                inner_tp=0;
                        }

                        //printf("tile_buffer[%d][%d]=img_serial[%d] (%d);\n",t,inner_tp,c,img_serial[c]);
                        tile_buffer[t][inner_tp]=img_serial[c];
                        tile_buffer_real[t][inner_tp]=img_serial_real[c];


                        inner_tp++;

                }



                //sdl swag
                #ifdef __unix__
                SDL_Window *window;
                SDL_Renderer *renderer;



                if (gopt( options, 'p' )) {

                        SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
                        SDL_RenderClear( renderer );
                }
                #endif


                //"draw pixels"
                //Probably on a loop


                int ty=0;
                int tx=0;
                int tile=0;
                int position=0;
                unsigned char grid[SCREEN_WIDTH][SCREEN_HEIGHT];

                memset(grid, 0, sizeof(grid[0][0]) * SCREEN_WIDTH * SCREEN_HEIGHT);

                for(ty=0; ty<SCREEN_HEIGHT/TILE_HEIGHT; ty++) {
                        for(tx=0; tx<SCREEN_WIDTH/TILE_WIDTH; tx++) {

                                //draw tile
                                int x=0, y=0;
                                for(y=0; y<TILE_HEIGHT; y++) {

                                        for(x=0; x<TILE_WIDTH; x++) {

                                          #ifdef __unix__

                                                if(gopt( options, 'p' )) {
                                                        if(tile_buffer[tile][position])
                                                                SDL_RenderDrawPoint(renderer, x+(tx*TILE_WIDTH), y+(ty*TILE_HEIGHT)); //Renders on middle of screen.
                                                }
                                          #endif

                                                grid[x+(tx*TILE_WIDTH)][y+(ty*TILE_HEIGHT)]=tile_buffer_real[tile][position];


                                                position++;
                                        }
                                }


                                tile++;
                                position=0;
                        }
                }


                //serialize grid

                int gy=0;
                int gx=0;
                int s_pos0=1;
                int s_pos1=0;
                unsigned char add=0;
                //int once=1;


                for(gy=0; gy<SCREEN_HEIGHT; gy++) {

                        for(gx=0; gx<SCREEN_WIDTH; gx++) {

                                /*
                                   if(!(s_pos0%2))
                                   printf("grid[ ][x] %02x\n",grid[gx][gy]);
                                   else
                                   printf("grid[x][ ] %02x\n",grid[gx][gy]);
                                 */

                                if(byte_mode==0) {
                                        if(!(s_pos0%2)) {

                                                //every second pixel
                                                unsigned char color_b = grid[gx][gy];
                                                add &= 0xF0;
                                                add |= (unsigned char)(color_b & 0x0F);

                                                //add 2 pixel byte
                                                img_serial_real_out[s_pos1++]=add;

                                                //printf(" add %02x\n",add);

                                        }else{
                                                //printf("grid %02x ",grid[gx][gy]);
                                                //every first pixel
                                                unsigned char color_a = grid[gx][gy];
                                                add &= 0x0F;
                                                add |= (unsigned char)((color_a ) << 4);

                                        }


                                        s_pos0++;
                                }else if(byte_mode==1) {
                                        img_serial_real_out[s_pos1++]=grid[gx][gy];


                                }
                        }

                }


/*
                int v=0;
                for(v=0; v<0x600;v++)
                  printf("[%03x]%02x ", v, img_serial_real_out[v]);
                //printf("\nimage size %d\n",sizeof(img_serial_real_out));
 */



                //mirror lines for bitmap format

                int ya=0;
                int xa=0;
                int p_=0;



                unsigned char img_serial_real_mirror_out[output_buffer_size]; //use half byte as pixel - for bmp
                memset(img_serial_real_mirror_out,0x00,output_buffer_size);

                for(ya=SCREEN_HEIGHT-1; ya>=0; ya--) { //264->0

                        if(byte_mode==0) {
                                for(xa=0; xa<SCREEN_WIDTH/2; xa++) {
                                        img_serial_real_mirror_out[p_]=swapNibbles(img_serial_real_out[ya*(SCREEN_WIDTH/2)+xa]);
                                        p_++;


                                }
                        }else if(byte_mode==1) {
                                for(xa=0; xa<SCREEN_WIDTH; xa++) {
                                        //img_serial_real_mirror_out[p_]=swapNibbles(img_serial_real_out[ya*SCREEN_WIDTH+xa]);
                                        img_serial_real_mirror_out[p_]=img_serial_real_out[ya*SCREEN_WIDTH+xa];
                                        //printf("%d-%d\n",p_,(ya*SCREEN_WIDTH+xa));
                                        p_++;


                                }

                        }

                }
//printf("\np size %d\n",p_-1);

/*
   int v=0;
   for(v=0; v< sizeof(img_serial_real_mirror_out);v++)
   printf("%02x ",img_serial_real_mirror_out[v]);
   printf("\nimage size %d\n",sizeof(img_serial_real_mirror_out));
 */
                if( gopt_arg( options, 'o', &filename_output ) && strcmp( filename_output, "-" ) ) {

                        FILE *write_ptr;

                        write_ptr = fopen(filename_output,"w+b"); // w for write, b for binary

                        fwrite(&bitmap_header,sizeof(bitmap_header[0]),sizeof(bitmap_header),write_ptr);
                        fwrite(&img_serial_real_mirror_out,sizeof(img_serial_real_mirror_out[0]),sizeof(img_serial_real_mirror_out),write_ptr);

//  fwrite(&img_serial_real_out,sizeof(img_serial_real_out[0]),sizeof(img_serial_real_out),write_ptr);


                        fclose(write_ptr);
                }





                //free file buffer
                free(buffer);

                #ifdef __unix__


                if( gopt( options, 'p' )) {

                        SDL_RenderPresent(renderer);


                        //Main loop flag
                        char quit = 0;

                        //Event handler
                        SDL_Event e;

                        //While application is running
                        while( !quit )
                        {
                                //Handle events on queue
                                while( SDL_PollEvent( &e ) != 0 )
                                {
                                        //User requests quit
                                        if( e.type == SDL_QUIT )
                                        {
                                                quit = 1;
                                        }
                                }

                        }

                        //Free resources and close SDL
                        close_sdl();
                }
                    #endif


        }

        return 0;
}

/*****************************************************************************/
/*                                                                           */
/* GBAconv 1.00                                                              */
/* Copyright (c) 2002-2017, Frederic Cambus                                  */
/* https://github.com/fcambus/gbaconv                                        */
/*                                                                           */
/* PCX to GBA Converter                                                      */
/*                                                                           */
/* Created:      2002-12-09                                                  */
/* Last Updated: 2017-02-07                                                  */
/*                                                                           */
/* GBAconv is released under the BSD 2-Clause license.                       */
/* See LICENSE file for details.                                             */
/*                                                                           */
/*****************************************************************************/

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

FILE *input_file;
unsigned char *input_file_buffer;
int input_file_size;
struct stat input_file_stat;

FILE *output_file;

struct pcx_header {
   char ID;
   char version;
   char encoding;
   char bits_per_pixel;
   short int x_min;
   short int y_min;
   short int x_max;
   short int y_max;
   short x_resolution;
   short y_resolution;
   char palette[48];
   char reserved;
   char number_of_bit_planes;
   short bytes_per_line;
   short palette_type;
   short x_screen_size;
   short y_screen_size;
   char filler[54];
} pcx_header;

unsigned char pcx_image_palette[768];
unsigned char *pcx_buffer;
int pcx_buffer_size;

int loop;

int current_byte;
int offset;
int run_count;
int run_position;



/*****************************************************************************/
/* Main                                                                      */
/*****************************************************************************/
int main (int argc, char *argv[]) {
   printf("-------------------------------------------------------------------------------\n");
   printf("      PCX to GBA Converter - GBAconv 1.00 (c) by Frederic Cambus 2002-2017\n");
   printf("-------------------------------------------------------------------------------\n\n");

   if (argc!=2) {
//      printf("USAGE: pcx2gba input.pcx output.inc array_name (Input File must be 8-bpp PCX)\n\n");
      printf("USAGE: pcx2gba input.pcx");// output.inc array_name (Input File must be 8-bpp PCX)\n\n");
      exit(0);
   }



/*****************************************************************************/
/* Load Input File                                                           */
/*****************************************************************************/
	
   stat (argv[1], &input_file_stat);
   input_file_size=input_file_stat.st_size;

   input_file_buffer=malloc(input_file_size);

   if (input_file_buffer==NULL) {
      printf("ERROR: Cannot allocate memory\n\n");
      exit(-1);
   }

   input_file=fopen(argv[1],"rb");
   if (input_file==NULL) {
      printf("ERROR: Cannot open file %s\n\n",argv[1]);
      exit(-1);
   }

   fread(input_file_buffer,input_file_size,1,input_file);
   fclose(input_file);

	int len = strlen(argv[1]);
	
	char refName[len];
	
	memcpy(refName, argv[1], len-4);
	refName[len] = '\0';
	
	char outfile[len-2];
	strcpy(outfile, refName);
	strcat(outfile, ".h");	

/*****************************************************************************/
/* Check that the file is a valid 8-bpp PCX                                  */
/*****************************************************************************/

   memcpy(&pcx_header,input_file_buffer,128);

   if (pcx_header.bits_per_pixel!=8) {
      printf("ERROR: Input File is not 8-bpp\n\n");
      exit(-1);
   }



/*****************************************************************************/
/* Uncompress RLE encoded PCX Input File                                     */
/*****************************************************************************/

   pcx_buffer_size=(pcx_header.x_max+1)*(pcx_header.y_max+1);
   pcx_buffer=malloc(pcx_buffer_size);

   while (loop<input_file_size-768-128) {
      current_byte=input_file_buffer[loop+128];

      if (current_byte>192) {
         run_count=current_byte-192;

         for (run_position=0;run_position<run_count;run_position++) {
            pcx_buffer[offset+run_position]=input_file_buffer[loop+128+1];
         }
         offset+=run_count;
         loop+=2;
      } else {
         pcx_buffer[offset]=current_byte;
         offset++;
         loop++;
      }
   }

   for (loop=0;loop<768;loop++) {
      pcx_image_palette[loop]=(input_file_buffer[input_file_size-768+loop]/8);
   }



/*****************************************************************************/
/* Create Output File                                                        */
/*****************************************************************************/
	
	output_file = fopen(outfile,"w");
	if (output_file == NULL) {
		printf("ERROR: Cannot create file %s\n\n", outfile);
		exit(-1);
	}
	
	printf("INPUT  FILE: %s (%ix%ix%i-bpp)\n",argv[1],pcx_header.x_max+1,pcx_header.y_max+1,pcx_header.bits_per_pixel);
	printf("OUTPUT FILE: %s\n\n",outfile);
	
	fprintf(output_file, "/*----------------------------------------------------------------------\n");
	fprintf(output_file, " * PCX to GBA Converter - GBAconv 1.00 (c) by Frederic Cambus 2002-2017 \n");
	fprintf(output_file, " *                                1.01 (c) quickmod by creek23 2018     \n");
	fprintf(output_file, " *----------------------------------------------------------------------*/\n");
	fprintf(output_file, "#define %s_WIDTH   %i\n"    , refName, pcx_header.x_max+1);
	fprintf(output_file, "#define %s_HEIGHT  %i\n\n\n", refName, pcx_header.y_max+1);
	
	fprintf(output_file, "const u16 %sData[] = {\n                    ", refName);
	int l_ctr = 0;
	int l_data = 0;
	int l_datasize = pcx_buffer_size/2;
	for (loop = 0;loop < l_datasize; ++loop) {
		l_data = (pcx_buffer[loop*2] | pcx_buffer[(loop*2)+1]<<8);
		if (l_data == 00) {
			fprintf(output_file,"0x0000");
		} else if (l_data < 16) {
			fprintf(output_file,"0x000%x",l_data);
		} else if (l_data < 256) {
			fprintf(output_file,"0x00%x",l_data);
		} else if (l_data < 4096) {
			fprintf(output_file,"0x0%x",l_data);
		} else {
			fprintf(output_file,"0x%x",l_data);
		}
		++l_ctr;
		if (loop == l_datasize-1) {
			fprintf(output_file, " ");
		} else {
			if (l_ctr == 10) {
				l_ctr = 0;
				fprintf(output_file, ",\n                    ");
			} else {
				fprintf(output_file, ", ");
			}
		}
	}
	
	fseek(output_file,ftell(output_file)-1,0);
	fprintf(output_file,"};\n\n");
	
	l_ctr = 0;
	fprintf(output_file,"const u16 %sPalette[] = {\n                    ", refName);
	for (loop = 0; loop < 256; ++loop) {
		l_data = (pcx_image_palette[loop*3] | pcx_image_palette[(loop*3)+1]<<5 | pcx_image_palette[(loop*3)+2]<<10);
		if (l_data == 00) {
			fprintf(output_file,"0x0000");
		} else if (l_data < 16) {
			fprintf(output_file,"0x000%x",l_data);
		} else if (l_data < 256) {
			fprintf(output_file,"0x00%x",l_data);
		} else if (l_data < 4096) {
			fprintf(output_file,"0x0%x",l_data);
		} else {
			fprintf(output_file,"0x%x",l_data);
		}
		++l_ctr;
		if (loop == 255) {
			fprintf(output_file, " ");
		} else {
			if (l_ctr == 10) {
				l_ctr = 0;
				fprintf(output_file, ",\n                    ");
			} else {
				fprintf(output_file, ", ");
			}
		}
	}
	
	fseek(output_file,ftell(output_file)-1,0);
	fprintf(output_file,"};\n");
	
/*****************************************************************************/
/* Terminate Program                                                         */
/*****************************************************************************/
	
	fclose(output_file);
	free(input_file_buffer);
	
	printf("Successfully created file %s\n\n", outfile);
	
	return (0);
}


/* Title: A custom uniform palette generator that fits my needings.
   Written by rlyeh ( retrodev.info@rlyeh http://www.retrodev.info ), (c) 2007.

   References:
   - http://en.wikipedia.org/wiki/List_of_palettes
   - http://www.compuphase.com/unifpal.htm

   License:

   Copyright (c) 2007, Mario Rodríguez Palomino.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

       * Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above copyright
         notice, this list of conditions and the following disclaimer in the
         documentation and/or other materials provided with the distribution.
       * Neither the name of the organization nor the names of its contributors
         may be used to endorse or promote products derived from this software
         without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY Mario Rodríguez Palomino "AS IS" AND ANY
   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL Mario Rodríguez Palomino BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*
An uniform palette is a palette designed to show many hicolor or truecolor images into a single palettized video mode. This rendered image loses colors obviously, but the trick works without altering the system palette. In fact, you have only to define the palette once in your setup.

I was working sometime ago about improving my own uniform palette, for those systems with a limited colour range display.

My current changes are:

- changing from a RGB 6x6x6 (216) colour cube (as seen in Websafe standard, or Macintosh palettes) to a RGB 8x8x4 (256) colour cube, which gives a full 256 colours spectrum range.

- adding three (full 256 ramp) gamma corrected tables for each R,G,B channel.

- balancing R,G,B values into better eye-looking values.

- compensating B component against R,G ones to give true grays.

truecolor, websafe, websafe dithered
truecolor, macintosh, macintosh dithered
truecolor, rgb332, rgb332 dithered
truecolor, my palette, my palette dithered
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define VERSION "Uniform palette generator, v0.1 by rlyeh (c) 2007"

#define GAMMA_DIV                       2.0
#define GAMMA_RED                       ((0.299 - 0.114) / GAMMA_DIV)
#define GAMMA_GREEN                     ((0.587 - 0.114) / GAMMA_DIV)
#define GAMMA_BLUE                      ((0.1141 - 0.114) / GAMMA_DIV)
#define GAMMA_STEPS                     256

#define COMPONENT_INTERPOLATION         0.86
#define COMPONENT_RED_BRIGHTNESS        0.299
#define COMPONENT_GREEN_BRIGHTNESS      0.587
#define COMPONENT_BLUE_BRIGHTNESS       0.114

#define VERBOSE 1

int main(int argc, char *argv[])
{
    FILE *fp;
    int i,r,g,b,entry;

    struct { unsigned char r,g,b; } gamma[256];
    struct { float r,g,b; } palette[256];

    printf(VERSION ". Build " __DATE__ " " __TIME__ "\n\n");

    if(argc != 2)
    {
        printf("Usage: %s outfile\n", argv[0]);
        exit(1);
    }


    // create custom gamma tables

    for(i=0; i < GAMMA_STEPS; i++)
    {
        gamma[i].r=(unsigned char)(255.0*pow( ((i * 256.0)/(GAMMA_STEPS-1)) /255.0, 1.0 - GAMMA_RED));

        gamma[i].g=(unsigned char)(255.0*pow( ((i * 256.0)/(GAMMA_STEPS-1)) /255.0, 1.0 - GAMMA_GREEN));

        gamma[i].b=(unsigned char)(255.0*pow( ((i * 256.0)/(GAMMA_STEPS-1)) /255.0, 1.0 - GAMMA_BLUE));

        if( VERBOSE ) printf("gamma #%-3d %3d %3d %3d\n", i, gamma[i].r, gamma[i].g, gamma[i].b);
    }

    // create custom colour tables

    for (i = 0; i < 256; i++)
    {
        int index = i;

        r = (index >> 5) & 0x7;
        g = (index >> 2) & 0x7;
        b = (index >> 0) & 0x3;

        //make a pure RGB332 colour
        palette[index].r = (r * 255.0) / 7.0;
        palette[index].g = (g * 255.0) / 7.0;
        palette[index].b = (b * 255.0) / 3.0;

        //fix blue component to give a few true greys
        palette[index].b = (b * 255.0) / 3.5;   //(2^3 / 2^2) = 2 -> 7.0 / 2 = 3.5

        //soft and blend components in non greys colours
        if((palette[index].r != palette[index].g) || (palette[index].g != palette[index].b) || (palette[index].r != palette[index].b))
        {
        float r1, g1, b1, media, mediar, mediag, mediab;

        r1 = palette[index].r;
        g1 = palette[index].g;
        b1 = palette[index].b;

        media = (r1 * COMPONENT_RED_BRIGHTNESS + g1 * COMPONENT_GREEN_BRIGHTNESS + b1 * COMPONENT_BLUE_BRIGHTNESS);

        if(!media) media = 1.0;

        mediar = ((r1) * (COMPONENT_INTERPOLATION) + ((g1 * b1) / media) * (1.0 - COMPONENT_INTERPOLATION) );
        mediag = ((g1) * (COMPONENT_INTERPOLATION) + ((r1 * b1) / media) * (1.0 - COMPONENT_INTERPOLATION) );
        mediab = ((b1) * (COMPONENT_INTERPOLATION) + ((r1 * g1) / media) * (1.0 - COMPONENT_INTERPOLATION) );

        if(mediar > 255.0 || mediag > 255.0 || mediab > 255.0)
        {
            printf("error: color overflow at r,g,b = %f %f %f (%d %d %d, %f)\n", r1,g1,b1,mediar,mediag,mediab,media);
            exit(1);
        }

        //apply gamma
        palette[index].r = gamma[(int)mediar].r;
        palette[index].g = gamma[(int)mediag].g;
        palette[index].b = gamma[(int)mediab].b;
        }
    }

    // make #255 colour a pure white
    //palette[255].r = palette[255].g = palette[255].b = 255.0;

    // make #255 colour (slighty white) more pure
    palette[255].r = (palette[255].r + 255.0) / 2.0;
    palette[255].g = (palette[255].g + 255.0) / 2.0;
    palette[255].b = (palette[255].b + 255.0) / 2.0;

    // save our palette to a Photoshop .act file

    {
    FILE *fp_act, *fp_h;
    char file_act[256], file_h[256];

    sprintf(file_act, "%s.act", argv[1]);
    sprintf(file_h,   "%s.h", argv[1]);

    fp_act = fopen(file_act, "wb");

    if(!fp_act)
    {
        printf("can't write file '%s' for writing\n", file_act);
        exit(1);
    }

    fp_h = fopen(file_h, "wb");

    if(!fp_h)
    {
        printf("can't write file '%s' for writing\n", file_h);
        exit(1);
    }

    fprintf(fp_h,"const unsigned char uniform_palette[] =\n{\n");

    for (i = 0; i < 256; )
    {
        unsigned char r1,g1,b1;

        r1 = palette[i].r;
        g1 = palette[i].g;
        b1 = palette[i].b;

        fputc(r1, fp_act);
        fputc(g1, fp_act);
        fputc(b1, fp_act);

        ++i;

        char str[256];
        sprintf(str, " 0x%02x,0x%02x,0x%02x%c%c", r1,g1,b1, i == 256 ? ' ' : ',', i % 4 ? ' ' : '\n');
        fprintf(fp_h, "%s", str );

        if( VERBOSE )
            printf("%s", str );
    }

    fprintf(fp_h,"};\n\n");

    fclose(fp_act);
    fclose(fp_h);
    }

    printf("Done!\n");

    return 0;
}

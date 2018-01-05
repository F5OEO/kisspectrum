/*
Raspberry Kiss Spectrum : KisSpectrum 
Copyright (c) 2017, Evariste F5OEO
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define PROGRAM_VERSION "0.0.1"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "VG/openvg.h"
#include "VG/vgu.h"
#include "fontinfo.h"
#include "shapes.h"
#include <math.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <liquid/liquid.h>

#define FFT_SIZE 1024

FILE *input;

 static char keep_running=0;
static void CleanAll()
{
}

static void signal_handler(int signo)
{
    if (signo == SIGINT)
        fputs("\nCaught SIGINT\n", stderr);
    else if (signo == SIGTERM)
        fputs("\nCaught SIGTERM\n", stderr);
    else if (signo == SIGHUP)
        fputs("\nCaught SIGHUP\n", stderr);
    else if (signo == SIGPIPE)
        fputs("\nReceived SIGPIPE.\n", stderr);
    else
        fprintf(stderr, "\nCaught signal: %d\n", signo);
    CleanAll();
    keep_running = 0;
}

static unsigned int m_ColorTbl[256];


 #define WALPHA(x)  (x << 24)
    #define WBLUE(x)   (x << 16)
    #define WGREEN(x)  (x << 8)
    #define WRED(x)    (x)

unsigned int setRgb(int r,int g,int b)
{
    return r|(g<<8)|(b<<16)|(0xFF<<24);
}

// Color Table Again Stolen from GQRX - Thx to Alex OZ9AEC - 
static void InitColorWaterFall()
{
// default waterfall color scheme
    for (int i = 0; i < 256; i++)
    {
        // level 0: black background
        if (i < 20)
            m_ColorTbl[i]=setRgb(0, 0, 0);
        // level 1: black -> blue
        else if ((i >= 20) && (i < 70))
            m_ColorTbl[i]=setRgb(0, 0, 140*(i-20)/50);
        // level 2: blue -> light-blue / greenish
        else if ((i >= 70) && (i < 100))
            m_ColorTbl[i]=setRgb(60*(i-70)/30, 125*(i-70)/30, 115*(i-70)/30 + 140);
        // level 3: light blue -> yellow
        else if ((i >= 100) && (i < 150))
            m_ColorTbl[i]=setRgb(195*(i-100)/50 + 60, 130*(i-100)/50 + 125, 255-(255*(i-100)/50));
        // level 4: yellow -> red
        else if ((i >= 150) && (i < 250))
            m_ColorTbl[i]=setRgb(255, 255-255*(i-150)/100, 0);
        // level 5: red -> white
        else if (i >= 250)
            m_ColorTbl[i]=setRgb(255, 255*(i-250)/5, 255*(i-250)/5);
        
    }

}

unsigned int SetColorFromFloat(float value)
{
    if(value>1.0) value=1.0;
    if(value<0.0) value=0.0;
    int Index=(value*255.0);   
    
    return m_ColorTbl[Index];
}

unsigned int SetColorFromInt(int value)
{
    if(value<0) value=0;
    if(value>255) value=255;
    int Index=(value);   
    
    return m_ColorTbl[Index];
}
 

static void Init()
{
 // register signal handlers
    if (signal(SIGINT, signal_handler) == SIG_ERR)
        fputs("Warning: Can not install signal handler for SIGINT\n", stderr);
    if (signal(SIGTERM, signal_handler) == SIG_ERR)
        fputs("Warning: Can not install signal handler for SIGTERM\n", stderr);
    if (signal(SIGHUP, signal_handler) == SIG_ERR)
        fputs("Warning: Can not install signal handler for SIGHUP\n", stderr);
    if (signal(SIGPIPE, signal_handler) == SIG_ERR)
        fputs("Warning: Can not install signal handler for SIGPIPE\n", stderr);

    InitColorWaterFall();
}
void print_usage()
{

	fprintf(stderr,\
"kisspectrum -%s\n\
Usage:kisspectrum -i File [-t IQType] [-s SampleRate] [-r FrameRate][-h] \n\
-i            Input IQ File I16 format (default stdin) \n\
-t            IQType {float,u8,i16} (default float) \n\
-s            SampleRate (default 1024K) \n\
-r            Display Framertae (default 25) \n\
-h            help (print this help).\n\
Example : .\\kisspectrum -i - \n\
\n", \
PROGRAM_VERSION);

} /* end function print_usage */


int main(int argc, char *argv[]) {

    int a;
	int anyargs = 1;
    enum {TYPE_FLOAT,TYPE_U8,TYPE_I16};
    unsigned int SampleRate=1024000;
    int fps=25;
    int TypeInput=TYPE_FLOAT;
    input = stdin;
    int SampleSize=sizeof(float complex);
    while (1)
	{
		a = getopt(argc, argv, "i:t:s:r:h");

		if (a == -1)
		{
			if (anyargs) break;
			else a = 'h'; //print usage and exit
		}
		anyargs = 1;

		switch (a)
		{
		case 'i': // InputFile
			input = fopen(optarg, "r");
			if (NULL == input)
			{
				fprintf(stderr, "Unable to open '%s': %s\n",
					optarg, strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
        case 't': // Input Type
			if (strcmp("float", optarg) == 0) {TypeInput = TYPE_FLOAT;SampleSize=sizeof(float complex);}
            if (strcmp("i16", optarg) == 0) {TypeInput = TYPE_I16;SampleSize=2*sizeof(int);}
             if (strcmp("u8", optarg) == 0) {TypeInput = TYPE_U8;SampleSize=2*sizeof(unsigned char);}
			break;
        case 's': // SampleRate in Symbol/s
			SampleRate = atoi(optarg);  
			break;
        case 'r': // Frame/s 
			fps = atoi(optarg);  
			break;
		case 'h': // help
			print_usage();
			exit(0);
			break;
		case -1:
			break;
		case '?':
			if (isprint(optopt))
			{
				fprintf(stderr, "kisspectrum `-%c'.\n", optopt);
			}
			else
			{
				fprintf(stderr, "kisspectrum: unknown option character `\\x%x'.\n", optopt);
			}
			print_usage();

			exit(1);
			break;
		default:
			print_usage();
			exit(1);
			break;
		}/* end switch a */
	}/* end while getopt() */

    Init();

    int FFTSize = 1024;

    float complex * iqin = (float complex*) malloc(FFTSize * sizeof(float complex));  
    float complex * fftout = (float complex*) malloc(FFTSize * sizeof(float complex));

    float complex *derot_out=(float complex*) malloc(FFTSize * sizeof(float complex));  
    unsigned char *iqin_u8=NULL; 
    if(TypeInput==TYPE_U8) iqin_u8=(unsigned char*) malloc(FFTSize * sizeof(unsigned char)*2);
  
    int *iqin_i16=NULL;
    if(TypeInput==TYPE_I16) iqin_i16=(int*) malloc(FFTSize * sizeof(int)*2);  
    
    
    fftplan fft_plan = fft_create_plan(FFTSize, iqin, fftout, LIQUID_FFT_FORWARD, 0);
    
    int width, height;
	init(&width, &height);				   // Graphics initialization
	Start(width, height);				   // Start the picture

    VGfloat *PowerFFTx=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);
	VGfloat *PowerFFTy=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);

    int OscilloWidth=(width-FFTSize);
    VGfloat *Oscillo_rex=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);
	VGfloat *Oscillo_rey=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);
    VGfloat *Oscillo_imx=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);
	VGfloat *Oscillo_imy=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);


    int ConstellationWidth=(width-FFTSize);
    //VGfloat *Constellationx=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);
	//VGfloat *Constellationy=(VGfloat*)malloc(sizeof(VGfloat)*FFTSize);

    int WaterfallHeight=height/2;
    int WaterfallWidth=FFTSize;
    // 32 bit RGBA FFT
    unsigned int *fftImage=malloc(WaterfallWidth*(WaterfallHeight)*sizeof(unsigned int));

     // 32 bit RGBA Constellation
    unsigned int *fftImageConstellation=malloc(ConstellationWidth*(height/2)*sizeof(unsigned int));


    int          ftype = LIQUID_FIRFILT_RRC; // filter type
    unsigned int k     = 4;                  // samples/symbol
    unsigned int m     = 7;                  // filter delay (symbols)
    float        beta  = 0.35f;               // filter excess bandwidth factor
    int          ms    = LIQUID_MODEM_QPSK;  // modulation scheme (can be unknown)
	
    symtrack_cccf symtrack = symtrack_cccf_create(ftype,k,m,beta,ms);
    
	Background(0, 0, 0);				   // Black background
    Fill(255, 255, 255, 1);
	
	BackgroundRGB(0,0,0,1);
    End();	
    

    Line(FFTSize/2,0,FFTSize/2,10);
			Stroke(0, 0, 255, 1);
			Line(0,height-300,256,height-300);
			StrokeWidth(2);   

    StrokeWidth(1);

	Stroke(150, 150, 200, 1);

    keep_running=1;
    int WaterfallY=0;

    VGfloat stops[] = {
		0.0, 1.0, 1.0, 1.0, 1.0,
		0.5, 0.5, 0.5, 0.5, 1.0,
		1.0, 0.0, 0.0, 0.0, 1.0
	};

    int SkipSamples=(SampleRate-(FFTSize*fps))/fps;  
    if(SkipSamples<0) SkipSamples=0;
    printf("Skip Samples = %d\n",SkipSamples);
    int SkipByte=SampleSize*SkipSamples;
    
    char *Dummy=malloc(SkipByte);

    int SpectrumPosX=0;//(width-FFTSize)/2;
    int SpectrumPosY=height/2;
    int WaterfallPosX=0;//(width-WaterfallWidth)/2;
    int WaterfallPosY=0;
    int OscilloPosX=FFTSize;
    int OscilloPosY=height/2;
    int ConstellationPosX=FFTSize;
    int ConstellationPosY=0;

    int Splash=0;
    while(keep_running)
    {  
        int Len=0; 
        fread(Dummy,1,SkipByte,input);
        switch(TypeInput)
        {
            case TYPE_FLOAT:
            {
                
                Len=fread(iqin,sizeof(float complex),FFTSize,input);      
            }
            break;
            case TYPE_U8:
            {
                
                Len=fread(iqin_u8,sizeof(unsigned char),FFTSize*2,input);
                 for(int i=0;i<Len/2;i++)
                 {
                    iqin[i]=(iqin_u8[i*2]-127.5)/127.0+_Complex_I *(iqin_u8[i*2+1]-127.5)/127.0;
                    
                 }   
            }
            break;
            case TYPE_I16:
            {
                
                Len=fread(iqin_i16,sizeof(int),FFTSize*2,input);
                for(int i=0;i<Len/2;i++)
                {
                    iqin[i]=iqin_i16[i*2]/32767.0+ _Complex_I *iqin_i16[i*2+1]/32767.0;
                  
                }
            }
            break;
        }
        if(Len<=0) break;
        fft_execute(fft_plan);
        
        if(WaterfallY>0)        
            WaterfallY=(WaterfallY-1);
        if(WaterfallY==0) 
        {
            memcpy(fftImage,fftImage+WaterfallWidth,(WaterfallWidth)*(WaterfallHeight-1)*sizeof(unsigned int));
        }
        memset(fftImageConstellation,0,ConstellationWidth*(height/2)*sizeof(unsigned int)); // Constellation is black
        WindowClear();  
        
        if(Splash<fps*5)
        {
            //Title  
		    char sTitle[100];
             Stroke(0, 128, 128, 1);
		    sprintf(sTitle,"KisSpectrum (F5OEO)");
            Fill(0,128-Splash,128-Splash, 1);
		    Text(50,height-50, sTitle, SerifTypeface, 20);
            Splash++;
        }
        Stroke(0, 128, 128, 0.8);
		
			
       
         for(int i=FFTSize/2;i<FFTSize;i++)
	    {
                    
			        PowerFFTx[i-FFTSize/2]=i-FFTSize/2+SpectrumPosX;
                    float dbAmp=20.0*log(cabsf(fftout[i])/FFTSize);
                    unsigned int idbAmp=(unsigned int)((dbAmp+160)*height/2.0/160.0);

			        PowerFFTy[i-FFTSize/2]=idbAmp+SpectrumPosY;
                   //printf("%f %u\n",dbAmp,idbAmp);
                   
                    //FillLinearGradient(PowerFFTx[i-FFTSize/2],height/2,PowerFFTx[i-FFTSize/2],PowerFFTy[i-FFTSize/2],stops,3);	
                    //Rect(PowerFFTx[i-FFTSize/2], height/2, 1, PowerFFTy[i-FFTSize/2]);
                    unsigned int idbAmpColor=(unsigned int)((dbAmp+180)*256.0/256.0);
                    fftImage[(WaterfallWidth)*(WaterfallHeight-1)+i-WaterfallWidth/2]=SetColorFromInt(idbAmpColor);	

                    Oscillo_imx[i]=OscilloPosX+i*OscilloWidth/FFTSize;
                    Oscillo_imy[i]=OscilloPosY+height/4+cimagf(iqin[i])*height/4;
                    
                    Oscillo_rey[i]=OscilloPosY+height/4+crealf(iqin[i])*height/4;
                                        
                   int PointY=(cimagf(iqin[i])+1.0)/2.0*height/2;
                    int PointX=(crealf(iqin[i])+1.0)/2.0*ConstellationWidth;
                    int Point=PointX+ConstellationWidth*PointY;
                    //if(Point<ConstellationWidth*height/2) 
                        fftImageConstellation[Point]=SetColorFromFloat((cabsf(iqin[i])+1.0)/4.0);
            
	    }
        for(int i=0;i<FFTSize/2;i++)
	    {
                    
			        PowerFFTx[i+FFTSize/2]=i+FFTSize/2+SpectrumPosX;
                    float dbAmp=20.0*log(cabsf(fftout[i])/FFTSize);
                    unsigned int idbAmp=(unsigned int)((dbAmp+160)*height/2.0/160.0);

			        PowerFFTy[i+FFTSize/2]=idbAmp+SpectrumPosY;
                    unsigned int idbAmpColor=(unsigned int)((dbAmp+180)*256.0/256.0);	
                    fftImage[(WaterfallWidth)*(WaterfallHeight-1)+i+WaterfallWidth/2]=SetColorFromInt(idbAmpColor);

                    Oscillo_imx[i]=OscilloPosX+i*OscilloWidth/FFTSize;
                    Oscillo_imy[i]=OscilloPosY+height/4+cimagf(iqin[i])*height/4;

                    Oscillo_rey[i]=OscilloPosY+height/4+crealf(iqin[i])*height/4;


                    int PointY=(cimagf(iqin[i])+1.0)/2.0*height/2;
                    int PointX=(crealf(iqin[i])+1.0)/2.0*ConstellationWidth;
                    int Point=PointX+ConstellationWidth*PointY;
                    /*if(Point<ConstellationWidth*height/2) 
                        fftImageConstellation[Point]=SetColorFromFloat((cabsf(iqin[i])+1.0)/4.0);*/
                   
	    }
        Polyline(PowerFFTx, PowerFFTy, FFTSize);

        Stroke(128, 0, 0, 1);
        Polyline(Oscillo_imx, Oscillo_rey, FFTSize);
        Stroke(0, 128, 0, 1);
        Polyline(Oscillo_imx, Oscillo_imy, FFTSize);
        makeimage(WaterfallPosX,WaterfallPosY,WaterfallWidth,WaterfallHeight,(VGubyte *)fftImage);
        makeimage(ConstellationPosX,ConstellationPosY,ConstellationWidth,height/2,(VGubyte *)fftImageConstellation);
        int num_written=0;
        symtrack_cccf_execute_block(symtrack,iqin,  FFTSize,derot_out, &num_written);
      
        for(int i=0;i<num_written;i++)
        {
             int PointY=(cimagf(derot_out[i])+1.0)/2.0*height/2;
                    int PointX=(crealf(derot_out[i])+1.0)/2.0*ConstellationWidth;
                    int Point=PointX+ConstellationWidth*PointY;
                      //printf("Derot %d\n",  Point);          

                    if((Point>0)&&(Point<ConstellationWidth*height/2)) 
                        fftImageConstellation[Point]=SetColorFromFloat(1.0);
        }
        usleep(100);
        End();						   // End the picture
        
    }
    
	End();						   // End the picture

	

free(PowerFFTx);
free(PowerFFTy);    
   fft_destroy_plan(fft_plan);
   free(iqin);
    free(fftout);
    free(fftImage);
	finish();
	exit(0);
}

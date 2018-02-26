/*
Raspberry Kiss Spectrum : KisSpectrum 
Copyright (c) 2017-2018, Evariste F5OEO
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
//
// Todo List
// Average FFT
// Title for each Window
// Use a general input (touchscreen/mouse/midi controler/omnirig)

//
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

#include "colormaps.py"

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


static void InitColorWaterFall()
{
// default waterfall color scheme
    for (int i = 0; i < 256; i++)
    {    

        m_ColorTbl[i]=setRgb(_viridis_data[i][0]*255,_viridis_data[i][1]*255,_viridis_data[i][2]*255);
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

#ifdef __ARM_NEON__
/*
void u8tofloat(
    size_t nsamples,
    unsigned char* buf,
    std::complex<float>* fo)
 {
  // Number to subtract from samples for normalization
  // See http://cgit.osmocom.org/gr-osmosdr/tree/lib/rtl/rtl_source_c.cc#n176
  const float32_t norm_ = 127.4f / 128.0f;
  float32x4_t norm = vld1q_dup_f32(&norm_);

  // Iterate over samples in blocks of 4 (each sample has I and Q)
  for (uint32_t i = 0; i < (nsamples / 4); i++) {
    uint32x4_t ui;
    ui[0] = buf[i * 8 + 0];
    ui[1] = buf[i * 8 + 2];
    ui[2] = buf[i * 8 + 4];
    ui[3] = buf[i * 8 + 6];

    uint32x4_t uq;
    uq[0] = buf[i * 8 + 1];
    uq[1] = buf[i * 8 + 3];
    uq[2] = buf[i * 8 + 5];
    uq[3] = buf[i * 8 + 7];

    // Convert to float32x4 and divide by 2^7 (128)
    float32x4_t fi = vcvtq_n_f32_u32(ui, 7);
    float32x4_t fq = vcvtq_n_f32_u32(uq, 7);

    // Subtract to normalize to [-1.0, 1.0]
    float32x4x2_t fiq;
    fiq.val[0] = vsubq_f32(fi, norm);
    fiq.val[1] = vsubq_f32(fq, norm);

    // Store in output
    vst2q_f32((float*) (&fo[i * 4]), fiq);
  }
}
*/
#endif 

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

// coordpoint marks a coordinate, preserving a previous color
void coordpoint(VGfloat x, VGfloat y, VGfloat size, VGfloat pcolor[4]) {
	Fill(128, 0, 0, 0.3);
	Circle(x, y, size);
	setfill(pcolor);
}

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
            if (strcmp("i16", optarg) == 0) {TypeInput = TYPE_I16;SampleSize=2*sizeof(short);}
             if (strcmp("u8", optarg) == 0) {TypeInput = TYPE_U8;SampleSize=2*sizeof(unsigned char);}
			break;
        case 's': // SampleRate in Symbol/s
			SampleRate = atoi(optarg);  
			break;
        case 'r': // Frame/s 
			fps = atoi(optarg);
            if(fps>25) fps=25; //Fixme should be teh framerate of screen mode  
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

    float complex * iqin = (float complex*) malloc(FFTSize *sizeof(float complex));  
    float complex * fftout = (float complex*) malloc(FFTSize * sizeof(float complex));
    
    float complex * fftavg = (float complex*) malloc(FFTSize * sizeof(float complex));

    float complex *derot_out=(float complex*) malloc(FFTSize * sizeof(float complex));  
    unsigned char *iqin_u8=NULL; 
    if(TypeInput==TYPE_U8) iqin_u8=(unsigned char*) malloc(FFTSize * SampleSize);
  
    short *iqin_i16=NULL;
    if(TypeInput==TYPE_I16) iqin_i16=(short*) malloc(FFTSize * SampleSize);  
    
    
    fftplan fft_plan = fft_create_plan(FFTSize, iqin, fftout, LIQUID_FFT_FORWARD, 0);
    
    int width, height;
	init(&width, &height);				   // Graphics initialization
    fprintf(stderr,"Display resolution = %dx%d\n",width,height);
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
     unsigned int *fftLine=malloc(WaterfallWidth*sizeof(unsigned int));
     // 32 bit RGBA Constellation
    unsigned int *fftImageConstellation=malloc(ConstellationWidth*(height/2)*sizeof(unsigned int));


    int          ftype = LIQUID_FIRFILT_RRC; // filter type
    unsigned int k     = 4;                  // samples/symbol
    unsigned int m     = 3;                  // filter delay (symbols)
    float        beta  = 0.35f;               // filter excess bandwidth factor
    int          ms    = LIQUID_MODEM_QPSK;  // modulation scheme (can be unknown)
	
    symtrack_cccf symtrack = symtrack_cccf_create(ftype,k,m,beta,ms);
    //symtrack_cccf_set_bandwidth(symtrack,0.1);

     iirfilt_crcf dc_blocker = iirfilt_crcf_create_dc_blocker(0.001);

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
    SkipSamples=(SkipSamples/FFTSize)*FFTSize;
    int TheoricFrameRate=SampleRate/FFTSize;
    int ShowFrame=TheoricFrameRate/fps;
    printf("TheoricFrameRate %d=%d/%d Showframe =%d\n",TheoricFrameRate,SampleRate,FFTSize,ShowFrame);
    if(SkipSamples<0) SkipSamples=0;
    fprintf(stderr,"Skip Samples = %d\n",SkipSamples);
    int SkipByte=SampleSize*SkipSamples;
    
    char *Dummy=malloc(SkipByte*SampleSize);

    int SpectrumPosX=0;//(width-FFTSize)/2;
    int SpectrumPosY=height/2;
    int WaterfallPosX=0;//(width-WaterfallWidth)/2;
    int WaterfallPosY=0;
    int OscilloPosX=FFTSize;
    int OscilloPosY=height/2;
    int ConstellationPosX=FFTSize;
    int ConstellationPosY=0;

    float FloorDb=180.0;
    float ScaleFFT=1.0;    

    int Splash=0;
    int NumFrame=0;
    
    fprintf(stderr,"Warning displayed only 1 IQ buffer every %d\n",ShowFrame);
    while(keep_running)
    {  
        int Len=0; 
        NumFrame++;
        //while(fread(Dummy,1,SkipByte,input)==0);
        switch(TypeInput)
        {
            case TYPE_FLOAT:
            {
                
                Len=fread(iqin,sizeof(float),FFTSize,input); 
                if(Len==0) break;     
            }
            break;
            case TYPE_U8:
            {
                
                Len=fread(iqin_u8,sizeof(char),FFTSize*2,input);
                if(Len==0) break;
                 for(int i=0;i<Len/2;i++)
                 {
                    iqin[i]=(iqin_u8[i*2]-127.5)/127.0+_Complex_I *(iqin_u8[i*2+1]-127.5)/127.0;
                    
                 }   
            }
            break;
            case TYPE_I16:
            {
                
                Len=fread(iqin_i16,sizeof(short),FFTSize*2,input);
                //printf("FFTSize=%d SampleSize=%d,Len =%d\n",FFTSize,SampleSize,Len);
                if(Len==0) break;
                for(int i=0;i<Len/2;i++)
                {
                    iqin[i]=iqin_i16[i*2]/32767.0+ _Complex_I *iqin_i16[i*2+1]/32767.0;
                    iirfilt_crcf_execute(dc_blocker, iqin[i], &iqin[i]);
                }
            }
            break;
        }
        if(Len<=0) break;
        //if((NumFrame%ShowFrame)!=0) continue;
        

        if((NumFrame%ShowFrame)==0)
        {
            fft_execute(fft_plan);
            for(int i=0;i<FFTSize;i++)
            {
               fftavg[i]+=fftout[i];
            }

            for(int i=0;i<FFTSize;i++)
              {
                    fftavg[i]+=fftavg[i];
                  //fftavg[i]+=fftavg[i]/(float)ShowFrame;
             }
        
        
            memset(fftImageConstellation,0,ConstellationWidth*(height/2)*sizeof(unsigned int)); // Constellation is black : Fixme ; need a window of I/Q constell for summing and averaging
      
        
       
		
			//symtrack_cccf_reset(symtrack);
        float dbpeak=-200;

         for(int i=FFTSize/2;i<FFTSize;i++)
	    {
                    
			        PowerFFTx[i-FFTSize/2]=i-FFTSize/2+SpectrumPosX;
                    
                    float dbAmp=20.0*log(cabsf(fftavg[i])/FFTSize);
                    if(dbAmp>dbpeak) dbpeak=dbAmp;
                    unsigned int idbAmp=(unsigned int)((dbAmp+FloorDb)*height/2.0/FloorDb*ScaleFFT);

			        PowerFFTy[i-FFTSize/2]=idbAmp+SpectrumPosY;

                   
                
                   //printf("%f %u\n",dbAmp,idbAmp);
                   
                    //FillLinearGradient(PowerFFTx[i-FFTSize/2],height/2,PowerFFTx[i-FFTSize/2],PowerFFTy[i-FFTSize/2],stops,3);	
                    //Rect(PowerFFTx[i-FFTSize/2], height/2, 1, PowerFFTy[i-FFTSize/2]);
                   unsigned int idbAmpColor=(unsigned int)((dbAmp+FloorDb)*2*ScaleFFT);//(unsigned int)((dbAmp+200));
                   // fftImage[(WaterfallWidth)*(WaterfallHeight-1)+i-WaterfallWidth/2]=SetColorFromInt(idbAmpColor);	
                    fftLine[i-WaterfallWidth/2]=SetColorFromInt(idbAmpColor);
                    //VGfloat shapecolor[4];
                    //shapecolor=SetColorFromInt(idbAmpColor);

                    // coordpoint(PowerFFTx[i-FFTSize/2],PowerFFTy[i-FFTSize/2],1,shapecolor);


                  
                    Oscillo_imx[i]=OscilloPosX+i*OscilloWidth/FFTSize;
                    Oscillo_imy[i]=OscilloPosY+height/4+cimagf(iqin[i])*height/4;
                    
                    Oscillo_rey[i]=OscilloPosY+height/4+crealf(iqin[i])*height/4;
                                        
                  /* int PointY=(cimagf(iqin[i])+1.0)/2.0*height/2;
                    int PointX=(crealf(iqin[i])+1.0)/2.0*ConstellationWidth;
                    int Point=PointX+ConstellationWidth*PointY;
                    if(Point<ConstellationWidth*height/2) 
                        fftImageConstellation[Point]=SetColorFromFloat((cabsf(iqin[i])+1.0)/4.0);*/
            
	    }
        for(int i=0;i<FFTSize/2;i++)
	    {
                    
			        PowerFFTx[i+FFTSize/2]=i+FFTSize/2+SpectrumPosX;
                    float dbAmp=20.0*log(cabsf(fftavg[i])/FFTSize);
                    if(dbAmp>dbpeak) dbpeak=dbAmp;
                    unsigned int idbAmp=(unsigned int)((dbAmp+FloorDb)*height/2.0/FloorDb*ScaleFFT);

			        PowerFFTy[i+FFTSize/2]=idbAmp+SpectrumPosY;
                     unsigned int idbAmpColor=(unsigned int)((dbAmp+FloorDb)*2*ScaleFFT);//(unsigned int)((dbAmp+200));
                   // printf("dbAmp %f ->%d\n",dbAmp,idbAmpColor);
                    //unsigned int idbAmpColor=(unsigned int)((dbAmp)+200);	
                    //fftImage[(WaterfallWidth)*(WaterfallHeight-1)+i+WaterfallWidth/2]=SetColorFromInt(idbAmpColor);
                    fftLine[i+WaterfallWidth/2]=SetColorFromInt(idbAmpColor);

                    Oscillo_imx[i]=OscilloPosX+i*OscilloWidth/FFTSize;
                    Oscillo_imy[i]=OscilloPosY+height/4+cimagf(iqin[i])*height/4;

                    Oscillo_rey[i]=OscilloPosY+height/4+crealf(iqin[i])*height/4;

                    /*
                    int PointY=(cimagf(iqin[i])+1.0)/2.0*height/2;
                    int PointX=(crealf(iqin[i])+1.0)/2.0*ConstellationWidth;
                    int Point=PointX+ConstellationWidth*PointY;
                    if(Point<ConstellationWidth*height/2) 
                        fftImageConstellation[Point]=SetColorFromFloat((cabsf(iqin[i])+1.0)/4.0);*/
                   
	    }
            //unsigned int idbAmpColor=(unsigned int)((dbpeak+FloorDb)*2*ScaleFFT);
            //unsigned int ColorAverage=SetColorFromInt(idbAmpColor);
            //Stroke(ColorAverage&0xff,(ColorAverage>>8)&0xFF,(ColorAverage>>16)&0xFF, 1);
           
            
           }
          
           
           
             // QPSK symbol tracking
            int num_written=0;
           // symtrack_cccf_reset(symtrack);    
         
         if((NumFrame%ShowFrame)==0)
         {
            
             
             ClipRect(OscilloPosX, height/2, width-FFTSize, height/2);
              WindowClear();  
              ClipEnd();
             //Scale(0.8,1);
            ClipRect(0, height/2,FFTSize, height/2);
             //Translate(0,-1);
              
             //Scale(1,1);
             Stroke(0, 128, 192, 0.8);   
             Polyline(PowerFFTx, PowerFFTy, FFTSize);
            
             float RemanenceRate=8;
             vgCopyPixels(0,height/2,0,height/2+RemanenceRate,FFTSize,height/2-RemanenceRate);
            /*
            for(int i=0;i<10;i++)
            {
                vgCopyPixels(0,height/2,0,height/2+i,FFTSize,height/2*((10-i)/10.0));
                //vgCopyPixels(0,height/2+(i*height/2)/10.0,0,height/2+(i*height/2)/10.0+(10-i*i),FFTSize,(height/2)/10.0);
                //vgCopyPixels(0,height-(height/2)*i/10.0,0,height-(height/2)*i/10.0+i,FFTSize,(height/2)/10.0-i);
            }*/
             ClipEnd();
             if(Splash<fps*5)
        {
            //Title  
		    char sTitle[100];
             Stroke(0, 128, 128, 1);
		    sprintf(sTitle,"KisSpectrum (F5OEO)");
            Fill(0,128-Splash,128-Splash, 1);
		    Text(FFTSize,height-50, sTitle, SerifTypeface, 20);
            
            Splash++;
        }
            //Scale(0,0);
            //Translate(0,0);
            //Stroke(128, 0, 0, 1);
            Polyline(Oscillo_imx, Oscillo_rey, FFTSize);
            Stroke(0, 128, 0, 1);
            Polyline(Oscillo_imx, Oscillo_imy, FFTSize);
           
           // ClipRect(WaterfallPosX, WaterfallPosY, WaterfallWidth, WaterfallHeight);
           // makeimage(WaterfallPosX,WaterfallPosY,WaterfallWidth,WaterfallHeight,(VGubyte *)fftImage);
              
                makeimage(WaterfallPosX,height/2-1,WaterfallWidth,1,(VGubyte *)fftLine); 
            vgCopyPixels(0,0,0,1,FFTSize,height/2);
             
            //ClipEnd();
            symtrack_cccf_execute_block(symtrack,iqin,  FFTSize,derot_out, &num_written);
            for(int i=0;i<num_written;i++)
                    {
                         int PointY=(cimagf(derot_out[i])+1.5)/3.0*height/2;
                                int PointX=(crealf(derot_out[i])+1.5)/3.0*ConstellationWidth;
                                int Point=PointX+ConstellationWidth*PointY;
                                  //printf("Derot %d\n",  Point);          

                                if((Point>0)&&(Point<ConstellationWidth*height/2)) 
                                    fftImageConstellation[Point]=SetColorFromFloat(1.0);
                                /*else
                                    fprintf(stderr,"%f+i%f\n",crealf(derot_out[i]),cimagf(derot_out[i]));*/
                    }    
        
            makeimage(ConstellationPosX,ConstellationPosY,ConstellationWidth,height/2,(VGubyte *)fftImageConstellation);
          
                End();
        }
       // NumFrame++;						   // End the picture
                for(int i=0;i<FFTSize;i++) fftavg[i]=0;
        
    }
    
	End();						   // End the picture

	

free(PowerFFTx);
free(PowerFFTy);    
   fft_destroy_plan(fft_plan);
   free(iqin);
    free(fftout);
    free(fftavg);    
    free(fftImage);
	finish();
	exit(0);
}

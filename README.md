# kisspectrum
Raspberry Kiss Spectrum :  KisSpectrum

Kiss is not related to hardrock (sorry) : https://en.wikipedia.org/wiki/KISS_principle

#Dependencies
- https://github.com/jgaeddert/liquid-dsp
- fftw3

#Example of usage using rtl-sdr dongle

rtl_sdr  -f 144525000 -g 10 -s 2048000 - | ./kisspectrum -t u8 -s 2048000 -r 25

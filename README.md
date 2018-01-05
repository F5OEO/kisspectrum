# kisspectrum
Raspberry Kiss Spectrum :  KisSpectrum 

#Dependencies
- https://github.com/jgaeddert/liquid-dsp
- fftw3

#Example of usage using rtl-sdr dongle

rtl_sdr  -f 144525000 -g 10 -s 2048000 - | ./kisspectrum -t u8 -s 2048000 -r 25

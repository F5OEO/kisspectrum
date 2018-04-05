# kisspectrum
Raspberry Kiss Spectrum :  KisSpectrum

Kiss is not related to hardrock (sorry) : https://en.wikipedia.org/wiki/KISS_principle

#Dependencies
- apt-get install fftw3-dev libjpeg-dev autoconf ttf-dejavu-core rtl-sdr
- https://github.com/jgaeddert/liquid-dsp

#Example of usage using rtl-sdr dongle

rtl_sdr  -f 144525000 -g 10 -s 2048000 - | ./kisspectrum -t u8 -s 2048000 -r 25

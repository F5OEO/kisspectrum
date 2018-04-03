mkfifo spectrum.iq
#sudo ../limetool/limerx -b 10000 -s 2000 -f 106000 -g 80 -t u8 > spectrum.iq &
rtl_sdr -p 30 -g 30 -f 380000000 -s 2000000 - > spectrum.iq &
#perf record  ./kisspectrum -i spectrum.iq -t u8 -s 1000000 -r 25 
time ./kisspectrum -i spectrum.iq -t u8 -s 2000000 -r 30

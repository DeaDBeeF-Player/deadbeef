CC=gcc
BASEOPT=-Wall -g -I ../simd -I ../dft -L../simd -L../dft
OPT=$(BASEOPT) -O

all : DFTExample DFTTestNaive

clean :
	rm -f *~ *.o nsfftplan.*.txt *.log *.dat a.out DFTExample DFTTestNaive DFTTestOoura DFTTestFFTW pi_fft_mod pi_fft_mod.c

../simd/libSIMD.a :
	@cd ../simd; make

../dft/libDFT.a :
	@cd ../dft; make

../ooura/fftsg.o :
	@cd ../ooura; make

DFTExample : DFTExample.c ../simd/libSIMD.a ../dft/libDFT.a
	$(CC) $(OPT) DFTExample.c -lDFT -lSIMD -lm -o DFTExample

DFTTestNaive : DFTTestNaive.c ../simd/libSIMD.a ../dft/libDFT.a
	$(CC) $(OPT) DFTTestNaive.c -lDFT -lSIMD -lm -o DFTTestNaive

DFTTestOoura : DFTTestOoura.c ../ooura/fftsg.o ../simd/libSIMD.a ../dft/libDFT.a
	$(CC) $(OPT) DFTTestOoura.c ../ooura/fftsg.o -lDFT -lSIMD -lm -o DFTTestOoura

DFTTestFFTW : DFTTestFFTW.c ../simd/libSIMD.a ../dft/libDFT.a
	$(CC) $(OPT) DFTTestFFTW.c -lDFT -lSIMD -lfftw3 -lm -o DFTTestFFTW

pi_fft_mod.c : ../ooura/pi_fft.c pi_fft.c.patch
	patch -o pi_fft_mod.c ../ooura/pi_fft.c pi_fft.c.patch

pi_fft_mod : ../simd/libSIMD.a ../dft/libDFT.a pi_fft_mod.c
	$(CC) $(OPT) pi_fft_mod.c -I ../dft -I ../simd -L../dft -L../simd -lm -lDFT -lSIMD -o pi_fft_mod

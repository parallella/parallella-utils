/*
Copyright (c) 2014, Adapteva, Inc.
Contributed by Fred Huettig <Fred@Adapteva.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  Neither the name of the copyright holders nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*

  porcutest.c

  Test of the Porcupine GPIO pins.  Requires a connection between
    two banks of GPIOs, one bank will write while the other reads.
    Read/Write are swapped for each test.

  Build:
  gcc -o porcutest porcutest.cpp para_gpio.cpp para_gpio.c -Wall

  Notes:
    Not intended for more than 31 "wires," meaning connections
    between banks of GPIOs.
*/

// Set number of GPIO pins to use for the test, from each bank of 24
// NOTE: The default devicetree has GPIO85 reserved as a USB reset
// (which it's not), so for the moment we'll skip that bit.
#define NWIRES 23

/*************************************
 *  The porcupine test is intended to be wired as follows:
 *    GPIOA
 *  signal: 21 20 17 16 13 12  9  8  5  4  1  0
 *  gpioXX: 75 74 71 70 67 66 63 62 59 58 55 54 \
 *  array:  22 20 18 16 14 12 10  8  6  4  2  0  \
 *  -------------------------------------------   \
 *  signal: 23 33 19 18 15 14 11 10  7  6  3  2    \
 *  gpioXX: 77 76 73 72 69 68 65 64 61 60 57 56  \ |
 *  array:  23 21 19 17 15 13 11  9  7  5  3  1  | |
 *                                               | |
 *    GPIOB                                      | |
 *  signal: 45 44 41 40 37 36 33 32 29 28 25 24  / |
 *  gpioXX: 99 98 95 94 91 90 87 86 83 82 79 78    /
 *  array:  46 44 42 40 38 36 34 32 30 28 26 24   /
 *  -------------------------------------------  /
 *  signal: 47 46 43 42 39 38 35 34 31 30 27 26 /
 *  gpioXX:101100 97 96 93 92 89 88 85 84 81 80 
 *  array:  47 45 43 41 39 37 35 33 31 29 27 25
 **************************************/
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "para_gpio.h"

void Usage() {

  printf("Usage:  porcutest -h  (show this help)\n");
  printf("        gpiotest [-c N] [-v] [-d]\n\n");

  printf("    options:\n");
  printf("        -c N  - Send an incrementing count across the links,\n");
  printf("                incrementing by N.  This test is normally not\n");
  printf("                included because it takes a while.\n");
  printf("                Walking-1 & 0 tests are always used.\n\n");

  printf("        -v    - Verbose mode, shows each value written & read,\n");
  printf("                otherwise only errors are printed.\n\n");

  printf("        -d    - Debug mode, waits for <return> on each pattern.\n");
  printf("                Implies -v.\n");
  printf("\n");
  printf("Note: This application needs (probably root) access to /sys/class/gpio\n");
  printf("\n");

}

// These arrays skip the pins connected to GPIO85 (the reserved one)
int nSkipArray[] = {
  0,   1,  2,  3,  4,  5,/**/  7,  8,  9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23
};

int nRevArray[] = {
  25, 24, 27, 26, 29, 28,/**/ 30, 33, 32, 35, 34,
  37, 36, 39, 38, 41, 40, 43, 42, 45, 44, 47, 46
};

int main(int argc, char *argv[]) {
  int	n, dir, c, rc, pol, nCountInc=0, verbose=0, debug=0;
  unsigned wval, rval, cumAB=0, cumBA=0, errs=0, tests=0;
  unsigned onesAB=0, onesBA=0, zerosAB=0xFFFFFFFF, zerosBA=0xFFFFFFFF;
  CParaGpio  *gpioa, *gpiob;
  char str[256];

  printf("PORCUTEST - Basic test of Porcupine GPIOs\n\n");

  while ((c = getopt (argc, argv, "hc:dv")) != -1) {
    switch (c) {

    case 'h':
      Usage();
      exit(0);

    case 'c':
      nCountInc = atoi(optarg);
      if(nCountInc < 0) {
	fprintf(stderr, "Count increment must be non-negative, exiting\n");
	exit(1);
      }
      break;

    case 'd':
      debug = 1;
      verbose = 1;
      break;

    case 'v':
      verbose = 1;
      break;

    case '?':
      if (optopt == 'w')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint (optopt))
	fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      else
	fprintf (stderr,
		 "Unknown option character `\\x%x'.\n",
		 optopt);
      exit(1);

    default:
      fprintf(stderr, "Unexpected result from getopt?? (%d:%c)\n", c, c);
      exit(1);
    }
  }

  printf("Initializing objects...\n");
  //  gpioa = new CParaGpio(0, NWIRES, true);
  gpioa = new CParaGpio(nSkipArray, NWIRES, true);
  if(!gpioa->IsOK()) {
    fprintf(stderr, "Object creation failed for GPIOA, exiting\n");
    exit(1);
  }

  gpiob = new CParaGpio(nRevArray, NWIRES, true);
  if(!gpiob->IsOK()) {
    fprintf(stderr, "Object creation failed for GPIOB, exiting\n");
    delete gpioa;
    exit(2);
  }

  printf("Success\n");

  for(dir=0; dir<2; dir++) {

    printf("Setting direction %s...\n", dir ? "B->A" : "A->B");
    CParaGpio *pgIn  = dir ? gpioa : gpiob;
    CParaGpio *pgOut = dir ? gpiob : gpioa;
    unsigned *pCum = dir ? &cumBA : &cumAB;
    unsigned *pOnes = dir ? &onesBA : &onesAB;
    unsigned *pZeros = dir ? &zerosBA : &zerosAB;

    rc = pgIn->SetDirection(para_dirin);
    if(rc != para_ok) {
      fprintf(stderr, "SetDirection(in) failed with code %d, exiting\n", rc);
      goto done;
    }

    rc = pgOut->SetDirection(para_dirout);
    if(rc != para_ok) {
      fprintf(stderr, "SetDirection(out) failed with code %d, exiting\n", rc);
      goto done;
    }

    for(pol = 0; pol < 2; pol++) {

      printf("Walking-%ds...\n", pol);
    
      for(n=0; n<=NWIRES; n++) {  // include one extra for all-0/1

	wval = 1<<n;
	if(!pol) wval = ~wval;
	wval &= (1<<NWIRES)-1;

	rc = pgOut->SetValue(wval);
	if(rc != para_ok) {
	  fprintf(stderr, "SetValue() failed with code %d, exiting\n", rc);
	  goto done;
	}

	rc = pgIn->GetValue(&rval);
	if(rc != para_ok) {
	  fprintf(stderr, "GetValue() failed with code %d, exiting\n", rc);
	  goto done;
	}

	tests++;
	*pOnes |= rval;
	*pZeros &= rval;

	if(rval != wval) {
	  printf("MISMATCH: wrote 0x%08X, read 0x%08X, xor 0x%08X\n", wval, rval, wval ^ rval);
	  errs++;
	  *pCum |= wval ^ rval;
	}
	else if(verbose) {
	  printf("OK: 0x%08X -> 0x%08X\n", wval, rval);
	}

	if(debug)
	  fgets(str, sizeof(str), stdin);

      }
    }

  } // dir

  if(nCountInc) {

    printf("Starting counter tests\n");

    for(dir=0; dir<2; dir++) {

      printf("Setting direction %s...\n", dir ? "B->A" : "A->B");
      CParaGpio *pgIn  = dir ? gpioa : gpiob;
      CParaGpio *pgOut = dir ? gpiob : gpioa;
      unsigned *pCum = dir ? &cumBA : &cumAB;
      
      rc = pgIn->SetDirection(para_dirin);
      if(rc != para_ok) {
	fprintf(stderr, "SetDirection(in) failed with code %d, exiting\n", rc);
	goto done;
      }

      rc = pgOut->SetDirection(para_dirout);
      if(rc != para_ok) {
	fprintf(stderr, "SetDirection(out) failed with code %d, exiting\n", rc);
	goto done;
      }

      for(wval = 0; wval < (1 << NWIRES); wval += nCountInc) {

	if(tests % 500 == 0) {
	  printf("%03.1f%%\r", (100. * wval) / (1 << NWIRES));
	  fflush(stdout);
	}

	rc = pgOut->SetValue(wval);
	if(rc != para_ok) {
	  fprintf(stderr, "SetValue() failed with code %d, exiting\n", rc);
	  goto done;
	}

	rc = pgIn->GetValue(&rval);
	if(rc != para_ok) {
	  fprintf(stderr, "GetValue() failed with code %d, exiting\n", rc);
	  goto done;
	}

	tests++;

	if(rval != wval) {
	  printf("MISMATCH: wrote 0x%08X, read 0x%08X, xor 0x%08X\n", wval, rval, wval ^ rval);
	  errs++;
	  *pCum |= wval ^ rval;
	}

      }

      printf("100%%   \n\n");
    }  // for(dir ...
  }  // if(nCountInc)


  printf("\nTotal of %d error(s) in %d tests.\n", errs, tests);

  if(errs) {
    printf("Masks: A->B = 0x%08X B->A = 0x%08X\n", cumAB, cumBA);
    printf("Zeros: A->B = 0x%08X B->A = 0x%08X\n", zerosAB, zerosBA);
    printf(" Ones: A->B = 0x%08X B->A = 0x%08X\n\n", onesAB, onesBA);
  }

 done:
  printf("Closing\n");
  gpioa->Close();
  gpiob->Close();
  delete gpioa;
  delete gpiob;

  return 0;
}

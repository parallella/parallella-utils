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

  Test of the Porcupine GPIO pins.

  Build:
  gcc -o porcutest porcutest.cpp para_gpio.cpp para_gpio.c -Wall
*/

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "para_gpio.h"

void Usage() {

  printf("\nUsage:  porcutest -h  (show this help)\n");
  printf("        gpiotest\n");
  printf("    run test\n");
  printf("\n");
  printf("Note: This application needs (probably root) access to /sys/class/gpio\n");
  printf("\n");

}

int nRevArray[] = {
  25, 24, 27, 26, 29, 28, 31, 30, 33, 32, 35, 34,
  37, 36, 39, 38, 41, 40, 43, 42, 45, 44, 47, 46
};

int main(int argc, char *argv[]) {
  int	n, dir, c, rc, pol, errs=0;
  unsigned int wval, rval;
  CParaGpio  *gpioa, *gpiob;

  printf("PORCUTEST - Basic test of Porcupine GPIOs\n\n");

  while ((c = getopt (argc, argv, "h")) != -1) {
    switch (c) {

    case 'h':
      Usage();
      exit(0);

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
  gpioa = new CParaGpio(0, 24, true);
  if(!gpioa->IsOK()) {
    fprintf(stderr, "Object creation failed for GPIOA, exiting\n");
    exit(1);
  }

  gpiob = new CParaGpio(nRevArray, 24, true);
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
    
      for(n=0; n<=24; n++) {  // include one extra for all-0/1

	wval = 1<<n;
	if(!pol) wval = ~wval;
	wval &= (1<<24)-1;

	rc = pgOut->SetValue(wval);
	if(rc != para_ok) {
	  fprintf(stderr, "SetValue() failed with code %d, exiting\n", rc);
	  goto done;
	}

	rc = pgIn->GetValue(&rval);
	if(rc != para_ok) {
	  fprintf(stderr, "SetValue() failed with code %d, exiting\n", rc);
	  goto done;
	}

	if(rval != wval) {
	  printf("MISMATCH: wrote 0x%08X, read 0x%08X\n", wval, rval);
	  errs++;
	}

      }

    }

  } // dir

 done:
  printf("Closing\n");
  gpioa->Close();
  gpiob->Close();
  delete gpioa;
  delete gpiob;

  return 0;
}

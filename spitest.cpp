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

  spitest.cpp

  Test of the Parallella bit-bang'in SPI library.  Requires a connection 
    to an external SPI device, for example through the Porcupine board.
    Allows any combination of read & write to one device

  Build:
  gcc -o spitest spitest.cpp para_spi.cpp para_gpio.cpp para_gpio.c -lstdc++ -Wall

  Notes:

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "para_spi.h"

void Usage() {

  printf("Usage:  spitest -h  (show this help)\n");
  printf("        spitest [-m MNO] [PP QQ RR SS]\n\n");

  printf("    options:\n");
  printf("        -m MNO  : Set SPI mode:\n");
  printf("                     M = active enable level (0/1)\n");
  printf("                     N = resting clock polarity (0/1)\n");
  printf("                     O = phase:\n");
  printf("                           0=sample on 1st clock edge\n");
  printf("                           1=sample on 2nd clock edge\n\n");

  printf("        PP : Clock signal GPIO ID (default 65)\n");
  printf("        QQ : MOSI signal GPIO ID (default 66)\n");
  printf("        RR : MISO signal GPIO ID (default 68)\n");
  printf("        SS : Select signal GPIO ID (default 64)\n\n");

  printf("   spitest will prompt for a # of bits to write and a hex value.\n");
  printf("     The number of bits must be no more than 32, the hex number\n");
  printf("     should not have any leading '0x' etc.\n\n");

  printf("   #bits hexval > 16 ABCD\n\n");

  printf("   Enter '-' to repeat the last operation, 'q' to quit.\n\n");

  printf("Note: This application needs (probably root) access to /sys/class/gpio\n");
  printf("\n");

}

int main(int argc, char *argv[]) {
  int nCLK=65, nMOSI=66, nMISO=68, nSS=64, n, c;
  int nCPOL=0, nCPHA=0, nEPOL=0, res;
  char str[256];
  unsigned nbits=0, wval=0, rval=0;
  CParaSpi  spi;

  printf("SPITEST - Basic test of Parallella SPI Module\n\n");

  while ((c = getopt(argc, argv, "hm:")) != -1) {
    switch (c) {

    case 'h':
      Usage();
      exit(0);

    case 'm':
      n = atoi(optarg);
      if(n < 0 || n > 111) {
	fprintf(stderr, "Mode setting must be 000 - 111, exiting");
	exit(1);
      }
      nEPOL = n / 100;
      nCPOL = (n % 100) / 10;
      nCPHA = n % 10;
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

  if(optind < argc) {

    if(optind != argc - 4) {
      fprintf(stderr, "If entering pin numbers, please enter all 4");
      exit(1);
    }

    nCLK = atoi(argv[argc-4]);
    nMOSI = atoi(argv[argc-3]);
    nMISO = atoi(argv[argc-2]);
    nSS   = atoi(argv[argc-1]);
  }

  printf("Initializing object...\n");

  spi.SetMode(nCPOL, nCPHA, nEPOL);

  res = spi.AssignPins(nCLK, nMOSI, nMISO, nSS);
  if(res) {
    fprintf(stderr, "spi.AssignPins returned %d", res);
    exit(1);
  }

  if(!spi.IsOK()) {
    fprintf(stderr, "SPI Object creation failed, exiting\n");
    exit(1);
  }

  printf("Success\n");

  printf("Enter 'q' to quit.\n\n");

  while(1) {

    printf("#bits hexval > ");

    if(fgets(str, 256, stdin) == NULL)
      break;

    if(str[0] == 'q' || str[0] == 'Q')
      break;

    if(str[0] != '-' && sscanf(str, "%d %x", &nbits, &wval) != 2) {

      printf(" ???\n");
      continue;

    }

    res = spi.Xfer(nbits, &wval, &rval);
    if(res) {
      fprintf(stderr, "Xfer() returned %d, exiting\n", res);
      break;
    }

    printf("Sent 0x%08X, Rcvd 0x%08X\n\n", wval, rval);

  }

  // done:
  printf("Closing\n");
  spi.Close();

  return 0;
}

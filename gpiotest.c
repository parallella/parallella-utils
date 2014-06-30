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

  gpiotest.c

  Basic (i.e. incomplete!) test of the para_gpio system.  This code was used 
    for debug during development of para_gpio.

  Build:
  gcc -o gpiotest gpiotest.c para_gpio.c -lrt -Wall
*/


#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "para_gpio.h"

void Usage() {

  printf("\nUsage:  gpiotest -h  (show this help)\n");
  printf("        gpiotest [-g G]\n");
  printf("    -g G - Use GPIO pin G (default 7)\n");
  printf("\n");
  printf("Note: This application needs (probably root) access to /sys/class/gpio\n");
  printf("\n");

}

int main(int argc, char *argv[]) {
  int	n, i, c, rc;
  int	gpio = 7;
  para_gpio  *pGpio;
  struct timespec  tsStart, tsEnd;
  double  dTimeElapsed;

  printf("GPIOTEST - Basic test of para_gpio\n\n");

  while ((c = getopt (argc, argv, "hg:")) != -1) {
    switch (c) {

    case 'h':
      Usage();
      exit(0);

    case 'g':
      gpio = atoi(optarg);
      if(gpio < 0) {
        fprintf(stderr, "GPIO # must be > 0, exiting\n");
        exit(1);
      }
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

  printf("Initializing...\n");
  if((rc = para_initgpio(&pGpio, gpio)) != para_ok) {
    fprintf(stderr, "para_initgpio() failed with code %d, exiting\n", rc);
    exit(1);
  }
  printf("Success, pausing 5 seconds\n");
  sleep(5);

  printf("Setting direction (forces output to 0!)...\n");
  if((rc = para_dirgpio(pGpio, para_dirout)) != para_ok) {
    fprintf(stderr, "para_dirpgio() failed with code %d, exiting\n", rc);
    exit(1);
  }

  printf("Pausing 5 seconds\n");
  sleep(5);

  printf("Reading multiple times\n> ");
  for(i=0; i<20; i++) {
    if((rc = para_getgpio(pGpio, &c)) != para_ok)
      printf("ERROR %d from para_getgpio()\n", rc);
    else
      printf("%d ", c);
  }

  n = 100000;
  printf("\nToggling pin %d times...\n", n);
  clock_gettime(CLOCK_REALTIME, &tsStart);
  for(i=0; i<n; i++) {
    if((rc = para_setgpio(pGpio, i & 1)) != para_ok) {
      printf("ERROR %d from para_setgpio()\n", rc);
      break;
    }
  }
  clock_gettime(CLOCK_REALTIME, &tsEnd);

  if(rc == para_ok) {
    dTimeElapsed = (double)(tsEnd.tv_sec - tsStart.tv_sec);
    dTimeElapsed += (double)(tsEnd.tv_nsec - tsStart.tv_nsec) / 1.0e9;
    printf("Took %.3lf seconds, %.0lf updates/sec\n", dTimeElapsed, n / dTimeElapsed);
  }

  printf("Write / Read test, %d times...\n", n);

  clock_gettime(CLOCK_REALTIME, &tsStart);
  for(i=0; i<n; i++) {

    if((rc = para_setgpio(pGpio, i & 1)) != para_ok) {
      printf("ERROR %d from para_setgpio()\n", rc);
      break;
    }
    if((rc = para_getgpio(pGpio, &c)) != para_ok) {
      printf("ERROR %d from para_getgpio()\n", rc);
      break;
    }
    if(c != (i&1)) {
      printf("Error, did not read same value as written!\n");
      break;
    }
  }
  clock_gettime(CLOCK_REALTIME, &tsEnd);

  if(rc == para_ok) {
    dTimeElapsed = (double)(tsEnd.tv_sec - tsStart.tv_sec);
    dTimeElapsed += (double)(tsEnd.tv_nsec - tsStart.tv_nsec) / 1.0e9;
    printf("Took %.3lf seconds, %.0lf ops/sec\n", dTimeElapsed, 2. * n / dTimeElapsed);
  }

  for(i=0; i<2; i++) {
  
    printf("Setting output to %d\n", i);
    para_dirgpio(pGpio, para_dirout);
    para_setgpio(pGpio, i);
    sleep(1);

    printf("Verifying level... ");
    para_getgpio(pGpio, &c);
    if(c == i)
      printf("OK\n");
    else
      printf("FAILED!\n");

    printf("Disabling output and monitoring input\n");
    n = 0;
    para_dirgpio(pGpio, para_dirin);
    do {
      para_getgpio(pGpio, &c);
    } while(c == i && ++n < 100000);

    if(c == i)
      printf("Gave up waiting for the input to transition\n");
    else
      printf("Input flipped after %d cycles\n", n);

  }

  printf("Closing\n");
  para_closegpio_ex(pGpio, true);  // always un-export the pin

  return 0;
}

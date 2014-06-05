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

  pmorse.c

  Toggles a GPIO pin, by default the one connected to the LED on the 
  parallella board, to transmit a message in morse code.  This might
  be useful, for example, in embedded applications where there may
  not be an easily-accessible user interface.

  See Usage() for invocation info, including that this may need to
  be run as root.

  To use the LED or other GPIO pin for something other than morse
  code, see the functions in para_gpio.h.

*/

/*   To Build:
  > make pmorse
or
  > gcc -o pmorse pmorse.c para_morse.c para_gpio.c -Wall
*/

// TODO:
//
//  - Can't really think of anything?

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "para_morse.h"

void Usage() {

  printf("\nUsage: pmorse [-w W] [-g G] [-r R] \"string to send\"\n");
  printf("    -w W - set rate to W words/min. (default 5)\n");
  printf("    -g G - Use GPIO pin G (default 7)\n");
  printf("    -r R - Repeat message R times (0 for 'forever')\n");
  printf("    \"string to send\" is a single string surrounded by quotes.\n");
  printf("                     Use <ab> for prosigns such as <SOS>.\n");
  printf("                     Escape quotes in the string with \"\\\".\n");
  printf("\n");
  printf("Note: This application needs (probably root) access to /sys/class/gpio\n");
  printf("\n");

}

int main(int argc, char *argv[]) {
  int	i, c, rc;
  int	wpm = 5, gpio = 7, repeat = 1;
  para_gpio  *pGpio;

  printf("PMORSE - Morse-code generator for Parallella board\n\n");

  while ((c = getopt (argc, argv, "hw:g:r:")) != -1) {
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

    case 'r':
      repeat = atoi(optarg);
      if(repeat < 0) {
	fprintf(stderr, "Repeat value must be >= 0, exiting\n");
	exit(1);
      }
      if(repeat == 0)
	repeat = -1;  // 4e9 ~= "forever"
      break;

    case 'w':
      i = atoi(optarg);
      if(i<1 || i>120) {
	fprintf(stderr, "Range for -w option is 1 to 120, exiting\n");
	exit(1);
      }
      wpm = i;  // dot duration in milliseconds
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

  if(optind >= argc) {
	fprintf(stderr, "ERROR: please enter a string to transmit.\n");
	exit(1);
  }

  if(optind < argc-1) {
	fprintf(stderr, "ERROR: Please enter one string, enclosed in quotes\"\n");
	exit(1);
  }

  printf("Initializing...\n");
  if((rc = para_initgpio(&pGpio, gpio)) != para_ok) {
    fprintf(stderr, "para_initgpio() failed with code %d, exiting\n", rc);
    exit(1);
  }
  if((rc = para_dirgpio(pGpio, para_dirout)) != para_ok) {
    fprintf(stderr, "para_dirpgio() failed with code %d, exiting\n", rc);
    exit(1);
  }

  printf("Sending string %s\n", argv[optind]);

  while(repeat-- != 0)
    para_morse(pGpio, argv[optind], wpm);

  // note this won't get called if user ctl-C's out, so gpio may remain exported
  printf("Closing\n");
  para_closegpio(pGpio);

  return 0;
}

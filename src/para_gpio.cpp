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

  para_gpio.cpp
  See the header file para_gpio.hpp for description & usage info.

*/

#include "para_gpio.h"
#include <stdio.h>

int  para_gpinit(int gpio);
void para_gpclose();
int  para_gpblink(int mson, int msoff);  // used by pmorse, but available
int  para_morse(char *str, int wpm);
void Usage() {

  printf("\nUsage: pmorse [-w W] [-g G] [-r R] \"string to send\"\n");
  printf("    -w W - set rate to W words/min. (default 5)\n");
  printf("    -g G - Use GPIO pin G (default 7)\n");
  printf("    -r R - Repeat message R times (0 for 'forever')\n");
  printf("    \"string to send\" is a single string surrounded by quotes.\n");
  printf("                     Use <ab> for prosigns such as <SOS>.\n");
  printf("                     Escape quotes in the string with \"\\\".\n");
  printf("\n");
  printf("Note: This application needs (root) access to /sys/class/gpio\n");
  printf("\n");

}

int main(int argc, char *argv[]) {
  int	i, c;
  int	wpm = 5, gpio = 7, repeat = 1;

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

  printf("Initilizing...\n");
  if(initgpio(gpio)) {
     fprintf(stderr, "initgpio() failed, exiting\n");
     exit(1);
  }

  printf("Sending string %s\n", argv[optind]);

  while(repeat-- != 0)
    pmorse(argv[optind], wpm);

  // note this won't get called if user ctl-C's out, but not critical
  printf("Closing\n");
  atexit(closegpio);void Usage() {

  printf("\nUsage: pmorse [-w W] [-g G] [-r R] \"string to send\"\n");
  printf("    -w W - set rate to W words/min. (default 5)\n");
  printf("    -g G - Use GPIO pin G (default 7)\n");
  printf("    -r R - Repeat message R times (0 for 'forever')\n");
  printf("    \"string to send\" is a single string surrounded by quotes.\n");
  printf("                     Use <ab> for prosigns such as <SOS>.\n");
  printf("                     Escape quotes in the string with \"\\\".\n");
  printf("\n");
  printf("Note: This application needs (root) access to /sys/class/gpio\n");
  printf("\n");

}

int main(int argc, char *argv[]) {
  int	i, c;
  int	wpm = 5, gpio = 7, repeat = 1;

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

  printf("Initilizing...\n");
  if(initgpio(gpio)) {
     fprintf(stderr, "initgpio() failed, exiting\n");
     exit(1);
  }

  printf("Sending string %s\n", argv[optind]);

  while(repeat-- != 0)
    pmorse(argv[optind], wpm);

  // note this won't get called if user ctl-C's out, but not critical
  printf("Closing\n");
  atexit(closegpio);

  return 0;
}


  return 0;
}

char *arrMorse[] = {
	"",		// 32 ' '  (not used as a character)
	"-.-.--",	// 33 '!'
	".-..-.", 	// 34 '"'
	"", 		// 35 '#' (not available)
	"...-..-", 	// 36 '$' (not official)
	"", 		// 37 '%' (not available)
	".-...", 	// 38 '&' (also "wait)
	".----.", 	// 39 '''
	"-.--.", 	// 40 '('
	"-.--.-", 	// 41 ')'
	"",	 	// 42 '*' (not available)
	".-.-.", 	// 43 '+'
	"--.--", 	// 44 ','
	"-...-", 	// 45 '-'
	".-.-.-", 	// 46 '.'
	"-..-.", 	// 47 '/'
	"-----", 	// 48 '0'
	".----", 	// 49 '1'
	"..---", 	// 50 '2'
	"...--", 	// 51 '3'
	"....-", 	// 52 '4'
	".....", 	// 53 '5'
	"-....", 	// 54 '6'
	"--...", 	// 55 '7'
	"---..", 	// 56 '8'
	"----.", 	// 57 '9'
	"---...", 	// 58 ':'
	"-.-.-.", 	// 59 ';'
	"",	 	// 60 '<' (special function)
	"-...-", 	// 61 '='
	"", 		// 62 '>' (special function)
	".-..-.", 	// 63 '?'
	".--.-.", 	// 64 '@'
	".-", 		// 65 'A'
	"-...", 	// 66 'B'
	"-.-.", 	// 67 'C'
	"-..", 		// 68 'D'
	".", 		// 69 'E'
	"..-.", 	// 70 'F'
	"--.", 		// 71 'G'
	"....", 	// 72 'H'
	"..",	 	// 73 'I'
	".---", 	// 74 'J'
	"-.-",	 	// 75 'K'
	".-..", 	// 76 'L'
	"--", 		// 77 'M'
	"-.", 		// 78 'N'
	"---",	 	// 79 'O'
	".--.", 	// 80 'P'
	"--.-", 	// 81 'Q'
	".-.",	 	// 82 'R'
	"...", 		// 83 'S'
	"-", 		// 84 'T'
	"..-", 		// 85 'U'
	"...-", 	// 86 'V'
	".--",		// 87 'W'
	"-..-", 	// 88 'X'
	"-.--", 	// 89 'Y'
	"--..", 	// 90 'Z'
	"-.--.", 	// 91 '['
	"-..-.", 	// 92 '\' (not used, made equal to '/')
	"-.--.-", 	// 93 ']' (not used, made equal to ')')
	"", 		// 94 '^' (not used)
	"..--.-", 	// 95 '_'
	".----." 	// 96 '`' (not used, made same as ''')
};

static int gpionum=-1;

#ifndef countof
  #define countof(x)   (sizeof(x)/sizeof(x[0]))
#endif

int pmorse(char *str, int wpm) {
  int	dottime = 1200/wpm;    // msec per dot
  int	c, lettergap = 3, wordgap = 7;
  char	*pcode, *pstr;

  for(pstr = str; *pstr; pstr++) {

    if(*pstr >= 'a' && *pstr <= 'z')  // convert letters to upper case
      *pstr -= 'a' - 'A';

    c = *pstr - ' ';
    if(c < 0 || c >= countof(arrMorse)) {
      fprintf(stderr, "\nIllegal character %d (%c) in string\n", *pstr, *pstr);
      continue;
    }

    if(*pstr == ' ') {
      blink(0, (wordgap-lettergap) * dottime);
      continue;
    }

    if(*pstr == '<') {
      lettergap = 1;
      continue;
    }

    if(*pstr == '>') {
      blink(0, (3-lettergap) * dottime);
      lettergap = 3;
      continue;
    }

    for(pcode = arrMorse[c]; *pcode; pcode++) {

      if(*pcode == '.')
        blink(dottime, dottime);
      else if(*pcode == '-')
        blink(3*dottime, dottime);
      else {
        fprintf(stderr, "pmorse() Internal Error: Unexpected character in array\n");
        return 1;
      }
    }

    blink(0, (lettergap-1) * dottime);
  } 

  return 0;
}

#ifdef USESYSFS
// Use the filesystem interface to control the GPIOs
// Base on: http://www.wiki.xilinx.com/GPIO+User+Space+App
//  remember to fflush the fp to get the pin to update!

#define GPIOBASE  "/sys/class/gpio/"
static FILE  *fdVal = NULL;

int blink(int mson, int msoff) {
  static char on[] = "1";
  static char off[] = "0";

  if(fdVal == NULL)
    return 1;

  if(mson) {
    if(!fwrite(on, 2, 1, fdVal))
      printf("Can't turn bit on??\n");
    fflush(fdVal);
    usleep(mson * 1000);
  }

  if(msoff) {
    if(!fwrite(off, 2, 1, fdVal))
      printf("Can't turn bit off??\n");
    fflush(fdVal);
    usleep(msoff*1000);
  }

  return 0;
}

int initgpio(int gpio) {
  FILE *fd;
  char str1[256], str2[256];

  if(gpio < 0) {
    fprintf(stderr, "Illegal gpio # in call to initgpio\n");
    return 1;
  }

  if(fdVal) {
    fprintf(stderr, "Multiple calls to initgpio()\n");
    return 2;
  }

  gpionum = gpio;

  sprintf(str1, GPIOBASE "gpio%d/direction", gpionum);
  if(access(str1, W_OK)) {  // Must export the pin if not already done

    if((fd = fopen(GPIOBASE "export", "wb")) == NULL) {
      fprintf(stderr, "Can't access the GPIO fs entry, run me as root?\n");
      return 1;
    }

    sprintf(str2, "%d\n", gpionum);
    if(!fwrite(str2, strlen(str2), 1, fd)) {
      fprintf(stderr, "Unable to export GPIO pin!\n");
      return 2;
    }
    fclose(fd);
  }

  // Set GPIO direction
  if((fd = fopen(str1, "wb")) == NULL) {
    fprintf(stderr, "Unable to set GPIO direction!\n");
    return 3;
  }

  if(!fwrite("out\n", 4, 1, fd)) {
    fprintf(stderr, "Unable to write GPIO direction\n");
    return 4;
  }
  fclose(fd);

  // Open value file, leave it open?
  sprintf(str1, GPIOBASE "gpio%d/value", gpionum);
  fdVal = fopen(str1, "wb");
  if(fdVal == NULL) {
    fprintf(stderr, "Unable to open GPIO value fd.\n");
    return 5;
  }

  return blink(0, 500);  // Make sure we start OFF
}

void closegpio() {
  FILE  *fd;
  char  str[256];

  if(fdVal) {
    fclose(fdVal);
    fdVal = NULL;
  }
  
  if(gpionum >= 0) {

    if((fd = fopen(GPIOBASE "unexport", "wb")) == NULL)
      return;

    sprintf(str, "%d\n", gpionum);
    fwrite(str, strlen(str), 1, fd);
    fclose(fd);

    gpionum = -1;
  }
}

#else  // !USESYSFS
// Legacy GPIO access, following:
// https://www.kernel.org/doc/Documentation/gpio/gpio-legacy.txt
// Does not seem to be available on Parallella.

#include <linux/gpio.h>

int blink(int mson, int msoff) {

  if(gpionum < 0)
    return 1;  // Uninitialized??

  if(mson) {
    gpio_set_value(gpionum, 1);
    usleep(mson * 1000);
  }

  if(msoff) {
    gpio_set_value(gpionum, 0);
    usleep(msoff*1000);
  }

  return 0;
}

int initgpio(int gpio) {

  if(gpio < 0) {
    fprintf(stderr, "Illegal GPIO number requested (<0)\n");
    return 1;
  }

  if(!gpio_is_valid(gpionum)) {
    fprintf(stderr, "Requested gpio # is not valid??\n");
    return 2;
  }

  gpionum = gpio;

  // Not really any init that needs to be done!
  return 0;
}

void closegpio () {

  gpionum = -1;

  // Not much to do here either!
}

#endif

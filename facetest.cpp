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

  facetest.cpp

  Basic interface to a "PiFace Command and Control" board from a Parallella.

  Build:
  gcc -o facetest facetest.cpp para_face.cpp para_spi.cpp para_gpio.cpp para_gpio.c -lstdc++ -Wall

  Usage:
  See below for optional arguments.  Once running the program will present
    a menu of options.

  Notes:

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <ifaddrs.h>

#include "para_face.h"

void Usage() {

  printf("Usage:  paratest -h  (show this help)\n");
  printf("        paratest [PP QQ RR SS]\n\n");

  printf("    options:\n");
  printf("        PP : Clock signal GPIO ID (default 65)\n");
  printf("        QQ : MOSI signal GPIO ID (default 66)\n");
  printf("        RR : MISO signal GPIO ID (default 68)\n");
  printf("        SS : Select signal GPIO ID (default 64)\n\n");

  printf("   paratest will prompt with a menu of choices.\n\n");

  printf("   Enter 'q' to quit.\n\n");

  printf("Note: This application needs (probably root) access to /sys/class/gpio\n");
  printf("\n");

}

void check(const char *str, int errno) {

  if(errno) {
    fprintf(stderr, "ERROR (%d) from %s\n", errno, str);
    exit(1);
  }
}

void menu() {

  printf("\n  Main Menu:\n");
  printf("\tL/l:\tBacklight On/Off\n");
  printf("\tD/d:\tDisplay On/Off\n");
  printf("\tC/c:\tCursor On/Off\n");
  printf("\tB/b:\tBlink Cursor On/Off\n");
  printf("\th:\tHome Cursor\n");
  printf("\ts:\tSet Cursor Position\n");
  printf("\tx:\tClear Display\n");
  printf("\tw:\tWrite String\n");
  printf("\ti:\tShow IP info\n");
  printf("\tp:\tPushbutton status\n");

  printf("\t<enter>: Repeat last command\n");
  printf("\t?:\tShow Menu\n");
  printf("\tq/Q:\tQuit\n\n");
}

int main(int argc, char *argv[]) {
  int nCLK=65, nMOSI=66, nMISO=68, nSS=64, run=1, c, m, n;
  char str[256];
  unsigned rval=0;
  CParaFace face;

  printf("FACETEST - Basic test of Parallella -> PiFace CAD Interface\n\n");

  while ((c = getopt(argc, argv, "h")) != -1) {
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

  check("face.AssignPins()", 
	face.AssignPins(nCLK, nMOSI, nMISO, nSS));

  check("face.Init()", face.Init());

  printf("Success\n");

  menu();

  while(run) {

    printf("\n> ");

    if(fgets(str, 256, stdin) == NULL)
      break;

    if(str[0] == '\n')
      str[0] = c;
    else
      c = str[0];

    switch(str[0]) {

    case 'L':
      check("face.Backlight()", face.Backlight(1));
      break;

    case 'l':
      check("face.Backlight()", face.Backlight(0));
      break;

    case 'D':
      check("face.Display()", face.Display(1));
      break;

    case 'd':
      check("face.Display()", face.Display(0));
      break;

    case 'C':
      check("face.Cursor()", face.Cursor(1));
      break;

    case 'c':
      check("face.Cursor()", face.Cursor(0));
      break;

    case 'B':
      check("face.Blink()", face.Blink(1));
      break;

    case 'b':
      check("face.Blink()", face.Blink(0));
      break;

    case 'h':
      check("face.Home()", face.Home());
      break;

    case 's':
      printf("Row (0/1) Column (0-15): ");
      if(fgets(str, 256, stdin) == NULL)
	break;
      if(sscanf(str, "%d %d", &m, &n) != 2) {
	printf("Just the two numbers and a space, please\n");
	break;
      }
      check("face.SetCursor()", face.SetCursor(n, m));
      break;

    case 'x':
      check("face.Clear()", face.Clear());
      break;

    case 'w':
      printf("        0123456789012345\n");
      printf("String: ");
      if(fgets(str, 256, stdin) == NULL)
	break;
      n = strlen(str);
      if(str[n-1] == '\n')
	str[n-1] = 0;
      check("face.Write()", face.Write(str));
      break;

    case 'i':
      struct ifaddrs *iflist, *ifa;
      if(getifaddrs(&iflist))
	break;
      str[0] = 0;
      for(ifa=iflist; ifa != NULL; ifa = ifa->ifa_next) {
	if(ifa->ifa_name[0] != 'l' && ifa->ifa_addr->sa_family == AF_INET) {
	  sprintf(str, "%d.%d.%d.%d",
	       ifa->ifa_addr->sa_data[2],
	       ifa->ifa_addr->sa_data[3],
	       ifa->ifa_addr->sa_data[4],
	       ifa->ifa_addr->sa_data[5]);
	  break;
	}
      }
      if(str[0]) {
	face.SetCursor(0,1);
	face.Write(str);
	face.Home();
      }
      freeifaddrs(iflist);
      break;

    case 'p':
      check("face.GetButtons()", face.GetButtons(&rval));
      printf("Button status: 0x%02X\n", rval);
      break;

    case 'q':
    case 'Q':
      run = 0;
      break;

    default:
      printf("Unrecognized option '%c'\n", str[0]);
    case '?':
      menu();
      break;

    }

  }

  // done:
  printf("Closing\n");

  return 0;
}

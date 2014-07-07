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
  para_morse.c - Morse-code via GPIO library for Parallella.

  See para_morse.h for details.
*/

#include "para_gpio.h"
#include <stdio.h>

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

#ifndef countof
  #define countof(x)   (sizeof(x)/sizeof(x[0]))
#endif

int para_morse(para_gpio *pGpio, char *str, int wpm) {
  int	dottime = 1200/wpm;    // msec per dot
  unsigned c, lettergap = 3, wordgap = 7;
  char	*pcode, *pstr;

  for(pstr = str; *pstr; pstr++) {

    if(*pstr >= 'a' && *pstr <= 'z')  // convert letters to upper case
      *pstr -= 'a' - 'A';

    c = *pstr - ' ';
    if(c >= countof(arrMorse)) {
      fprintf(stderr, "\nIllegal character %d (%c) in string\n", *pstr, *pstr);
      continue;
    }

    if(*pstr == ' ') {
      para_blinkgpio(pGpio, 0, (wordgap-lettergap) * dottime);
      continue;
    }

    if(*pstr == '<') {
      lettergap = 1;
      continue;
    }

    if(*pstr == '>') {
      para_blinkgpio(pGpio, 0, (3-lettergap) * dottime);
      lettergap = 3;
      continue;
    }

    for(pcode = arrMorse[c]; *pcode; pcode++) {

      if(*pcode == '.')
        para_blinkgpio(pGpio, dottime, dottime);
      else if(*pcode == '-')
        para_blinkgpio(pGpio, 3*dottime, dottime);
      else {
        fprintf(stderr, "pmorse() Internal Error: Unexpected character in array\n");
        return 1;
      }
    }

    para_blinkgpio(pGpio, 0, (lettergap-1) * dottime);
  } 

  return 0;
}


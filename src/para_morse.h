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
  para_morse.h - Morse-code via GPIO library for Parallella.

  Usage:
  1.  Open a gpio pin using the para_gpio library:

    #include "para_morse.h"
    para_gpio *gpio;

    if(para_initgpio(&gpio, 7))
      <failed>

  2.  Configure the pin as an output (or open-drain/source as needed)

    if(para_dirgpio(gpio, para_dirout))
      <failed>

  3.  Call this function providing the gpio object, the string to send, and
      the speed in words per minute

    para_morse(gpio, "Hello World", 5);
  
  Notes:

  1.  Prosigns may be sent by enclosing the characters with <brackets>, e.g.
      "<SOS>"  This suppresses the extra time between characters.

  2.  Case is ignored.

  3.  The standard puncutation characters are supported, see the array below.

*/

#include "para_gpio.h"

int para_morse(para_gpio *pGpio, char *str, int wpm);




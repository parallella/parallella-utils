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

  para_gpio.c
  See the header file para_gpio.h for description & usage info.

*/

#include "para_gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Use the filesystem interface to control the GPIOs
// Base on: http://www.wiki.xilinx.com/GPIO+User+Space+App
//  remember to fflush the fp to get the pin to update!

#define GPIOBASE  "/sys/class/gpio/"

int  para_initgpio(para_gpio **ppGpio, int nID) {

  return para_initgpio_ex(ppGpio, nID, true);
}

int  para_initgpio_ex(para_gpio **ppGpio, int nID, bool bMayExist) {
  FILE *fd;
  char str1[256], str2[256];
  int  rc = para_ok;

  if(ppGpio == NULL)
    return para_badgpio;

  *ppGpio = NULL;

  if(nID < 0) {
    fprintf(stderr, "Illegal gpio # in call to initgpio");
    return para_outofrange;
  }

  *ppGpio = (para_gpio *)malloc(sizeof(para_gpio));
  if(*ppGpio == NULL) {
    fprintf(stderr, "Unable to allocate a new para_gpio structure\n");
    return para_outofmemory;
  }

  (*ppGpio)->nID = nID;
  (*ppGpio)->eDir = para_dirunk;
  (*ppGpio)->fpVal = NULL;
  (*ppGpio)->fpDir = NULL;
  (*ppGpio)->bIsNew = false;

  sprintf(str1, GPIOBASE "gpio%d/direction", nID);
  if(!access(str1, F_OK)) {

    if(!bMayExist) {
      fprintf(stderr, "GPIO %d already allocated and bMayExist is false", nID);
      rc = para_alreadyopen;
      goto initfail;
    }

  } else { // directory does not exist

    if((fd = fopen(GPIOBASE "export", "w")) == NULL) {
      fprintf(stderr, "Can't access the GPIO fs entry, run me as root?\n");
      rc = para_noaccess;
      goto initfail;
    }

    sprintf(str2, "%d\n", nID);
    if(!fwrite(str2, strlen(str2), 1, fd)) {
      fprintf(stderr, "Unable to export GPIO pin!\n");
      rc = para_fileerr;
      goto initfail;
    }
    fclose(fd);

    (*ppGpio)->bIsNew = true;
  }

  if((fd = fopen(str1, "w")) == NULL) {
    fprintf(stderr, "Can't open the direction file %s\n", str1);
    rc = para_fileerr;
    goto initfail;
  }
  (*ppGpio)->fpDir = fd;

  sprintf(str1, GPIOBASE "gpio%d/value", nID);
  if((fd = fopen(str1, "r+")) == NULL) {  // Read & Write on same fp
    fprintf(stderr, "Can't open the value file\n");
    rc = para_fileerr;
    goto initfail;
  }
  (*ppGpio)->fpVal = fd;

  return rc;

 initfail:
  para_closegpio(*ppGpio);
  free(*ppGpio);
  *ppGpio = NULL;
  return rc;    
}

void para_closegpio(para_gpio *pGpio) {

  para_closegpio_ex(pGpio, false);
}

void para_closegpio_ex(para_gpio *pGpio, bool bForceUnexport) {
  char  str[256];
  FILE  *fp;

  if(pGpio == NULL)
    return;

  if(pGpio->fpVal) {
    fclose(pGpio->fpVal);
    pGpio->fpVal = NULL;
  }
  
  if(pGpio->fpDir) {
    fclose(pGpio->fpDir);
    pGpio->fpDir = NULL;
  }
  
  if(pGpio->nID >= 0 && (pGpio->bIsNew || bForceUnexport)) {
    if((fp = fopen(GPIOBASE "unexport", "w")) != NULL) {

      sprintf(str, "%d\n", pGpio->nID);
      fwrite(str, strlen(str), 1, fp);
      fclose(fp);
    }
  }

  pGpio->nID = -1;
}

int para_setgpio(para_gpio *pGpio, int nValue) {
  static const char str0[] = "0\n";
  static const char str1[] = "1\n";
  static const char strIn[] = "in\n";
  static const char strOut[] = "out\n";
  const char *str;
  int res;

  if(pGpio == NULL)
    return para_badgpio;

  if(pGpio->fpVal == NULL || pGpio->fpDir == NULL)
    return para_notopen;

  switch(pGpio->eDir) {

  case para_dirout:
    res = fwrite(nValue & 1 ? str1 : str0, strlen(str0), 1, pGpio->fpVal);
    fflush(pGpio->fpVal);
    break;

  case para_dirwand:
    str = nValue & 1 ? strIn : strOut; // value is preset to 0
    res = fwrite(str, strlen(str), 1, pGpio->fpDir); // need to fix for MBCS?
    fflush(pGpio->fpDir);
    break;

  case para_dirwor:
    str = nValue & 1 ? strOut : strIn; // value is preset to 1
    res = fwrite(str, strlen(str), 1, pGpio->fpDir);
    fflush(pGpio->fpDir);
    break;

  case para_dirin:
  case para_dirunk:
  default:
    return para_nodir;
  }

  return res ? para_ok : para_fileerr;
}

int para_dirgpio(para_gpio *pGpio, para_gpiodir eDir) {
  static const char str0[] = "0\n";
  static const char str1[] = "1\n";
  static const char strIn[] = "in\n";
  static const char strOut[] = "out\n";
  int  res;

  if(pGpio == NULL)
    return para_badgpio;

  if(pGpio->fpVal == NULL || pGpio->fpDir == NULL)
    return para_notopen;

  switch(eDir) {

  case para_dirin:
    res = fwrite(strIn, strlen(strIn), 1, pGpio->fpDir);
    fflush(pGpio->fpDir);
    break;

  case para_dirout:
    res = fwrite(strOut, strlen(strOut), 1, pGpio->fpDir);
    fflush(pGpio->fpDir);
    break;

  case para_dirwand:
    res = fwrite(str0, strlen(str0), 1, pGpio->fpVal);
    fflush(pGpio->fpVal);
    break;

  case para_dirwor:
    res = fwrite(str1, strlen(str1), 1, pGpio->fpVal);
    fflush(pGpio->fpVal);
    break;

  case para_dirunk:
  default:
    return para_nodir;
  }

  pGpio->eDir = eDir;

  return res ? para_ok : para_fileerr;
}

int para_getgpio(para_gpio *pGpio, int *pValue) {
  char str[256];
  
  if(pGpio == NULL)
    return para_badgpio;

  if(pGpio->fpVal == NULL)
    return para_notopen;

  if(pValue == NULL)
    return para_badarg;

  fseek(pGpio->fpVal, 0L, SEEK_SET);

  if(fgets(str, 256, pGpio->fpVal)) {

    if(str[0] == '0')
      *pValue = 0;
    else if(str[0] == '1')
      *pValue = 1;
    else
      return para_badreturn;

    return para_ok;

  } else {

    return para_fileerr;
  }
}

int para_blinkgpio(para_gpio *pGpio, int nMSOn, int nMSOff) {
  int res;

  if(nMSOn) {
    res = para_setgpio(pGpio, 1);
    if(res != para_ok)
      return res;

    usleep(nMSOn * 1000);
  }

  res = para_setgpio(pGpio, 0);
  if(res != para_ok)
    return res;

  usleep(nMSOff*1000);

  return 0;
}

// Legacy GPIO access, following:
// https://www.kernel.org/doc/Documentation/gpio/gpio-legacy.txt
// Does not seem to be available on Parallella.

// REMOVED

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
  See the header file para_gpio.h for description & usage info.

*/

#include "para_gpio.h"
#include <unistd.h>

 // Using pins in this order puts them in "sane" order
 // on the Porcupine GPIO headers.  Note these IDs must be
 // offset by EXTGPIOSTART!
static int nPorcOrder[EXTGPIONUM] = {
    0,  2,  1,  3,  4,  6,  5,  7,  8, 10,  9, 11,
   12, 14, 13, 15, 16, 18, 17, 19, 20, 22, 21, 23,
   24, 26, 25, 27, 28, 30, 29, 31, 32, 34, 33, 35,
   36, 38, 37, 39, 40, 42, 41, 43, 44, 46, 45, 47,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
 };

CParaGpio::CParaGpio() {
  
  nPins = 0;
  bIsOK = true;
 }

CParaGpio::CParaGpio(int nStartID, int nNumIDs/*=1*/, bool bPorcOrder/*=false*/) {
  int n, p;

  if(nStartID < 0 || nNumIDs > MAXPINSPEROBJECT || 
     nStartID + nNumIDs >= LASTGPIOID) {

    nPins = 0;
    bIsOK = false;
    return;  // FAIL
  }

  bIsOK = true;

  for(n = 0; n < nNumIDs; n++) {

    p = n + nStartID;

    if(bPorcOrder)
      p = nPorcOrder[p] + EXTGPIOSTART;

    if(para_initgpio(pGpio + n, p) != para_ok)  // Will be set to NULL on error
      bIsOK = false;
  }

  nPins = nNumIDs;
}

CParaGpio::CParaGpio(int *pIDArray, int nNumIDs, bool bPorcOrder/*=false*/) {
  int n, p;

  if(nNumIDs < 0 || nNumIDs > MAXPINSPEROBJECT) {
    nPins = 0;
    bIsOK = false;
    return;
  }

  bIsOK = true;

  for(n = 0; n < nNumIDs; n++) {

    p = pIDArray[n];

    if(bPorcOrder)
      p = nPorcOrder[p] + EXTGPIOSTART;

    if(para_initgpio(pGpio + n, p) != para_ok)  // Will be set to NULL on error
      bIsOK = false;
  }

  nPins = nNumIDs;
}

CParaGpio::~CParaGpio() {

  Close();
}

int CParaGpio::AddPin(int nID, bool bPorcOrder/*=false*/) {
  int  ret;

  if(nPins >= MAXPINSPEROBJECT)
    return para_outofmemory;

  if(bPorcOrder)
    nID = nPorcOrder[nID] + EXTGPIOSTART;

  ret = para_initgpio(pGpio + nPins, nID);

  if(ret != para_ok)
    bIsOK = false;
  else
    nPins++;

  return ret;
}

int CParaGpio::SetDirection(para_gpiodir eDir) {
  int n, res, ret = para_ok;

  for(n = 0; n < nPins; n++) {

    res = para_dirgpio(pGpio[n], eDir);

    if(res != para_ok)
      ret = res;  // return last error, if any
  }

  return ret;
}

int CParaGpio::GetDirection(para_gpiodir *pDir) {

  return para_noaccess;  // TODO: Implement C function for this!
}

int CParaGpio::SetValue(unsigned long long nValue) {
  int n, res, ret = para_ok;

  for(n = 0; n < nPins; n++) {

    res = para_setgpio(pGpio[n], (int)((nValue >> n) & 1));

    if(res != para_ok)
      ret = res;
  }

  return ret;
}

int CParaGpio::GetValue(unsigned long long *pValue) {
  int n, bit, res, ret = para_ok;

  *pValue = 0;

  for(n = 0; n < nPins; n++) {

    res = para_getgpio(pGpio[n], &bit);

    if(res != para_ok)
      ret = res;
    else
      *pValue |= ((unsigned long long)(bit & 1)) << n;

  }

  return ret;
}

int CParaGpio::GetValue(unsigned *pValue) {
  int n, bit, res, ret = para_ok;

  if(nPins > 32)
    return para_outofrange;

  *pValue = 0;

  for(n = 0; n < nPins; n++) {

    res = para_getgpio(pGpio[n], &bit);

    if(res != para_ok)
      ret = res;
    else
      *pValue |= (bit & 1) << n;

  }

  return ret;
}

int CParaGpio::WaitLevel(int nPin, int nValue, int nTimeout) {

  return para_noaccess;  // TODO: Implement C function for this!
}

int CParaGpio::WaitEdge(int nPin, int nValue, int nTimeout) {

  return para_noaccess;  // TODO: Implement C function for this!
}

int CParaGpio::Blink(unsigned long long nMask, int nMSOn, int nMSOff) {
  int ret, n;

  for(n=0; n < nPins; n++) {

    if(nMask & ( 1LL << n ))
      if((ret = para_setgpio(pGpio[n], 1)) != para_ok)
	return ret;
  }

  usleep(nMSOn * 1000);

  for(n=0; n < nPins; n++) {

    if(nMask & (1<<n))
      if((ret = para_setgpio(pGpio[n], 0)) != para_ok)
	return ret;
  }

  usleep(nMSOff * 1000);

  return para_ok;
}

void CParaGpio::Close() {
  int  n;

  for(n = 0; n < nPins; n++)
    para_closegpio(pGpio[n]);

  nPins = 0;
  bIsOK = true;
}

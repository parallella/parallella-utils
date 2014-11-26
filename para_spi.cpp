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

  para_spi.cpp
  See the header file para_spi.h for description & usage info.

*/

#include "para_spi.h"

#define SPINPINS   4
#define SPICLKPIN  0
#define SPIMOSIPIN 1
#define SPIMISOPIN 2
#define SPIENBPIN  3

CParaSpi::CParaSpi() {

  m_nCPOL = 0;
  m_nCPHA = 0;
  m_nEPOL = 0;
}

CParaSpi::CParaSpi(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder/*=false*/,
		   int nCPOL/*=0*/, int nCPHA/*=0*/, int nEPOL/*=0*/) {

  AssignPins(nClk, nMOSI, nMISO, nCE, bPorcuOrder);
  SetMode(nCPOL, nCPHA, nEPOL);
}

CParaSpi::~CParaSpi() {

}

int CParaSpi::AssignPins(int nClk, int nMOSI, int nMISO, int nCE,
			 bool bPorcuOrder/*=false*/) {
  int  res, ret=para_ok;

  res = AddPin(nClk, bPorcuOrder);
  if(res) ret = res;
  res = AddPin(nMOSI, bPorcuOrder);
  if(res) ret = res;
  res = AddPin(nMISO, bPorcuOrder);
  if(res) ret = res;
  res = AddPin(nCE, bPorcuOrder);
  if(res) ret = res;

  res = SetMode(m_nCPOL, m_nCPHA, m_nEPOL);
  if(res) ret = res;

  return ret;
}

int CParaSpi::SetMode(int nCPOL, int nCPHA, int nEPOL) {
  int res, ret = para_ok;

  m_nCPOL = nCPOL ? 1 : 0;
  m_nCPHA = nCPHA ? 1 : 0;
  m_nEPOL = nEPOL ? 1 : 0;

  if(!bIsOK || nPins != SPINPINS)
    return para_notopen;

  res = para_dirgpio(pGpio[SPIENBPIN], para_dirout);
  if(res) ret = res;
  res = para_setgpio(pGpio[SPIENBPIN], 1-m_nEPOL);  // Inactive state
  if(res) ret = res;

  res = para_dirgpio(pGpio[SPICLKPIN], para_dirout);
  if(res) ret = res;
  res = para_setgpio(pGpio[SPICLKPIN], m_nCPOL);  // Inactive state
  if(res) ret = res;

  res = para_dirgpio(pGpio[SPIMOSIPIN], para_dirout);
  if(res) ret = res;

  res = para_dirgpio(pGpio[SPIMISOPIN], para_dirin);
  if(res) ret = res;

  return ret;
}

int CParaSpi::Xfer(int nBits, unsigned nWVal, unsigned *pRVal/*=NULL*/) {

  return Xfer(nBits, &nWVal, pRVal);
}

int CParaSpi::Xfer(int nBits, unsigned *pWVal, unsigned *pRVal/*=NULL*/) {
  int n, clk, res, currentVal, lastVal;
  int rval;

  if(pWVal == NULL)
    para_setgpio(pGpio[SPIMOSIPIN], 0);

  if(pRVal)
    *pRVal = 0;  // clear all bits to start

  // Enable = active
  res = para_setgpio(pGpio[SPIENBPIN], m_nEPOL);
  if(res)
    return res;

  clk = m_nCPOL;


  for(n = nBits-1; n >= 0; n--) {


  if(m_nCPHA) { //if being reading on second edge start with first edge already passed
    clk = 1-clk;
    res = para_setgpio(pGpio[SPICLKPIN], clk);
    if(res) return res;
  }

    if(pWVal) { //a speedhack by checking last value
            currentVal = ((*pWVal >> n) & 1);
            if(n == nBits-1) //check if first loop
            {
      res = para_setgpio(pGpio[SPIMOSIPIN], currentVal);
      lastVal=currentVal;
            }
            else if ((currentVal)!=lastVal)
            {
    res = para_setgpio(pGpio[SPIMOSIPIN], currentVal);
    lastVal=currentVal;
            }

      if(res) return res;
    }

    clk = 1-clk;
    res = para_setgpio(pGpio[SPICLKPIN], clk);//advance the clock to tell device to read
    if(res) return res;

    if(pRVal) {
      res = para_getgpio(pGpio[SPIMISOPIN], &rval);
      if(res) return res;
      *pRVal |= rval << n;
    }

    if(!m_nCPHA) { //if reading on first clock edge, change the clock back to idle after the data has been written
      clk = 1-clk;
      res = para_setgpio(pGpio[SPICLKPIN], clk);
      if(res) return res;
    }
  }

  // Enable = inactive
  res = para_setgpio(pGpio[SPIENBPIN], 1-m_nEPOL);
  if(res)
    return res;

  return para_ok;
}


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

  para_face.cpp
  See the header file para_face.h for description & usage info.

*/

#include "para_face.h"
#include <unistd.h>

CParaFace::CParaFace() {

  m_nCLK  = 65;
  m_nMOSI = 66;
  m_nMISO = 68;
  m_nCE   = 64;
  m_bPorcuOrder = false;

  m_bBacklight = false;
  m_bCursor = false;
  m_bBlink = false;
  m_bOn = false;
  
}

CParaFace::CParaFace(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder/*=false*/) {

  m_nCLK  = nClk;
  m_nMOSI = nMOSI;
  m_nMISO = nMISO;
  m_nCE   = nCE;
  m_bPorcuOrder = bPorcuOrder;

  m_bBacklight = false;
  m_bCursor = false;
  m_bBlink = false;
  m_bOn = false;
}

CParaFace::~CParaFace() {

}

int CParaFace::AssignPins(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder/*=false*/) {

  m_nCLK  = nClk;
  m_nMOSI = nMOSI;
  m_nMISO = nMISO;
  m_nCE   = nCE;
  m_bPorcuOrder = bPorcuOrder;

  return para_ok;
}

int CParaFace::Init() {
  int res;

  // Set-up the SPI object
  res = spi.AssignPins(m_nCLK, m_nMOSI, m_nMISO, m_nCE, m_bPorcuOrder);
  if(res) return res;

  // Set-up the MCP23S17, we assume IOCON.BANK = 0 (reset value)
  // Set Port A as inputs
  res = McpSet(FACEMCP_IODIRA, 0xFF);
  if(res) return res;
  // Set Port A pull-ups on
  res = McpSet(FACEMCP_GPPUA, 0xFF);
  if(res) return res;
  // Set Port B outputs to zero
  res = McpSet(FACEMCP_GPIOB, 0x00);
  if(res) return res;
  // Set Port B as outputs
  res = McpSet(FACEMCP_IODIRB, 0x00);
  if(res) return res;

  // Set the HD44780 LCD controller to 4-bit mode
  res = LcdSendIR(0x30, 1);  // First set to 8-bit mode, just in case
  if(res) return res;
  usleep(5000);
  res = LcdSendIR(0x30, 1);  // Again, in case we started in 4-bit mode
  if(res) return res;
  usleep(200);
  res = LcdSendIR(0x30, 1);  // And yet again, just because
  if(res) return res;

  res = LcdSendIR(0x20, 1);  // NOW we can set 4-bit mode safely
  if(res) return res;
  res = LcdSendIR(0x28);  // Set lsbs for 2-line 5x8 mode
  if(res) return res;
  res = LcdSendIR(0x06);  // Auto-increment the DDRAM address
  if(res) return res;

  return para_ok;
}

int CParaFace::Backlight(bool bOn) {
  unsigned val;

  val = bOn ? FACELCD_LED : 0;
  m_bBacklight = bOn;

  return McpSet(FACEMCP_GPIOB, val);
}

int CParaFace::Display(bool bOn) {
  unsigned val;

  m_bOn = bOn;

  val = 0x08 // Display on/off control
    | (m_bOn ? 0x04 : 0)
    | (m_bCursor ? 0x02 : 0)
    | (m_bBlink ? 0x01 : 0);

  return LcdSendIR(val);
}

int CParaFace::Blink(bool bOn) {

  m_bBlink = bOn;

  return Display(m_bOn);
}

int CParaFace::Cursor(bool bOn) {

  m_bCursor = bOn;

  return Display(m_bOn);
}

int CParaFace::Clear() {
  int res;

  res = LcdSendIR(1);

  if(res == para_ok)
    usleep(5000);  // TODO: Add wait for BF here, DS doesn't say how long this takes!

  return res;
}

int CParaFace::GetCursor(int *pnCol, int *pnRow) {

  // TODO: Add this!

  return para_ok;
}

int CParaFace::Home() {
  int res;

  res = LcdSendIR(2);

  if(res == para_ok)
    usleep(2000);

  return res;
}
 
int CParaFace::SetCursor(int nCol, int nRow) {
  int val;

  val = 0x80 | (nRow << 6) | nCol;

  return LcdSendIR(val);
}

int CParaFace::Write(char *str) {
  int res, n;

  for(n=0; str[n]; n++) {

    res = LcdSendDR(str[n]);
    if(res) return res;
  }
    
  return para_ok;
}

int CParaFace::GetButtons(unsigned *pButtons) {

  return McpGet(FACEMCP_GPIOA, pButtons);
}

// Internal functions
int CParaFace::McpSet(int nReg, unsigned nData) {
  unsigned val;

  val = (FACEMCPWR << 16)
    | (nReg << 8)
    | nData;

  return spi.Xfer(24, val);
}

int CParaFace::McpGet(int nReg, unsigned *pData) {
  int res;
  unsigned val;

  val = (FACEMCPWR << 16)
    | (nReg << 8);

  res = spi.Xfer(24, val, pData);

  *pData &= 0xFF;

  return res;
}

int CParaFace::LcdSendIR(int nByte, int nCycles/*=2*/) {
  int res;
  unsigned val;

  val = m_bBacklight ? FACELCD_LED : 0;  // RS=RW=E=0
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;
  res = McpSet(FACEMCP_IODIRB, 0);  // all outputs on for write
  if(res) return res;

  val |= ((nByte >> 4) & 0x0F) | FACELCD_EN;
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  val ^= FACELCD_EN;  // de-assert E
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  if(nCycles == 2) {

    val &= 0xF0;  // clear data bits
    val |= (nByte & 0x0F) | FACELCD_EN;
    res = McpSet(FACEMCP_GPIOB, val);
    if(res) return res;

    val ^= FACELCD_EN;  // de-assert E
    res = McpSet(FACEMCP_GPIOB, val);
    if(res) return res;
  }

  usleep(50);  // This is enough for anything EXCEPT "HOME" (1.52ms)

  return para_ok;
}

int CParaFace::LcdSendDR(int nByte) {
  int res;
  unsigned val;

  val = m_bBacklight ? FACELCD_LED : 0;  // RS=RW=E=0
  val |= FACELCD_RS;

  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;
  res = McpSet(FACEMCP_IODIRB, 0);  // all outputs on for write
  if(res) return res;

  val |= ((nByte >> 4) & 0x0F) | FACELCD_EN;
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  val ^= FACELCD_EN;  // de-assert E
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  val &= 0xF0;  // clear data bits
  val |= (nByte & 0x0F) | FACELCD_EN;
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  val ^= FACELCD_EN;  // de-assert E
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  usleep(50);  // This is enough for any write to DDRAM

  return para_ok;
}

int CParaFace::LcdGetStatus(int *pBusy, int *pAddr/*=NULL*/) {
  int res;
  unsigned val, rval;

  val = m_bBacklight ? FACELCD_LED : 0;  // RS=RW=E=0

  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;
  res = McpSet(FACEMCP_IODIRB, 0x0F);  // low bits inputs for read
  if(res) return res;

  val |= FACELCD_RW | FACELCD_EN;
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  res = McpGet(FACEMCP_GPIOB, &rval);
  if(res) return res;
  if(pBusy)
    *pBusy = (rval >> 3) & 1;
  if(pAddr)
    *pAddr = (rval << 4) & 0x70;

  val ^= FACELCD_EN;  // de-assert E
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;
  val |= FACELCD_EN;  // re-assert E
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  res = McpGet(FACEMCP_GPIOB, &rval);
  if(res) return res;
  if(pAddr)
    *pAddr |= rval & 0x0F;

  val ^= FACELCD_EN;  // de-assert E
  res = McpSet(FACEMCP_GPIOB, val);

  return res;
}

int CParaFace::LcdGetDR(int *pByte) {
  int res;
  unsigned val, rval;

  val = m_bBacklight ? FACELCD_LED : 0;  // RS=RW=E=0
  val |= FACELCD_RS;

  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;
  res = McpSet(FACEMCP_IODIRB, 0x0F);  // low bits inputs for read
  if(res) return res;

  val |= FACELCD_RW | FACELCD_EN;
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  res = McpGet(FACEMCP_GPIOB, &rval);
  if(res) return res;
  if(pByte)
    *pByte = (rval << 4) & 0xF0;

  val ^= FACELCD_EN;  // de-assert E
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;
  val |= FACELCD_EN;  // re-assert E
  res = McpSet(FACEMCP_GPIOB, val);
  if(res) return res;

  res = McpGet(FACEMCP_GPIOB, &rval);
  if(res) return res;
  if(pByte)
    *pByte |= rval & 0x0F;

  val ^= FACELCD_EN;  // de-assert E
  res = McpSet(FACEMCP_GPIOB, val);

  return res;
}

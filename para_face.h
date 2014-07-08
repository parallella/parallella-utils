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

  para_face.h

  Header file for the para_face library, enabling use of GPIO pins
  of the Parallella to talk to a "PiFace Command and Control" board.
  This module makes use of the CParaSpi class from para_spi.cpp.

  Member Functions:

    Except for the constructors, all functions return 0 (success) or an
      error code from the para_gpio.h underlying code.

    CParaFace()  - Constructs an object with default pin assignments.

    CParaFace(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder) -
      Constructs an object using the specified pins.

    Init() - Initializes the SPI object and sets up the port expander on the 
      PiFace card.  Must be called before any of the following functions.

    Backlight(bool bOn) - Turns the LCD backlight on or off.

    Display(bool bOn) - Turns the entire display on or off, not including
      the backlight.  The display memory is not affected.

    Blink(bool bOn) - Turns the cursor-blink on or off.

    Cursor(bool bOn) - Turns the underline cursor on or off.

    Clear() - Clears the display.

    GetCursor(int *pnCol, int *pnRow) - Gets the current col & row
      of the cursor.

    Home() - Moves the cursor to 0,0.

    SetCursor(int nCol, int nRow) - Places the cursor at nCol/nRow.

    Write(char *str) - Writes the zero-terminated string str to the current
      cursor location.

    GetButtons(unsigned *pButtons) - Returns the state of the 8 button inputs.

*/

#ifndef PARA_FACE_H
#define PARA_FACE_H

#include "para_spi.h"
#include <stdlib.h>

// MCP addresses
#define FACEMCPWR 0x40
#define FACEMCPRD 0x41

// MCP registers, assumes bank=0
#define FACEMCP_IODIRA 0x00
#define FACEMCP_IODIRB 0x01
#define FACEMCP_GPPUA  0x0C
#define FACEMCP_GPPUB  0x0D
#define FACEMCP_GPIOA  0x12
#define FACEMCP_GPIOB  0x13

// Defines for bit positions in GPIOB, LCD interface
#define FACELCD_LED 0x80
#define FACELCD_RS  0x40
#define FACELCD_RW  0x20
#define FACELCD_EN  0x10

class CParaFace {
protected:
  CParaSpi  spi;
  int m_nCLK;
  int m_nMOSI;
  int m_nMISO;
  int m_nCE;
  bool m_bPorcuOrder;
  bool m_bBacklight;
  bool m_bCursor;
  bool m_bBlink;
  bool m_bOn;

  // Internal functions
  int McpSet(int nReg, unsigned nData);
  int McpGet(int nReg, unsigned *pData);
  int LcdSendIR(int nByte, int nCycles=2);
  int LcdSendDR(int nByte);
  int LcdGetStatus(int *pBusy, int *pAddr=NULL);
  int LcdGetDR(int *pByte);

 public:
  CParaFace();
  CParaFace(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder=false);
  ~CParaFace();
  int AssignPins(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder=false);
  int Init();
  int Backlight(bool bOn);
  int Display(bool bOn);
  int Blink(bool bOn);
  int Cursor(bool bOn);
  int Clear();
  int GetCursor(int *pnCol, int *pnRow);
  int Home();
  int SetCursor(int nCol, int nRow);
  int Write(char *str);
  int GetButtons(unsigned *pButtons);

};

#endif  // PARA_FACE_H

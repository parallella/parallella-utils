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

  para_spi.h

  Header file for the para_spi library, enabling use of GPIO pins
  of the Parallella to talk SPI to external devices.

  This class is derived from the Parallella GPIO Class, Member functions:

    Except for the constructors, all functions return 0 (success) or an
      error code.

    CParaSpi()  - Constructs an 'empty' GPIO object which may later be 
      assigned to a group of pins.

    CParaSpi(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder,
        int nCPOL=0, int nCPHA=0, int nEPOL=0) -

    AssignPins(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder) -
      Assigns pins in the event that the empty constructor was used.
      It is an error to use this function if pins have already been assigned.

    SetMode(int nCPOL=0, int nCPHA=0, int nEPOL=0) - Sets the various
      polarities / phases as described below.

    Xfer(int nBits, unsigned *pWVal, unsigned *pRVal=NULL) - Transfers
      nBits bits, transmitting from the buffer pointed-to by pWVal and
      capturing into the buffer pRVal.  Either (or both!) pWVal and/or
      pRVal may be null to either transmit 0s / discard incoming
      bits, respectively, as desired.

  Inherited functions:

    IsOK() - Checks that all pin assignments were successful, returns
      true if no errors have occurred during pin assignment, including
      if no pins have been asssigned, returns false otherwise.

    Close() - Releases all pins from the object.  This happens automatically
      when the object is destroyed.  New pins may be added with AddPin()
      after calling this functions.

  Modes:

    The following mode settings are available either when the object is 
      created or at any time later.

    nCPOL - Indicates the "rest" polarity of the SPI clock signal.  The
      clock will sit at 0/1 between transactions as set by this mode.

    nCPHA - Indicates whether data is captured on the first (nCPHA = 0)
      or second (nCPHA = 1) edge of the clock after the enable signal
      is set active.  Both the master and slave are assumed to update and
      capture data on the same edges, with capture always happening one 
      edge after update.

    nEPOL - Sets the active polarity of the enable signal high (1) or
      low (0).  Enable will be set to this level during the transfer
      and inactive at all other times.

   The following diagram shows all combinations of settings for one
     transaction.

     Enable
    EPOL=0   ---\____________________________/----
    EPOL=1   ___/----------------------------\____

      Clock
    CPOL=0   ________/---\___/---\___/---\_________
    CPOL=1   --------\___/---\___/---\___/---------

      Data           |   |   |   |   |   |
    CPHA=0   XXXXX000000001111111122222222XXXXXXXXX
    CPHA=1   XXXXXXXXX0000000011111111222222222XXXX

  Caveats:

    There has been no attempt to make this thread-safe.  It is intended
      (but as yet untested) that multiple objects may be created that share
      clock and miso/mosi lines but have different enables.  This will only
      work if only one object can have an active Read or Write operation at
      any time.  The class itself does not enforce this, the application
      must.

*/

#ifndef PARA_SPI_H
#define PARA_SPI_H

#include <stdlib.h>  // for NULL
#include "para_gpio.h"

class CParaSpi : public CParaGpio {
 protected:
  int m_nCPOL;
  int m_nCPHA;
  int m_nEPOL;

 public:
  CParaSpi();
  CParaSpi(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder,
        int nCPOL=0, int nCPHA=0, int nEPOL=0);
  ~CParaSpi();
  int AssignPins(int nClk, int nMOSI, int nMISO, int nCE, bool bPorcuOrder=false);
  int SetMode(int nCPOL, int nCPHA, int nEPOL);
  int Xfer(int nBits, unsigned nWVal, unsigned *pRVal=NULL);
  int Xfer(int nBits, unsigned *pWVal, unsigned *pRVal=NULL);

};

#endif  // PARA_SPI_H

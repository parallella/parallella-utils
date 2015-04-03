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

  evolt.c

  Report, and optionally set, the Epiphany core voltage on the
    Parallella board.

*/

/*   To Build:

  Install the libi2c-dev package, and maybe i2c-tools.

  > make
or
  > gcc -o evolt evolt.c -Wall

     To Run:

  > sudo ./evolt -h

   (will show help)

*/

// TODO:
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <linux/i2c-dev.h>

#define kSTRMAX    256
#define kDEVPATH  "/dev/i2c-%d"
#define kPMICADR  0x68
#define kNREGS    7

  // Voltage-setting constants
#define kDCDMINV  (0.825)
#define kDCDSTEP  (0.025)
#define kDCDMAX   (0x6F)
#define kLDOMINV  (0.900)
#define kLDOSTEP  (0.050)
#define kLDOMAX   (0x36)

  // Register Addresses
#define kDCD1ADR  0
#define kDCD2ADR  1
#define kLDO1ADR  2
#define kLDO2ADR  3
#define kDCDPARAM 4
#define kSYSPARAM 5
#define kSRCTL    6

  // SYSPARAM bits
#define kDCD1ENB  (1<<0)
#define kDCD2ENB  (1<<1)
#define kLDO1ENB  (1<<2)
#define kLDO2ENB  (1<<3)
#define kSYSBITS  (0x60)

#define kEPIMAX   (1.20)

int  nI2cbus = 0;

void Usage() {

  printf(
	 "\nUsage:  evolt [-e #.## | -z] [-d]\n"
	 "        evolt -h\n\n"

	 "Reports the core and DDR3-SDRAM I/O voltage settings from the PMIC.\n\n"

	 "   -e #.##   Sets the core voltage to #.##.  Valid range is 0.825V\n"
	 "             (PMIC min.) to 1.2V (Epiphany max.).  Note the datasheet\n"
	 "             minimum for operation is 0.9V.\n\n"

	 "   -z        Turn Epiphany core power OFF.\n\n"

	 "   -d        Dump raw register values.\n\n"

	 "   -h        Show this help info.\n\n");
}

int main(int argc, char *argv[]) {
  char strFName[kSTRMAX];
  unsigned char nData[kNREGS];
  int  nRet=0, nFd, nSetEpi=0, nDisEpi=0, nDump=0, c;
  size_t  nBytes;
  double  dEVolt=1.0;

  printf("\nEVolt - Epiphany Voltage Status / Control Utility\n\n");

  opterr = 0;
     
  while ((c = getopt (argc, argv, "he:zd")) != -1) {
    switch (c) {

    case 'h':
      Usage();
      return 0;

    case 'e':
      dEVolt = atof(optarg);
      nSetEpi = 1;
      break;

    case 'z':
      nDisEpi = 1;
      break;

    case 'd':
      nDump = 1;
      break;

    case '?':
      if (optopt == 'e')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint (optopt))
	fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      else
	fprintf (stderr,
		 "Unknown option character `\\x%x'.\n",
		 optopt);
      return 1;

    default:
      abort ();
    }
  }

  if(nSetEpi && nDisEpi) {
    printf("WARNING: Can't set & disable at the same time, ignoring -e.\n\n");
    nSetEpi = 0;
  }
  
  if(nSetEpi && (dEVolt < kDCDMINV || dEVolt > kEPIMAX)) {
    printf("ERROR: Voltage out of range (%.3f - %.3f).  Reading only.\n\n",
	   kDCDMINV, kEPIMAX);
    nSetEpi = 0;
  }

  sprintf(strFName, kDEVPATH, nI2cbus);
  nFd = open(strFName, O_RDWR);

  if(nFd < 0) {
    printf("ERROR: Unable to open I2C bus! (%m)\n       Run as root?\n");
    return 1;
  }

  if(ioctl(nFd, I2C_SLAVE, kPMICADR) < 0) {
    printf("ERROR: Unable to set I2C slave address! (%m)\n");
    nRet = 2;
    goto CloseExit;
  }

  for(c=0; c<kNREGS; c++) {  // Doesn't work to read all N at once??

    nBytes = write(nFd, &c, 1);  // Write 1-byte starting register address
    if(nBytes != 1) {
      printf("ERROR: could not write register # to device\n");
      goto CloseExit;
    }
    nBytes = read(nFd, nData+c, 1);  //, kNREGS);
    if(nBytes != 1) { //kNREGS) {
      printf("ERROR: Could not read all registers\n");
      goto CloseExit;
    }
  }

  if(nDump) {
    printf("    Raw register values:\n");
    for(c=0; c<kNREGS; c++)
      printf("Reg %d: 0x%X\n", c, nData[c]);
    printf("\n");
  }

  printf("    Current settings:\n");
  printf("Epiphany Core: %.3f %s\n",
	 nData[kDCD1ADR] * kDCDSTEP + kDCDMINV,
	 (nData[kSYSPARAM] & kDCD1ENB) ? "" : "(Disabled)");
  printf("DDR3 SDRAM IO: %.3f %s\n",
	 nData[kDCD2ADR] * kDCDSTEP + kDCDMINV,
	 (nData[kSYSPARAM] & kDCD2ENB) ? "" : "(Disabled)");
  printf("VDD_ADJ:       %.3f %s\n",
	 nData[kLDO1ADR] * kLDOSTEP + kLDOMINV,
	 (nData[kSYSPARAM] & kLDO1ENB) ? "" : "(Disabled)");
  printf("VDD_GPIO:      %.3f %s\n\n",
	 nData[kLDO2ADR] * kLDOSTEP + kLDOMINV,
	 (nData[kSYSPARAM] & kLDO2ENB) ? "" : "(Disabled)");

  if(nSetEpi) {

    nData[0] = kDCD1ADR;
    if(dEVolt < kDCDMINV) dEVolt = kDCDMINV;
    nData[1] = (int)((dEVolt - kDCDMINV + kDCDSTEP/2) / kDCDSTEP);
    if(nData[1] > kDCDMAX)  nData[1] = kDCDMAX;
    dEVolt = nData[1] * kDCDSTEP + kDCDMINV;

    if(write(nFd, nData, 2) != 2) {
      printf("ERROR: Could not write new voltage.\n");
      goto CloseExit;
    }

    nData[0] = kSYSPARAM;
    nData[1] = nData[kSYSPARAM] | kDCD1ENB;

    if(write(nFd, nData, 2) != 2) {
      printf("ERROR: Could not write enable bit.\n");
      goto CloseExit;
    }

    printf("Epiphany core voltage set to %.3f.\n\n", dEVolt);
  }

  if(nDisEpi) {

    nData[0] = kSYSPARAM;
    nData[1] = nData[kSYSPARAM] & (~kDCD1ENB);

    if(write(nFd, nData, 2) != 2) {
      printf("ERROR: Could not clear enable bit.\n");
      goto CloseExit;
    }

    printf("Epiphany core voltage disabled!\n\n");
  }

 CloseExit:
  close(nFd);
  return nRet;
}

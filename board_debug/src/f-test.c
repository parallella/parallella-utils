/*
f-test.c

Copyright (C) 2014 Adapteva, Inc.
Contributed by Fred Huettig <support@adapteva.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program, see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>

#include "e_regs.h"
#include "e_dma.h"
#include "elink2.h"

#define ZTEMP
int GetTemp(float *);

#ifndef countof
  #define countof(x)  (sizeof(x)/sizeof(x[0]))
#endif

// for gdb

void peek(uint32_t *p, int len)
{
    while (len--) {
        printf("0x%x  ", *(p++));
    }
    printf("\n");
}

#if defined(BROKEN_64B_READS) || defined(BROKEN_64B_WRITES)
#define memcpy aligned_memcpy
static inline void *aligned_word_memcpy(uint32_t *__restrict__ dst,
                const uint32_t *__restrict__ src, size_t size)
{
        void *ret = dst;

        assert ((size % 4) == 0);
        assert ((size == 0) || ((uintptr_t) src) % 4 == 0);
        assert ((size == 0) || ((uintptr_t) dst) % 4 == 0);

        size >>= 2;
        while (size--)
                *(dst++) = *(src++);

        return ret;
}

static inline void *aligned_memcpy(void *__restrict__ dst,
                const void *__restrict__ src, size_t size)
{
        size_t n, aligned_n;
        uint8_t *d;
        const uint8_t *s;

        n = size;
        d = (uint8_t *) dst;
        s = (const uint8_t *) src;

        if (!(((uintptr_t) d ^ (uintptr_t) s) & 3)) {
                /* dst and src are evenly WORD (un-)aligned */

                /* Align by WORD */
                if (n && (((uintptr_t) d) & 1)) {
                        *d++ = *s++; n--;
                }
                if (((uintptr_t) d) & 2) {
                        if (n > 1) {
                                *((uint16_t *) d) = *((const uint16_t *) s);
                                d+=2; s+=2; n-=2;
                        } else if (n==1) {
                                *d++ = *s++; n--;
                        }
                }

                aligned_n = n & (~3);
                aligned_word_memcpy((uint32_t *) d, (uint32_t *) s, aligned_n);
                d += aligned_n; s += aligned_n; n -= aligned_n;

                /* Copy remainder in largest possible chunks */
                switch (n) {
                case 2:
                        *((uint16_t *) d) = *((const uint16_t *) s);
                        d+=2; s+=2; n-=2;
                        break;
                case 3:
                        *((uint16_t *) d) = *((const uint16_t *) s);
                        d+=2; s+=2; n-=2;
                case 1:
                        *d++ = *s++; n--;
                }
        } else if (!(((uintptr_t) d ^ (uintptr_t) s) & 1)) {
                /* dst and src are evenly half-WORD (un-)aligned */

                /* Align by half-WORD */
                if (n && ((uintptr_t) d) & 1) {
                        *d++ = *s++; n--;
                }

                while (n > 1) {
                        *((uint16_t *) d) = *((const uint16_t *) s);
                        d+=2; s+=2; n-=2;
                }

                /* Copy remaining byte */
                if (n) {
                        *d++ = *s++; n--;
                }
        } else {
                /* Resort to single byte copying */
                while (n) {
                        *d++ = *s++; n--;
                }
        }

        assert(n == 0);
        assert((uintptr_t) dst + size == (uintptr_t) d);
        assert((uintptr_t) src + size == (uintptr_t) s);

        return dst;
}
#endif


void usage();
void GoUser();
void GoAuto(char *cmdstr, int loop);

void ShowPlatformInfo();
void ParseFpgaVersion(int version);
void Registers();
void DumpFpgaRegs();
void Reset();
void ToggleLed();
void MemAccess();
void PeekPoke();
void DmaTest();
void MemTest();
void StreamTest();
void MemAccess64();
void SetCautious();
void OlaTest();
void OlaTest2();
void TimerTest();
void ScriptTest();
void TXSpeed();
void ClockGate();

int Dma1D(unsigned int srcaddr, unsigned int dstaddr,
	  unsigned int count, unsigned int size);

#define STRINGMAX  256
#define EBASE      0x80800000
#define ASHAREBASE 0x3E000000
#define ESHAREBASE 0x8E000000

#define COLS 4
#define ROWS 4

#define COREADDR(a)  ((((a) & 0xC) << 24) | (((a) & 3) << 20))

int  g_cautious = 1;
int  g_auto = 0;
int  g_error = 0;
char g_scriptname[STRINGMAX] = "cd ../bytefail; ./test.sh";

char defAuto[] = "RCTldMs6oOx";
char defExec[] ="RcCT";

struct menu_st {
  const char  id;
  const char  *desc;
  void       (*func)();
};

struct menu_st topmenu[] = {
  {'r', "FPGA System Registers", &Registers},
  {'p', "Platform Info", &ShowPlatformInfo},
  {'R', "Reset", &Reset},
  {'l', "LED Toggle", &ToggleLed},
  {'m', "Memory write/read", &MemAccess},
  {'P', "Peek/Poke", &PeekPoke},
  {'d', "DMA Test", &DmaTest},
  {'M', "Memory test", &MemTest},
  {'s', "Streaming Test", &StreamTest},
  {'6', "64b Memory write/read", &MemAccess64},
  {'c', "Toggle Cautious mode", &SetCautious},
  {'o', "Ola Test", &OlaTest},
  {'O', "Ola Test #2", &OlaTest2},
  {'t', "Timer Test", &TimerTest},
  {'x', "eXternal Script Test", &ScriptTest},
  {'T', "TX Speed", &TXSpeed},
  {'C', "Toggle Clock Gating", &ClockGate},
  {'Q', "Quit", NULL},
  {0,   NULL, NULL}
};

int main(int argc, char *argv[]) {
  int  c;
  char *strAuto = NULL, *strExec = NULL;

  printf("\n\tf-test Parallella Info/Debug\n\n");

#ifdef BROKEN_64B_READS
  printf("ATTENTION: Compiled with BROKEN_64B_READS\n");
#endif
#ifdef BROKEN_64B_WRITES
  printf("ATTENTION: Compiled with BROKEN_64B_WRITES\n");
#endif
#ifdef SLOW_EPIPHANY_TX
  printf("ATTENTION: Compiled with SLOW_EPIPHANY_TX\n");
#endif

  opterr = 0;

  while((c = getopt(argc, argv, "ha::x::s:")) != -1) {
    switch(c) {

    case 'h':
      usage();
      return 0;

    case 'a':
      if(optarg)
        strAuto = optarg;
      else
        strAuto = defAuto;  // Default
      break;

    case 'x':
      if(optarg)
        strExec = optarg;
      else
        strExec = defExec;  // Default
      break;

     case 's':
       strncpy(g_scriptname, optarg, STRINGMAX);
       break;

    case '?':
      if(isprint(optopt))
	fprintf(stderr, "Unknown option '-%c'.\n", optopt);
      else
	fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
      return 1;

    default:
      abort();
    }
  }

  if(strExec)
    GoAuto(strExec, 0);

  if(strAuto)
    GoAuto(strAuto, 1);

  if(!strExec && !strAuto)
    GoUser();

  return 0;
}

void GoUser() {
  char command[STRINGMAX];
  int  n;

  while(1) {

    printf("\n");
    for(n=0; topmenu[n].id; n++)
      printf("  %c> %s\n", topmenu[n].id, topmenu[n].desc);

    printf("\nSelect: ");
    fgets(command, STRINGMAX, stdin);

    for(n=0; topmenu[n].id; n++)
      if(command[0] == topmenu[n].id)
	break;

    if(!topmenu[n].id) {
      printf("Please enter a choice from the menu!\n");
      continue;
    }

    if(!topmenu[n].func)
      break;  // Quit

    (*topmenu[n].func)();

  };
}

void GoAuto(char *strcmd, int loop) {
  int c, n, loopcount=0;

  g_auto = 1;

  do {

    if(loop)
      printf("\n\tLoop %d\n\n", ++loopcount);

    for(c=0; strcmd[c]; c++) {

      for(n=0; topmenu[n].id; n++) {

        if(topmenu[n].id == strcmd[c] && topmenu[n].func)
          (*topmenu[n].func)();

        if(loop && g_error)
          goto goterror;

      }
    }
  } while(loop);

  return;

goterror:
  printf("ERROR FOUND! HALTING TEST AFTER LOOP %d\n\n", loopcount);

  while(1) {
    ToggleLed();
    usleep(100000);      // fast flashing indicates error
  }
}

void Registers() {
  char command[STRINGMAX];
  unsigned  cmdnum, target;
  int  ret;

  while(1) {

    DumpFpgaRegs();

    printf("\nSelect register to write (q to quit) > ");
    fgets(command, STRINGMAX, stdin);
    cmdnum = atoi(command);
    target = 0;

    if((command[0] & 0x5F) == 'Q')
      break;

    switch(cmdnum) {
    case 0:
      if(command[0] == '0')  // Be sure it's legit
	target = E_SYS_RESET;
      break;
    case 1: target = E_SYS_CFGTX; break;
    case 2: target = E_SYS_CFGRX; break;
    case 3: target = E_SYS_CFGCLK; break;
    case 4: target = E_SYS_CHIPID; break;
    case 5: target = E_SYS_VERSION; break;
    case 6: target = E_SYS_RXGPIO; break;
    case 7: target = E_SYS_TXGPIO; break;
    }

    if(!target) {
      printf("ERROR: Please enter a valid selection\n");
      continue;
    }

    printf("Value (hex) > 0x");
    fgets(command, STRINGMAX, stdin);
    cmdnum = strtol(command, NULL, 16);

    ret = f_write(target, cmdnum);
    if(ret)
      printf("ERROR: f_write() returned %d\n", ret);

  };
}

void usage() {

  printf("Usage:\n> f-test -h\n"
	 "\tPrint this help message and exit.\n"
	 "> f-test -x[%s]\n"
	 "\t-x = Execute one or more listed commands and return\n"
	 "> f-test -a[%s]\n"
	 "\t-a = Run listed tests automatically, stop on failure\n"
	 "   No space allowed between option and argument,\n"
	 "   -a or -x by itself will run the default strings shown here.\n"
	 "> f-test\n"
	 "\tNo args = run interactvely\n"
         "Note that -x and -a may be combined to do init then loop.\n\n",
	 defExec, defAuto);
}

void ShowPlatformInfo() {
  int ret;
  e_syscfg_version_u  version;

  printf("   Platform info:\n\n");

  printf("\n    Attempting to read version from FPGA...\n");

  printf("Uh-oh, is it dead Jim??\r");
  ret = f_read(E_SYS_VERSION, &(version.reg));
  if(ret) {
    printf("f_read() returned %d.\n", ret);
    return;
  }

  printf("                                           \r");
  ParseFpgaVersion(version.reg);
}

static char *gen_strings[] = {
  "INVALID!",
  "Parallella-I",
  "Out of Range!"};

#define GEN_MAX (countof(gen_strings)-1)

static char *plat_strings_P1[] = {
  "INVALID!",
  "E16, 7Z020, GPIO connectors",
  "E16, 7Z020, no GPIO",
  "E16, 7Z010, GPIO",
  "E16, 7Z010, no GPIO",
  "E64, 7Z020, GPIO",
  "Out of Range!"};

#define PLAT_MAX_P1 (countof(plat_strings_P1)-1)

// Type 'A' applies to the first 5 platforms (update as needed)
#define PLAT_GROUP_P1A  5
// Next types should increment from group A, e.g.
#define PLAT_GROUP_P1B  (PLAT_GROUP_P1A + 4)

static char *type_strings_P1A[] = {
  "INVALID!",
  "HDMI enabled, GPIO unused",
  "Headless, GPIO unused",
  "Headless, 24/48 singled-ended GPIOs from EMIO",
  "HDMI enabled, 24/48 singled-ended GPIOs from EMIO",
  "Out of Range!"};

#define TYPE_MAX_P1A (countof(type_strings_P1A)-1)

void ParseFpgaVersion(int version) {
  int a, b, c, d;
  char  *str;

  a = (version >> 24) & 0xFF;
  b = (version >> 16) & 0xFF;
  c = (version >> 8)  & 0xFF;
  d =  version        & 0xFF;

  if(a > 16 && a < 21) {
    printf("Old-style datecode: %X/%02X/%X rev. %d (no other info)\n",
           b, c, a, d);
  } else {

    if((a & 0x80) != 0 && a != 0xFF) {
      printf("DEBUG/EXPERIMENTAL Version Detected!\n");
      a &= 0x7F;
    }

    str = gen_strings[(a < GEN_MAX) ? a : GEN_MAX];
    printf("Generation %d: %s\n", a, str);

    if(a == 1) { // Parallella-1  (todo: make an enum for these)

      str = plat_strings_P1[(b < PLAT_MAX_P1) ? b : PLAT_MAX_P1];
      printf("Platform   %d: %s\n", b, str);

      if(b <= PLAT_GROUP_P1A) {

        str = type_strings_P1A[(c < TYPE_MAX_P1A) ? c : TYPE_MAX_P1A];
        printf("Type       %d: %s\n", c, str);
        printf("Revision   %d\n", d);

      } else {

        printf("Sorry, can't interpret type for unknown platform\n");
        printf("Type %d, revision %d\n", c, d);

      }
    } else {

      printf("Sorry, can't interpret unknown generation!\n");
      printf("Platform %d, type %d, revision %d\n\n", b, c, d);
    }
  }

  printf("\n");
}

void DumpFpgaRegs() {
  int  ret = 0, n=0;
  unsigned int       resetn;
  e_sys_txcfg_t      txcfg;
  e_sys_rxcfg_t      rxcfg;
  e_sys_clkcfg_t     clkcfg;
  e_sys_chipid_t     coreid;
  e_sys_version_t    version;
  e_syscfg_gpio_u    gpioin, gpioout;

  do {
#if 0 /* TODO: Enable */
    ret = f_read(E_SYS_RESET, &resetn);
    if(ret) break;
    printf("%d> E_SYS_RESET:\t\t0x%08X\n", n++, resetn);
#endif

#if 1
    ret = f_read(E_SYS_CFGTX, &(txcfg.reg));
    if(ret) break;
    printf("%d> E_SYS_CFGTX:\t\t0x%08X\n", n++, txcfg.reg);
#endif

#if 1
    ret = f_read(E_SYS_CFGRX, &(rxcfg.reg));
    if(ret) break;
    printf("%d> E_SYS_CFGRX:\t\t0x%08X\n", n++, rxcfg.reg);
#endif

#if 0
    ret = f_read(E_SYS_CFGCLK, &(clkcfg.reg));
    if(ret) break;
    printf("%d> E_SYS_CFGCLK:\t0x%08X\n", n++, clkcfg.reg);
#endif

#if 0
    ret = f_read(E_SYS_CHIPID, &(coreid.reg));
    if(ret) break;
    printf("%d> E_SYS_CHIPID:\t0x%08X\n", n++, coreid.reg);
#endif

#if 0
    ret = f_read(E_SYS_VERSION, &(version.reg));
    if(ret) break;
    printf("%d> E_SYS_VERSION:\t0x%08X\n", n++, version.reg);
#endif

#if 1
    ret = f_read(E_SYS_RXGPIO, &(gpioin.reg));
    if(ret) break;
    printf("%d> E_SYS_RXGPIO:\t0x%08X\n", n++, gpioin.reg);
#endif

#if 0
    ret = f_read(E_SYS_TXGPIO, &(gpioout.reg));
    if(ret) break;
    printf("%d> E_SYS_TXGPIO:\t0x%08X\n", n++, gpioout.reg);
#endif

  } while(0);

  if(ret)
    printf("ERROR: f_read() returned %d.\n", ret);

}

void Reset() {
  e_sys_reset_t  resetcfg;
  e_sys_txcfg_t  txcfg;
  e_sys_rxcfg_t  rxcfg;
  //e_sys_clkcfg_t clkcfg;
  int   ret = 0;

  do {

    resetcfg.reset = 1;
    ret = f_write(E_SYS_RESET, resetcfg.reg);
    if(ret) break;
//    usleep(10);

    /* Do we need this ? */
    resetcfg.reset = 0;
    ret = f_write(E_SYS_RESET, resetcfg.reg);
    if(ret) break;
//    usleep(10);

    uint32_t chipid = 0x808 /* >> 2 */;
    ret = f_write(E_SYS_CHIPID, chipid /* << 2 */);
    if(ret) break;
//    usleep(10);

    txcfg.enable = 1;
    txcfg.mmu_enable = 0;
    ret = f_write(E_SYS_CFGTX, txcfg.reg);
    if(ret) break;

    rxcfg.enable = 1;
    rxcfg.mmu_enable = 0;
    rxcfg.mmu_cfg = 1; // "static" remap_addr
    rxcfg.remap_mask = 0xfe0; // should be 0xfe0 ???
    rxcfg.remap_base = 0x3e0;
    ret = f_write(E_SYS_CFGRX, rxcfg.reg);
    if(ret) break;

#if 0 // ?
    rx_dmacfg.enable = 1;
    if (sizeof(int) != ee_write_esys(E_SYS_RXDMACFG, rx_dmacfg.reg))
      goto err;
    usleep(1000);
#endif

    txcfg.ctrlmode = 0x5; /* Force east */
    txcfg.ctrlmode_select = 0x1;

    ret = f_write(E_SYS_CFGTX, txcfg.reg);
    if(ret) break;

#ifdef SLOW_EPIPHANY_TX
    uint32_t divider = 1; /* Divide by 4, see data sheet */
#else
    uint32_t divider = 0; /* Divide by 2, see data sheet */
#endif
    ret = f_write(EBASE + COREADDR((2<<2)+3) + 0xF0300, divider);
    if(ret) break;

    txcfg.ctrlmode_select = 0x0;
    txcfg.ctrlmode = 0x0;
    ret = f_write(E_SYS_CFGTX, txcfg.reg);
    if(ret) break;

  } while (0);

  if(ret)
    printf("ERROR: Got return value of %d\n", ret);

//  usleep(10);

  printf("DONE\n");
}

void ToggleLed() {
  int            ret;
  e_sys_txcfg_t  txcfg;
  static unsigned int led_state = 0;

  do {

    if(!g_auto)
      printf("Setting LED state to %s\n", led_state?"OFF":"ON");

    ret = f_read(E_SYS_CFGTX, &txcfg.reg);
    if(ret) break;

    //Set "direct north" routing mode to access north IO registers
    txcfg.ctrlmode = 1;
    ret = f_write(E_SYS_CFGTX, txcfg.reg);
    if(ret) break;

    //Write to north IO registers
    ret = f_write(0x80AF030c, 0x03FFFFFF);
    if(ret) break;

    ret = f_write(0x80AF0318, led_state);
    if(ret) break;
    led_state ^= 1;

    //Set config register back to normal
    txcfg.ctrlmode = 0;
    ret = f_write(E_SYS_CFGTX, txcfg.reg);
    if(ret) break;

  } while(0);

  if(ret) {
    printf("ERROR: Got return value of %d\n", ret);
    g_error++;
  }
}

void MemAccess() {
  unsigned int  data, rdata;
  int ret;

  do {
    printf("Reading from core(0,0), address 0:\n");
    usleep(100000);

    ret = f_read(0x80800000, &data);
    if(ret) break;

    printf("  0x%08X\nSending inverse, reading:\n", data);
    usleep(100000);

    ret = f_write(0x80800000, ~data);
    if(ret) break;

    ret = f_read(0x80800000, &rdata);
    if(ret) break;

    printf("  Got 0x%08X (0x%08X)\n", rdata, data|rdata);
    if((data|rdata) != 0xFFFFFFFF)
      g_error++;

  } while(0);

  if(ret) {
    printf("ERROR: Got a return value of %d\n", ret);
    g_error++;
  }

}

#define COREADDR(a)  ((((a) & 0xC) << 24) | (((a) & 3) << 20))

void MemTest() {
  int seed, n, m, core, errors, errtotal=0, loops=1, loop;
  unsigned int data[256], rdata[256], allmask;
  char command[STRINGMAX];
  long long int  ll;
  FILE  *logfile = NULL;
#ifdef ZTEMP
  float  temp;
#endif

  printf("\n\tMemTest\n");

  if(g_auto) {

    loops = 10;

    seed = time(NULL);
    srand(seed);

    for(n=0; n<256; n++)
      data[n] = rand();

  } else {  // user mode

    logfile = fopen("MemTest.log", "wa");
    if(logfile == NULL) {
      printf("ERROR: Unable to open log file\n");
      return;
    }

    printf("Constant data or Random?\n(C/R)> ");
    fgets(command, STRINGMAX, stdin);
    if((command[0] & 0x5F) == 'C') {

      printf("Enter 64b hex data value\n> 0x");
      fgets(command, STRINGMAX, stdin);
      if(sscanf(command, "%llx", &ll) < 1) {
	printf("Error parsing number\n");
	return;
      }

      for(n=0; n<256; n+=2)
	*((long long int *)(data + n)) = ll;

    } else {

      seed = time(NULL);
      printf("Using seed value of %d\n", seed);
      srand(seed);

      for(n=0; n<256; n++)
	data[n] = rand();
    }

    printf("# times to loop:\n> ");
    fgets(command, STRINGMAX, stdin);
    loops = atoi(command);
  }

  for(loop=1; loop<=loops; loop++) {

    errtotal = 0;
    allmask  = 0;

    for(core=0; core<16; core++) {
      if(loops == 1)
	printf("Core %d\n", core);
      errors = 0;

      for(m=0; m<32768; m+=1024) {

	//      printf("Core %d address 0x%X\n", core, EBASE + COREADDR(core));
	//      sleep(2);
	f_writearray(EBASE + COREADDR(core) + m, data, 1024);
	f_readarray(EBASE + COREADDR(core) + m, rdata, 1024);

	for(n=0; n<256; n++)
	  if(rdata[n] != data[n]) {

	    if(loops == 1)
	      printf("  0x%08X wanted 0x%08X got 0x%08X (0x%08X)\n",
		     EBASE+COREADDR(core)+m+(n*4),
		     data[n], rdata[n], data[n] ^ rdata[n]);

	    allmask |= data[n] ^ rdata[n];
	    errors++;
	  }

	//      printf("%06d> %d errors\n", m, errors);
      }

      if(loops == 1)
	printf("   %d errors\n", errors);
      errtotal += errors;
    }

#ifdef ZTEMP
    GetTemp(&temp);
    printf("Tz = %5.1f  ", temp);
    if(logfile)
      fprintf(logfile, "Tz = %5.1f  ", temp);
#endif

    printf("Loop %5d done, %6d errors total (0x%08X)\n",
	   loop, errtotal, allmask);
    if(logfile)
      fprintf(logfile, "Loop %5d done, %6d errors total (0x%08X)\n",
	      loop, errtotal, allmask);

    if(errtotal) g_error++;
  }

}

void PeekPoke() {
  char command[STRINGMAX], *arg1, *last;
  unsigned base, addr, data, rdata;

  printf("Peek/Poke: Enter E for epiphany or A for arm addresses\n");
  fgets(command, STRINGMAX, stdin);
  command[0] &= 0x5F;
  if(command[0] == 'E')
    base = EBASE;
  else if(command[0] == 'A')
    base = ASHAREBASE;
  else {
    printf("ERROR: Please enter E or A\n");
    return;
  }

  do {
    printf("Enter r/w/c address [data/count], or q to quit: ");
    fgets(command, STRINGMAX, stdin);

    command[0] &= 0x5F;

    if(command[0] == 'Q')
      break;

    if(command[0] != 'R' && command[0] != 'W' && command[0] != 'C') {
      printf("First character must be r, w, c, or q.\n");
      continue;
    }

    if(!sscanf(command+2, "0x%x", &addr))
      if(!sscanf(command+2, "%d", &addr)) {
	printf("Can't understand!\n");
	continue;
      }

    arg1 = command+1;
    do {
      last = arg1;
      arg1 = strchr(last+1, ' ');
    } while(arg1 == last + 1);

    if(!arg1 && command[0] == 'W') {
      printf("Please enter both address & data separated by a space.\n");
      continue;
    } else if(!arg1) {
      data = 1;
    } else {

      if(!sscanf(arg1+1, "0x%x", &data))
	if(!sscanf(arg1+1, "%d", &data)) {
	  printf("Can't understand second argument!\n");
	  continue;
	}
    }

    if(command[0] == 'W') {

      f_write(base + addr, data);
      printf("  (0x%08X) <- 0x%08X\n", base+addr, data);

    } else if(command[0] == 'R') {

      while(data--) {
	f_read(base+addr, &rdata);
	printf("  (0x%08X) -> 0x%08X\n", base+addr, rdata);
	addr += 4;
      }
    } else {  // Clear

      while(data--) {
	f_write(base+addr, 0);
	addr += 4;
      }
      printf("Cleared\n");
    }

  } while(1);
}

#define DESCADDR 0x1000

void DmaTest() {
  unsigned int  data, datah, l, n, inl, inh, errors=0;
  unsigned long long int dataw, inw, mask;

  printf("DMA test\n\tEpiphany -> SDRAM\n");

  inl = 0xDEADBEEF;
  inh = 0xACCE55ED;
  inw = inh;
  inw <<= 32;
  inw |= inl;

  f_write(EBASE, inl);  // Value to be moved
  f_write(EBASE+4, inh);  // For double-word move
  f_write(ASHAREBASE, 0x0);  // Destination for data
  f_write(ASHAREBASE+4, 0x0);

  f_read(ASHAREBASE, &data);
  f_read(ASHAREBASE+4, &datah);
  printf("Starting value: 0x%08X:%08X\n", datah, data);

  for(l=0; l<4; l++) {  // try all different sizes

    mask = (l == 3) ? -1LL :
      ((1ULL << (8 << l)) - 1);

    for(n=0; n<8; n += (1<<l)) {

      f_write(ASHAREBASE, 0x0);
      f_write(ASHAREBASE+4, 0x0);

      // source, dest, count, size
      if(Dma1D(n, ESHAREBASE, 1, l)) {
        printf("ERROR!\n");
	return;
      }

      f_read(ASHAREBASE, &data);
      f_read(ASHAREBASE+4, &datah);
      printf("Xfer %d/%d       -> 0x%08X:%08X\n", l, n, datah, data);

      dataw = datah;
      dataw <<= 32;
      dataw |= data;
      if(dataw != ((inw >> (8*n)) & mask)) {
	printf("ERROR");
	errors++;
      }
    }

    for(n = (1<<l); n<8; n += (1<<l)) {

      f_write(ASHAREBASE, 0x0);
      f_write(ASHAREBASE+4, 0x0);

      // source, dest, count, size
      if(Dma1D(0, ESHAREBASE+n, 1, l))
	return;

      f_read(ASHAREBASE, &data);
      f_read(ASHAREBASE+4, &datah);
      printf("Xfer %d/-%d      -> 0x%08X:%08X\n", l, n, datah, data);

      dataw = datah;
      dataw <<= 32;
      dataw |= data;
      if(dataw != ((inw & mask) << (8*n))) {
	printf("ERROR\n");
	errors++;
      }
    }
  }

  f_read(EBASE + E_REG_DMA0COUNT, &data);
  printf("Count = 0x%08X\n", data);

  printf("\n\tSDRAM -> Epiphany\n");

  inl = 0xCABBA6E5;
  inh = 0xBABBA6E5;
  inw = inh;
  inw <<= 32;
  inw |= inl;

  f_write(EBASE, 0);     // Destination
  f_write(EBASE+4, 0);   // " For double-word move
  f_write(ASHAREBASE, inl);  // Source data
  f_write(ASHAREBASE+4, inh);

  f_read(EBASE, &data);
  f_read(EBASE+4, &datah);
  printf("Starting value: 0x%08X:%08X\n", datah, data);

  for(l=0; l<4; l++) {  // try all different sizes

    mask = (l == 3) ? -1LL :
      ((1ULL << (8 << l)) - 1);

    for(n=0; n<8; n += (1 << l)) {

      f_write(EBASE, 0);
      f_write(EBASE+4, 0);

      if(Dma1D(ESHAREBASE+n, 0, 1, l))
	return;

      f_read(EBASE, &data);
      f_read(EBASE+4, &datah);
      printf("Xfer %d/%d       -> 0x%08X:%08X\n", l, n, datah, data);

      dataw = datah;
      dataw <<= 32;
      dataw |= data;
      if(dataw != ((inw >> (8*n)) & mask)){
	printf("ERROR\n");
	errors++;
      }
    }

    for(n = (1<<l); n<8; n += (1 << l)) {

      f_write(EBASE, 0);
      f_write(EBASE+4, 0);

      if(Dma1D(ESHAREBASE, n, 1, l))
	return;

      f_read(EBASE, &data);
      f_read(EBASE+4, &datah);
      printf("Xfer %d/-%d      -> 0x%08X:%08X\n", l, n, datah, data);

      dataw = datah;
      dataw <<= 32;
      dataw |= data;
      if(dataw != ((inw & mask) << (8*n))) {
	printf("ERROR\n");
	errors++;
      }
    }
  }

  f_read(EBASE + E_REG_DMA0COUNT, &data);
  printf("Count = 0x%08X\n", data);


  if(errors) {
    printf("%d ERRORS detected!\n", errors);
    g_error++;
  }
}

#define STREAMXFERS  256

void StreamTest() {
  char command[STRINGMAX];
  unsigned int  datah, datal, n, l, beats, errors=0;
  long long int data[STREAMXFERS], rdata[STREAMXFERS];

  printf("\n\tDMA Streaming-write test, Epiphany -> SDRAM\n");

  if(g_auto) {

    l = 3;  // doublewords
    beats = sizeof(data) / (1<<l);

  } else {

    printf("Enter size: 0=bytes, 1=halfwords 2=words 3=doublewords\n");
    fgets(command, STRINGMAX, stdin);
    l = atoi(command);
    beats = sizeof(data) / (1<<l);

    printf("Enter # of beats (max %d):\n", beats);
    fgets(command, STRINGMAX, stdin);
    beats = atoi(command);
    if(beats > sizeof(data) / (1<<l))
    beats = sizeof(data) / (1<<l);
  }

  printf("Moving %d bytes total.\n", beats * (1<<l));

  printf("Clearing SDRAM buffer...\n");
  if(g_cautious)
    sleep(1);

  memset(data, 0, sizeof(data));
  f_writearray(ASHAREBASE, (unsigned *)data, sizeof(data));

  printf("Writing Epiphany buffer...\n");
  if(g_cautious)
    sleep(1);

  for(n=0; n<STREAMXFERS; n++)
    data[n] = n * 0x0001000100010001LL + 0x0003000200010000LL;
  f_writearray(EBASE, (unsigned *)data, sizeof(data));

  printf("Reading-back from destination buffer...\n");
  if(g_cautious)
    sleep(1);

  f_read(ASHAREBASE, &datal);
  f_read(ASHAREBASE+4, &datah);
  printf("Starting value: 0x%08X:%08X\n", datah, datal);

  printf("Doing DMA, size=%d, count=%d...\n", l, beats);
  if(g_cautious)
    sleep(1);

  //  if(Dma1D(0, ESHAREBASE, STREAMXFERS, l))
  if(Dma1D(0, ESHAREBASE, beats, l))
    return;

  printf("Reading destination buffer...\n");
  if(g_cautious)
    sleep(1);

  f_read(ASHAREBASE, &datal);
  f_read(ASHAREBASE+4, &datah);
  printf("Xfer %d       -> 0x%08X:%08X\n", l, datah, datal);

  f_readarray(ASHAREBASE, (unsigned *)rdata, sizeof(rdata));

  for(n=0; n<(beats*(1<<l)/8); n++)
    if(rdata[n] != data[n]) {
        printf("i: %d want: 0x%.16jx got: 0x%.16jx\n", n, data[n], rdata[n]);

      errors++;
    }

  printf("\nFound %d errors out of %d dwords.\n", errors, beats*(1<<l)/8);
  if(errors)
    g_error++;
}

void MemAccess64() {
  long long int  data, rdata;
  volatile long long int  *llptr;
  unsigned offset;

  if(f_map(EBASE, (void **)&llptr, &offset)) {
    printf("ERROR, can't map into Epiphany space\n");
    return;
  }

  printf("Reading from core(0,0), address 0, 64b:\n");
  usleep(100000);

  data = *llptr;

  printf("  0x%016llX\nSending inverse, reading:\n", data);
  usleep(100000);

  *llptr = ~data;
  rdata = *llptr;

  printf("  Got 0x%016llX (0x%016llX)\n", rdata, data | rdata);

  if((data | rdata) != 0xFFFFFFFFFFFFFFFFULL)
    g_error++;

  f_unmap((void *)llptr);
}

void SetCautious() {

  g_cautious = !g_cautious;
  printf("\nCautious mode %s\n\n", g_cautious?"ON":"OFF");
}

void* memcpy_random(void *dst, void *src, size_t count)
{
  int i;
  uint8_t *u8_dst = (uint8_t *) dst;
  uint8_t *u8_src = (uint8_t *) src;
  size_t chunk;

  size_t chunk_sizes[] = {1, 3, 137, 8, 257, 4, 256, 99, 23, 44, 6, 55};
  //size_t chunk_sizes[] = {1, 8};

  for (i=0; count > 0; i++) {
    /* chunk = (i%2) ? 1 : 8; */ /* <-- even this would fail */

    chunk = chunk_sizes[i%(sizeof(chunk_sizes)/sizeof(chunk_sizes[0]))];

    if (chunk > count)
      chunk = count;

    if (u8_dst != (uint8_t *) memcpy((void *) u8_dst, (void *) u8_src, chunk))
      return NULL;

    u8_dst += chunk;
    u8_src += chunk;
    count  -= chunk;
  }
  return dst;
}

#define OLABUFSIZE 0x8000

void OlaTest() {
  uint64_t expected, read, tmp, allerrs=0ULL;
  volatile uint64_t  *baseptr, *llptr;
  uint32_t offset;
  unsigned base = EBASE;
  int errors = 0;
  int i,j;
  char  command[STRINGMAX];

  // Reference buffer
  uint64_t buf[OLABUFSIZE>>3];

  // Read back buffer
  uint64_t rbuf[OLABUFSIZE>>3];

  printf("Ola Test #1\n");

  if(g_auto) {

    command[0] = 'R';

  } else {

    printf("Random >R<eads or >W<rites?\n");
    fgets(command, STRINGMAX, stdin);

  }

  if((command[0]&0x5F) == 'R') {

    printf("Testing \"Random\" reads over %d bytes.\n", OLABUFSIZE);

    // Write something "random" that uses all bits into reference buffer
    for (offset=0; offset < OLABUFSIZE; offset+=8) {
      tmp = (((~((uint64_t) offset)) &(0xffffULL))       << 48ULL  |
	     (   (uint64_t) offset)                      << 32ULL  |
	     ((~((uint64_t) offset)) &(0xffffffffULL)));

      buf[offset>>3] = tmp;
    }

    for (i=0; i < ROWS; i++, base += ((64-COLS)<<20)) {
      for (j=0; j < COLS; j++, base += (1<<20)) {

	printf("Testing core %d,%d\n", i, j);

	if(f_map_sz(base, (void **)&baseptr, NULL, OLABUFSIZE)) {
	  printf("ERROR, can't map into Epiphany space\n");
	  return;
	}

	// Copy reference buffer w/ normal memcpy
	if (baseptr != memcpy((void *)baseptr, (void *)buf, sizeof(buf))) {
	  printf("memcpy() failed\n");
	  return;
	}

	// Clear the read buffer
	memset(rbuf, 0, sizeof(rbuf));

	// Read it back in "random" chunks
	if (rbuf != memcpy_random(rbuf, (void *)baseptr, sizeof(buf))) {
	  printf("memcpy_random() failed\n");
	  return;
	}

	// Walk buffers and compare data
	for (offset = 0, llptr = rbuf; offset < OLABUFSIZE; offset += 8, llptr++) {
	  expected = buf[offset>>3];
	  read = *llptr;
	  if (read != expected) {
	    printf("Error core(%d,%d) @0x%X\n ", i, j, offset);
	    printf("expected:  0x%016llX ", expected);
	    printf("read: 0x%016llX ", read);
	    printf("xor: 0x%016llX\n", expected ^ read);
	    allerrs |= expected ^ read;
	    errors++;
	  }
	}
	f_unmap_sz((void *)baseptr, OLABUFSIZE);
      }
    }

  } else {

    printf("\nTesting \"Random\" writes over %d bytes.\n", OLABUFSIZE);

    // Write something "random" that uses all bits into reference buffer
    for (offset=0; offset < OLABUFSIZE; offset+=8) {
      tmp = (((~((uint64_t) offset)) &(0xffffULL))       << 48ULL  |
	     (   (uint64_t) offset)                      << 32ULL  |
	     ((~((uint64_t) offset)) &(0xffffffffULL)));

      // Make it different from before
      buf[offset>>3] = tmp ^ 0x0123456789ABCDEFULL;
    }

    base = EBASE;

    for (i=0; i < ROWS; i++, base += ((64-COLS)<<20)) {
      for (j=0; j < COLS; j++, base += (1<<20)) {

	printf("Testing core %d,%d\n", i, j);

	if(f_map_sz(base, (void **)&baseptr, NULL, OLABUFSIZE)) {
	  printf("ERROR, can't map into Epiphany space\n");
	  return;
	}

	// Copy reference buffer in random chunks
	if (baseptr != memcpy_random((void *)baseptr, (void *)buf, sizeof(buf))) {
	  printf("memcpy_random() failed\n");
	  return;
	}

	// Clear the read buffer
	memset(rbuf, 0, sizeof(rbuf));

	// Read it back normally
	if (rbuf != memcpy(rbuf, (void *)baseptr, sizeof(buf))) {
	  printf("memcpy() failed\n");
	  return;
	}

	// Walk buffers and compare data
	for (offset = 0, llptr = rbuf; offset < OLABUFSIZE; offset += 8, llptr++) {
	  expected = buf[offset>>3];
	  read = *llptr;
	  if (read != expected) {
	    printf("Error core(%d,%d) @0x%X\n ", i, j, offset);
	    printf("expected:  0x%016llX ", expected);
	    printf("read: 0x%016llX ", read);
	    printf("xor: 0x%016llX\n", expected ^ read);
	    allerrs |= expected ^ read;
	    errors++;
	  }
	}
	f_unmap_sz((void *)baseptr, OLABUFSIZE);
      }
    }
  }

  if (!errors) {
    printf("Everything seems ok\n");
  } else {
    printf("%d errors reported, accumulated XOR: 0x%016llX\n", errors, allerrs);
    g_error++;
  }

}

void OlaTest2() {
  unsigned int offset, test, done, errors=0;
  volatile uint8_t *p;
  volatile uint8_t *q;
  volatile uint8_t *base;

  volatile uint64_t out[2] = { 0x0123456789abcdefULL, 0xfedcba9876543210ULL };
  volatile uint64_t in[2] = { 0 };

  if(f_map(EBASE, (void **)&base, &offset)) {
    printf("Can't map memory\n");
    return;
  }

  printf("Ola Test #2\n");

  for(test=1, done=0; !done; test++) {

    printf("Test %d ", test);
    if(g_cautious) sleep(1);

    // Initialize e-ram
    memcpy((void *)base, (void *)out, sizeof(out));
    // Clear the local memory
    memset((void *) in, 0, sizeof(in));

    switch(test) {

    case 1:
      printf("memcpy(in, base, sizeof(in))...\n");
      if(g_cautious) sleep(1);
      memcpy((void *)in, (void *)base, sizeof(in));
      break;

    case 2:
      printf("8/8/16/32/64 direct reads...\n");
      if(g_cautious) sleep(1);
      p = base;
      q = (uint8_t *) in;

      *q++ = *p++;
      *q++ = *p++;
      *((uint16_t *) q) = *((uint16_t *) p);
      p += 2; q += 2;
      *((uint32_t *) q) = *((uint32_t *) p);
      p += 4; q += 4;
      *((uint64_t *) q) = *((uint64_t *) p);
      break;

    case 3:
      printf("mem-mem mixed reads...\n");
      if(g_cautious) sleep(1);
      p = (void *) out;
      q = (uint8_t *) in;
      *q++ = *p++;
      memcpy((void *) q, (void *) p, 8);
      q +=8; p += 8;
      *q++ = *p++;
      memcpy((void *) q, (void *) p, 6);
      break;

    case 4:
      printf("mixed reads from e-mem...\n");
      if(g_cautious) sleep(1);
      p = base;
      q = (uint8_t *) in;
      *q++ = *p++;
      memcpy((void *) q, (void *) p, 8);
      q +=8; p += 8;
      *q++ = *p++;
      memcpy((void *) q, (void *) p, 6);
      done = 1;  // last test
      break;

    default:
      done = 1;
    }

    printf("In:\t0x%llX 0x%llX\n",  out[0], out[1]);
    printf("Out:\t0x%llX 0x%llX\n", in[0],  in[1]);
    if(out[0] != in[0] || out[1] != in[1]) {
      printf("  FAILED!\n");
      errors++;
    } else {
      printf("  PASSED\n");
    }
  }

  f_unmap((void *)base);

  printf("Done\n\n");

  if(errors)
    g_error++;
}

void TimerTest() {
  uint32_t  reg;

  printf("Timer Test, please wait 2 seconds...\n");

  f_read(EBASE+E_REG_CONFIG, &reg);
  reg &= ~(0xF << 4);  // Off first
  f_write(EBASE+E_REG_CONFIG, reg);

  f_write(EBASE+E_REG_CTIMER0, 0xFFFFFFFF);

  reg |= (1 << 4);
  f_write(EBASE+E_REG_CONFIG, reg);

  sleep(2);

  reg &= ~(0xF << 4);
  f_write(EBASE+E_REG_CONFIG, reg);

  f_read(EBASE+E_REG_CTIMER0, &reg);
  reg = 0xFFFFFFFF - reg;

  printf("%u cycles in 2 seconds = %0.2fMHz\n", reg, (double)reg / 2. / 1.0e6);

  printf("Done!\n\n");
}

void ScriptTest() {
  char cmd[STRINGMAX];
  int  ret;

  if(!g_auto) {

    printf("Enter script to run (<return> for %s)\n", g_scriptname);
    fgets(cmd, STRINGMAX, stdin);

    if(cmd[0] != '\n')
      strncpy(g_scriptname, cmd, STRINGMAX);
  }

  printf("Running \"%s\"\n", g_scriptname);

  ret = system(g_scriptname);

  if(ret) {
    printf("FAILED %d\n", ret);
    g_error++;
  } else {
    printf("PASSED\n");
  }
}

void TXSpeed() {
  int            ret;
  e_sys_txcfg_t  txcfg;

  printf("Setting Epiphany eLink TX speed to 1/2\n");
  usleep(100000);

  do {

    ret = f_read(E_SYS_CFGTX, &txcfg.reg);
    if(ret) break;

    // Set "Direct East" routing mode
    txcfg.ctrlmode = 5;
    ret = f_write(E_SYS_CFGTX, txcfg.reg);
    if(ret) break;

    ret = f_write(EBASE + COREADDR((2<<2)+3) + 0xF0300, 1);
    if(ret) break;

    // return to normal routing
    txcfg.ctrlmode = 0;
    ret = f_write(E_SYS_CFGTX, txcfg.reg);
    if(ret) break;

  } while(0);

  if(ret)
    printf("ERROR, got return value of %d\n", ret);
  else
    printf("Done\n");

}

void ClockGate() {
  static int gateEnable = 0;
  unsigned core, data, addr;

  gateEnable = !gateEnable;
  printf("Setting all clock-gating bits to %d\n", gateEnable);

  for (core=0; core<16; core++) {

    //eCore clock gating
    addr = EBASE + COREADDR(core) + 0xF0400;
    f_read(addr, &data);
    data = gateEnable ? (data | 0x00400000) : (data & ~0x00400000);
    printf("0x%08X <- 0x%08X\n", addr, data);
    f_write(addr, data);

    //eMesh clock gating
    addr = EBASE + COREADDR(core) + 0xF0700;
    f_read(addr, &data);
    data = gateEnable ? (data | 0x00000002) : (data & ~0x00000002);
    printf("0x%08X <- 0x%08X\n", addr, data);
    f_write(addr, data);

  }
}

#if 0
void my_reset_system() {

  e_init(NULL);
  e_get_platform_info(&platform);
  ee_write_esys(E_SYS_RESET, 0);//reset
  usleep(200000);

  //Open all cores
  e_open(&dev, 0, 0, platform.rows, platform.cols);

  //shut down north link
  if(1){
      row=0;
      col=2;

      ee_write_esys(E_SYS_CONFIG, 0x10000000);
      data = 0x000000FFF;
      e_write(&dev, row, col, 0xf0304, &data, sizeof(int));
      data = 0x000000FFF;
      e_write(&dev, row, col, 0xf0308, &data, sizeof(int));
      ee_write_esys(E_SYS_CONFIG, 0x00000000);
  }

  //Shut down west link (WEST==2,0)
  if(1){
    row=2;
    col=0;
    ee_write_esys(E_SYS_CONFIG, 0xd0000000);
    data = 0x000000FFF;
    e_write(&dev, row, col, 0xf0304, &data, sizeof(int));
    data = 0x000000FFF;
    e_write(&dev, row, col, 0xf0308, &data, sizeof(int));
    ee_write_esys(E_SYS_CONFIG, 0x00000000);
  }

  //Shut down south link (SOUTH==7,2)
  if(1){
    if ((dev.type == E_E64G401)){
      row=7;
      col=2;
    }
    else{
      row=3;
      col=2;
    }

    ee_write_esys(E_SYS_CONFIG, 0x90000000);
    data = 0x000000FFF;
    e_write(&dev, row, col, 0xf0304, &data, sizeof(int));
    data = 0x000000FFF;
    e_write(&dev, row, col, 0xf0308, &data, sizeof(int));
    ee_write_esys(E_SYS_CONFIG, 0x00000000);
  }

   //Change elink clock divider (temporary workaround due to FPGA timing issue)
  if(1){
    //east link register is in a different place in e64
    if ((dev.type == E_E64G401)){
      row=2;
      col=7;
    }
    else{
      row=2;
      col=3;
    }
    //Writing to the east ELINK transmit config register
    ee_write_esys(E_SYS_CONFIG, 0x50000000);
    data = 0x1;
    e_write(&dev, row, col, 0xf0300, &data, sizeof(int));
    ee_write_esys(E_SYS_CONFIG, 0x00000000);
  }

 //Reset chip one more time (west side))
  if(0){
    row=2;
    col=0;
    ee_write_esys(E_SYS_CONFIG, 0xd0000000);
    data = 0x000000001;
    e_write(&dev, row, col, 0xf0324, &data, sizeof(int));
    ee_write_esys(E_SYS_CONFIG, 0x00000000);
  }

  //Enable Clock Gating
  if(0){
    for (i=0; i<platform.rows; i++) {
      for (j=0; j<platform.cols; j++) {
  	//eCore clock gating
	data=0x00400000;
	e_write(&dev, i, j, 0xf0400, &data, sizeof(data));
	//eMesh clock gating
	data=0x00000002;
	e_write(&dev, i, j, 0xf0700, &data, sizeof(data));
      }
    }
  }

  //Close down device
  e_close(&dev);
  return E_OK;
}

#endif

int Dma1D(unsigned int srcaddr, unsigned int dstaddr,
	  unsigned int count, unsigned int size) {
  int n, stride;
  e_dma_stat_t  dmast;

  // Set-up core 0,0 DMA descriptor (must be local)
  f_write(EBASE + DESCADDR,   // Config bits
	  E_DMA_ENABLE |
	  E_DMA_MASTER |
	  ((size & 3) << 5));  // 0=byte, 1=hword, 2=word, 3=dword

  stride = (1<<size) * 0x00010001;

  f_write(EBASE + DESCADDR + 4, stride);  // Inner stride dest:source
  f_write(EBASE + DESCADDR + 8, 0x00010000 + count);  // Count- outer:inner
  f_write(EBASE + DESCADDR +12, 0x00100010);  // Outer stride dest:source
  f_write(EBASE + DESCADDR +16, srcaddr);  // Source address
  f_write(EBASE + DESCADDR +20, dstaddr);  // Destination address

  // Kick it off!
  f_write(EBASE + E_REG_DMA0CONFIG, (DESCADDR << 16) |
	  E_DMA_STARTUP);

  if(!g_cautious) {

    for(n=0; n<1000; n++) {
      f_read(EBASE + E_REG_DMA0STATUS, (unsigned int *)(&dmast));
      if(dmast.dmastate == E_DMA_IDLE)
	break;
    }

    if(dmast.dmastate != E_DMA_IDLE) {
      printf("ERROR: Timeout waiting for DMA (%d)\n", dmast.dmastate);
      return -1;
    }
  }

  return 0;
}

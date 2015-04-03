/*
  File: elink2.h

  This file is part of the Epiphany Software Development Kit.

  Copyright (C) 2014 Adapteva, Inc.
  See AUTHORS for list of contributors.
  Support e-mail: <support@adapteva.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License (LGPL)
  as published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  and the GNU Lesser General Public License along with this program,
  see the files COPYING and COPYING.LESSER.  If not, see
  <http://www.gnu.org/licenses/>.
*/

#ifndef E_LINK2_H
#define E_LINK2_H

// New Epiphany system registers
typedef enum {
	E_SYS_REG_BASE  = 0x70000000,
	E_SYS_RESET     = E_SYS_REG_BASE + 0x0040,
	E_SYS_CFGTX     = E_SYS_REG_BASE + 0x0044,
	E_SYS_CFGRX     = E_SYS_REG_BASE + 0x0048,
	E_SYS_CFGCLK    = E_SYS_REG_BASE + 0x004C,
	E_SYS_COREID    = E_SYS_REG_BASE + 0x0050,
	E_SYS_VERSION   = E_SYS_REG_BASE + 0x0054,
	E_SYS_GPIOIN    = E_SYS_REG_BASE + 0x0058,
	E_SYS_GPIOOUT   = E_SYS_REG_BASE + 0x005C,
} e_sys_reg_id_t;

typedef struct e_syscfg_tx_st {
  unsigned int   enable:1;
  unsigned int   mmu:1;
  unsigned int   mode:2;      // 0=Normal, 1=GPIO
  unsigned int   ctrlmode:4;
  unsigned int   clkmode:4;   // 0=Full speed, 1=1/2 speed
  unsigned int   resvd:20;
} e_syscfg_tx_t;

typedef union {
  e_syscfg_tx_t s;  // Can't do anonymous structures in a union?
  unsigned int reg;
} e_syscfg_tx_u;

typedef struct {
  unsigned int  enable:1;
  unsigned int  mmu:1;
  unsigned int  path:2;    // 0=Normal, 1=GPIO, 2=Loopback
  unsigned int  monitor:1;
  unsigned int  resvd:27;
} e_syscfg_rx_t;

typedef union {
  e_syscfg_rx_t s;
  unsigned int reg;
} e_syscfg_rx_u;

typedef struct {
  unsigned int  divider:4;  // 0=off, 1=F/64 ... 7=F/1
  unsigned int  pll:4;      // TBD
  unsigned int  resvd:24;
} e_syscfg_clk_t;

typedef union {
  e_syscfg_clk_t s;
  unsigned int  reg;
} e_syscfg_clk_u;

typedef struct {
  unsigned int  col:6;
  unsigned int  row:6;
  unsigned int  resvd:20;
} e_syscfg_coreid_t;

typedef union {
  e_syscfg_coreid_t s;
  unsigned int  reg;
} e_syscfg_coreid_u;

typedef struct {
  unsigned char  revision;
  unsigned char  type;
  unsigned char  platform;
  unsigned char  generation;
} e_syscfg_version_t;

typedef union {
  e_syscfg_version_t s;
  unsigned int  reg;
} e_syscfg_version_u;

// The following is for E_CFG_SYSDATA_IN or E_CFG_SYSDATA_OUT
typedef struct {
  unsigned int  data:8;
  unsigned int  frame:1;
  unsigned int  wait_rd:1;
  unsigned int  wait_wr:1;
  unsigned int  resvd:21;
} e_syscfg_gpio_t;

typedef union {
  e_syscfg_gpio_t s;
  unsigned int  reg;
} e_syscfg_gpio_u;

// Functions in f-link.c:
int  f_map(unsigned addr, void **ptr, unsigned *offset);
void f_unmap(void *ptr);
int  f_map_sz(unsigned addr, void **ptr, unsigned *offset, unsigned size);
void f_unmap_sz(void *ptr, unsigned size);
  // 32b read/write
int f_read(unsigned addr, unsigned *data);
int f_write(unsigned addr, unsigned data);
  // array read/write
int f_readarray(unsigned addr, unsigned *data, unsigned bytes);
int f_writearray(unsigned addr, unsigned *data, unsigned bytes);

#endif  // E_LINK2_H

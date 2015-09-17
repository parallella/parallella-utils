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
	E_SYS_REG_BASE  = 0x81000000,
	E_SYS_RESET     = E_SYS_REG_BASE + 0xF0200,
	E_SYS_CFGCLK    = E_SYS_REG_BASE + 0xF0204,
	E_SYS_CHIPID    = E_SYS_REG_BASE + 0xF0208,
	E_SYS_TXGPIO    = E_SYS_REG_BASE + 0xF0218, /* ETX_GPIO */
	E_SYS_VERSION   = E_SYS_REG_BASE + 0xF020C,
	E_SYS_CFGTX     = E_SYS_REG_BASE + 0xF0210,
	E_SYS_CFGRX     = E_SYS_REG_BASE + 0xF0300,
	E_SYS_RXGPIO    = E_SYS_REG_BASE + 0xF0308, /* ERX_GPIO */
} e_sys_reg_id_t;

// #include "/opt/adapteva/esdk/tools/host.armv7l/include/epiphany-hal-data.h"

typedef union {
	unsigned int reg;
	struct {
		unsigned int reset:1;
//		unsigned int chip_reset:1;
//		unsigned int reset:1;
	};
} e_sys_reset_t;


typedef union {
	unsigned int reg;
	struct {
		unsigned int cclk_enable:1;
		unsigned int lclk_enable:1;
		unsigned int cclk_bypass:1;
		unsigned int lclk_bypass:1;
		unsigned int cclk_divider:4;
		unsigned int lclk_divider:4;
	};
} e_sys_clkcfg_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int col:6;
		unsigned int row:6;
	};
} e_sys_chipid_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int platform:8;
		unsigned int revision:8;
	};
} e_sys_version_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int enable:1;
		unsigned int mmu_enable:1;
		unsigned int mmu_cfg:2;
		unsigned int ctrlmode:4;
		unsigned int ctrlmode_select:1;
		unsigned int transmit_mode:3;
	};
} e_sys_txcfg_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int enable:1;
		unsigned int mmu_enable:1;
		unsigned int mmu_cfg:2;
		unsigned int remap_mask:12;
		unsigned int remap_base:12;
		unsigned int timeout:2;
	};
} e_sys_rxcfg_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int enable:1;
		unsigned int master_mode:1;
		unsigned int __reserved1:3;
		unsigned int width:2;
		unsigned int __reserved2:3;
		unsigned int message_mode:1;
		unsigned int src_shift:1;
		unsigned int dst_shift:1;
	};
} e_sys_rx_dmacfg_t;

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

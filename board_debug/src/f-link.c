/*
f-link.c

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

//#define BROKEN_64B_WRITES
//#define BROKEN_64B_READS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "elink2.h"

unsigned page_size = 0;
int      mem_fd = -1;

int f_read(unsigned addr, unsigned *data) {
  int  ret;
  unsigned offset;
  char *ptr;

  ret = f_map(addr, (void **)&ptr, &offset);

  if(ret)
    return ret;

  /* Read value from the device register */
  *data = *((unsigned *)(ptr + offset));

  f_unmap(ptr);

  return 0;
}

int f_write(unsigned addr, unsigned data) {
  int  ret;
  unsigned offset;
  char *ptr;

  ret = f_map(addr, (void **)&ptr, &offset);

  if(ret)
    return ret;

  /* Write value to the device register */
  *((unsigned *)(ptr + offset)) = data;

  f_unmap(ptr);

  return 0;
}

int f_readarray(unsigned addr, unsigned *data, unsigned bytes) {
  unsigned  offset;
  char     *pptr, *vptr;
  int       ret, segsize;

  vptr = (char *)data;

  while(bytes) {

    ret = f_map(addr, (void **)&pptr, &offset);
    if(ret) return ret;

    if(offset + bytes > page_size)
      segsize = page_size - offset;
    else
      segsize = bytes;

#ifdef BROKEN_64B_READS
    int n;
    for(n=0; n<segsize; n+=4, offset+=4)
      *((unsigned *)(vptr + n)) = *((unsigned *)(pptr + offset));
#else
    memcpy(vptr, pptr + offset, segsize);
#endif

    f_unmap(pptr);

    bytes -= segsize;
    addr += segsize;
    vptr += segsize;
  }

  return 0;
}

int f_writearray(unsigned addr, unsigned *data, unsigned bytes) {
  unsigned  offset;
  char     *pptr, *vptr;
  int       ret, segsize;

  vptr = (char *)data;

  while(bytes) {

    ret = f_map(addr, (void **)&pptr, &offset);
    if(ret) return ret;

    if(offset + bytes > page_size)
      segsize = page_size - offset;
    else
      segsize = bytes;

#ifdef BROKEN_64B_WRITES
    int n;
    for(n=0; n<segsize; n+=4, offset+=4)
      *((unsigned *)(pptr + offset)) = *((unsigned *)(vptr + n));
#else
    memcpy(pptr + offset, vptr, segsize);
#endif
    f_unmap(pptr);

    bytes -= segsize;
    addr += segsize;
    vptr += segsize;
  }

  return 0;
}

int f_map(unsigned addr, void **ptr, unsigned *offset) {
  unsigned page_addr;
 
  if(!page_size)
    page_size = sysconf(_SC_PAGESIZE);

  /* Open /dev/mem file if not already */
  /* Assume this will be closed on program exit */
  if(mem_fd < 1) {
    mem_fd = open ("/dev/epiphany/mesh0", O_RDWR);
    //mem_fd = open ("/dev/epiphany", O_RDWR);
    //mem_fd = open ("/dev/mem", O_RDWR);
    if (mem_fd < 1) {
      perror("f_map");
      return -1;
    }
  }

  /* mmap the device into memory */
  page_addr = (addr & (~(page_size-1)));

  if(offset != NULL)
    *offset = addr - page_addr;

  *ptr = mmap(NULL, page_size, PROT_READ|PROT_WRITE, MAP_SHARED, 
	      mem_fd, page_addr);

  if(*ptr == MAP_FAILED || !*ptr) {
    perror(NULL);
    return -2;
  }


  return 0;
}	

void f_unmap(void *ptr) {
  
  if(ptr && page_size)
    munmap(ptr, page_size);
}

int f_map_sz(unsigned addr, void **ptr, unsigned *offset, unsigned size) {
  unsigned page_addr;
 
  if(!page_size)
    page_size = sysconf(_SC_PAGESIZE);

  /* Open /dev/mem file if not already */
  /* Assume this will be closed on program exit */
  if(mem_fd < 1) {
    mem_fd = open ("/dev/epiphany/elink0", O_RDWR);
    //mem_fd = open ("/dev/epiphany", O_RDWR);
    //mem_fd = open ("/dev/mem", O_RDWR);
    if (mem_fd < 1) {
      perror("f_map");
      return -1;
    }
  }

  /* mmap the device into memory */
  page_addr = (addr & (~(page_size-1)));

  if(offset != NULL)
    *offset = addr - page_addr;

  *ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, 
	      mem_fd, page_addr);

  if(! *ptr)
    return -2;

  return 0;
}	

void f_unmap_sz(void *ptr, unsigned size) {
  
  if(ptr && page_size)
    munmap(ptr, size);
}

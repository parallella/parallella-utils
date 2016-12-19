#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SLCR_IDCODE_MASK	0x1F000
#define SLCR_IDCODE_SHIFT	12

#define SLCR_BASE		0xF8000000
#define PSS_IDCODE_OFFS		0x530

#define XILINX_ZYNQ_7010	0x2
#define XILINX_ZYNQ_7015	0x1b
#define XILINX_ZYNQ_7020	0x7
#define XILINX_ZYNQ_7030	0xc
#define XILINX_ZYNQ_7035	0x12
#define XILINX_ZYNQ_7045	0x11
#define XILINX_ZYNQ_7100	0x16

int decode(uint32_t idcode)
{
	idcode = (idcode & SLCR_IDCODE_MASK) >> SLCR_IDCODE_SHIFT;

	switch (idcode) {
	case XILINX_ZYNQ_7010:
		printf("7z010\n");
		return 0;
	case XILINX_ZYNQ_7015:
		printf("7z015\n");
		return 0;
	case XILINX_ZYNQ_7020:
		printf("7z020\n");
		return 0;
	case XILINX_ZYNQ_7030:
		printf("7z030\n");
		return 0;
	case XILINX_ZYNQ_7035:
		printf("7z035\n");
		return 0;
	case XILINX_ZYNQ_7045:
		printf("7z045\n");
		return 0;
	case XILINX_ZYNQ_7100:
		printf("7z100\n");
		return 0;

	default:
		printf("unknown\n");
		return 1;
	}
}

int main()
{
	uint32_t *slcr, *pss_idcode;
	int ret, fd;

	fd = open("/dev/mem", O_RDONLY | O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "Could not open /dev/mem\n");
		return 1;
	}

	slcr = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, SLCR_BASE);
	if (slcr == MAP_FAILED) {
		fprintf(stderr, "mmap failed\n");
		return 1;
	}

	pss_idcode = (uint32_t *)
		((uintptr_t) slcr + (uintptr_t) PSS_IDCODE_OFFS);

	ret = decode(*pss_idcode);

	munmap(slcr, 4096);
	close(fd);

	return ret;
}

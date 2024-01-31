/*
 * devmem2.c: Simple program to read/write from/to any location in memory.
 *
 *  Copyright (C) 2000, Jan-Derk Bakker (jdb@lartmaker.nl)
 *
 *
 * This software has been developed for the LART computing board
 * (http://www.lart.tudelft.nl/). The development has been sponsored by
 * the Mobile MultiMedia Communications (http://www.mmc.tudelft.nl/)
 * and Ubiquitous Communications (http://www.ubicom.tudelft.nl/)
 * projects.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <inttypes.h>

int
main(int argc, char **argv)
{
	int fd;
	void *map_base, *virt_addr;
	unsigned long long writeval;
	off_t target;
	int access_type = 'w';
	long page_size;

	if (argc < 2) {
		fprintf(stderr, "usage: %s address [type [data]]\n"
			"  address  memory address to act upon\n"
			"  type     access operation type\n"
			"            b  byte (8 bit)\n"
			"            h  halfword (16 bit)\n"
			"            w  word (32 bit)\n"
			"            d  doubleword (64 bit)\n"
			"  data     data to write\n",
			argv[0]);
		exit(1);
	}
	page_size = sysconf(_SC_PAGESIZE);
	setvbuf(stdout, NULL, _IONBF, 0);
	target = strtoull(argv[1], 0, 0);

	if (argc > 2)
		access_type = tolower(argv[2][0]);

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
		err(1, "/dev/mem");

	/* Map two pages */
	map_base = mmap(0, page_size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~(page_size - 1));
	if (map_base == MAP_FAILED)
		err(1, "mmap");
	printf("0x%" PRIx64 ": ", (uint64_t) target);

	virt_addr = map_base + (target & (page_size - 1));
	switch (access_type) {
#define READ(type, format) \
	do { \
		type value; \
		memcpy(&value, virt_addr, sizeof(value)); \
		printf(format, value); \
	} while (0)

	case 'b':
		READ(uint8_t, "0x%02" PRIx8);
		break;
	case 'h':
		READ(uint16_t, "0x%04" PRIx16);
		break;
	case 'w':
		READ(uint32_t, "0x%08" PRIx32);
		break;
	case 'd':
		READ(uint64_t, "0x%016" PRIx64);
		break;
	default:
		fprintf(stderr, "Illegal data type '%c'.\n", access_type);
		exit(2);
	}

	if (argc > 3) {
		writeval = strtoull(argv[3], 0, 0);
		switch (access_type) {
#define WRITE(type, format) \
	do { \
		printf(" -> " format " -> ", (type) writeval); \
		*((volatile type *) virt_addr) = writeval; \
	} while (0)

		case 'b':
			WRITE(uint8_t, "0x%02" PRIx8);
			READ(uint8_t, "0x%02" PRIx8);
			break;
		case 'h':
			WRITE(uint16_t, "0x%04" PRIx16);
			READ(uint16_t, "0x%04" PRIx16);
			break;
		case 'w':
			WRITE(uint32_t, "0x%08" PRIx32);
			READ(uint32_t, "0x%08" PRIx32);
			break;
		case 'd':
			WRITE(uint64_t, "0x%016" PRIx64);
			READ(uint64_t, "0x%016" PRIx64);
			break;
		}
	}
	printf("\n");

	if (munmap(map_base, page_size * 2) == -1)
		err(1, "munmap");
	close(fd);
	return 0;
}

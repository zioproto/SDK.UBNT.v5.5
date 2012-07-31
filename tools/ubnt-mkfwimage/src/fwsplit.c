/*
 * Copyright (C) 2007 Ubiquiti Networks, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <zlib.h>
#include <limits.h>
#include <unistd.h>


#include "fw.h"

static int debug = 0;
static char prefix[PATH_MAX];

#define MAX_PARTS 8

typedef struct fw_part {
	part_t* header;
	unsigned char* data;
	u_int32_t data_size;
	part_crc_t* signature;
} fw_part_t;

typedef struct fw {
	u_int32_t size;
	char version[256];
	fw_part_t parts[MAX_PARTS];
	int part_count;
} fw_t;

static int
fw_check_header(const header_t* h) {
        u_int32_t crc;
        int len = sizeof(header_t) - 2 * sizeof(u_int32_t);

        crc = crc32(0L, (unsigned char*)h, len);
	DEBUG("Calculated CRC: 0x%08X, expected: 0x%08X\n", htonl(crc), h->crc);
        if (htonl(crc) != h->crc)
                return -1;

        return 0;
}

static int
fw_parse(const unsigned char* base, unsigned long size, fw_t* fw) {
	const header_t* h = (const header_t*)base;
	part_t* p;
	signature_t* sig;
	u_int32_t crc;
	int i = 0;

	if (fw == NULL)
		return -1;

	if (fw_check_header(h)) {
		return -2;
	}
	memset(fw, 0, sizeof(fw_t));
	fw->size = size;
	memcpy(fw->version, h->version, sizeof(fw->version));
	INFO("Firmware version: '%s'\n", fw->version);

	p = (part_t*)(base + sizeof(header_t));
	i = 0;
	while (strncmp(p->magic, MAGIC_END, MAGIC_LENGTH) != 0) {
		DEBUG("Partition (%c%c%c%c): %s [%u]\n",
				p->magic[0], p->magic[1], p->magic[2], p->magic[3],
				p->name, ntohl(p->index));
		DEBUG("  Partition size: 0x%X\n", ntohl(p->part_size));
		DEBUG("  Data size: %u\n", ntohl(p->data_size));

		if ((strncmp(p->magic, MAGIC_PART, MAGIC_LENGTH) == 0) && (i < MAX_PARTS)) {
			fw_part_t* fwp = &fw->parts[i];

			fwp->header = p;
			fwp->data = (unsigned char*)p + sizeof(part_t);
			fwp->data_size = ntohl(p->data_size); 
			fwp->signature = 
				(part_crc_t*)(fwp->data + fwp->data_size);

			crc = htonl(crc32(0L, (unsigned char*)p, 
					  fwp->data_size + sizeof(part_t)));
			if (crc != fwp->signature->crc) {
				WARN("Invalid '%s' CRC (claims: %u, but is %u)\n", 
						fwp->header->name, fwp->signature->crc, crc);
			}
		}

		p = (part_t*)((unsigned char*)p + sizeof(part_t) + 
			      ntohl(p->data_size) + sizeof(part_crc_t));

		/* check bounds */
		if (((unsigned char*)p - base) >= size) {
			return -3;
		}
		++i;
	}
	fw->part_count = i;

	sig = (signature_t*)p;
	if (strncmp(sig->magic, MAGIC_END, MAGIC_LENGTH) != 0) {
		ERROR("Bad firmware signature\n");
		return -4;
	}

	crc = htonl(crc32(0L, base, (unsigned char*)sig - base));
	if (crc != sig->crc) {
		WARN("Invalid signature CRC (claims: %u, but is %u)\n",
		     sig->crc, crc);
	}

	return 0;
}

static int
fw_split(const fw_t* fw, const char* prefix) {
	int i;
	const fw_part_t* fwp;
	FILE* f;
	char filename[PATH_MAX];

	snprintf(filename, sizeof(filename), "%s.txt", prefix);

	INFO("Creating descriptor file:\n\t%s\n", filename);
	/* write descriptor file */
	f = fopen(filename, "w");
	if (f == NULL) {
		ERROR("Couldn't open file '%s' for writing!\n", filename);
		return -1;
	}

	for (i = 0; i < fw->part_count; ++i) {
		fwp = &fw->parts[i];

		fprintf(f, "%s\t\t0x%02X\t0x%08X\t0x%08X\t0x%08X\t0x%08X\t%s.%s\n",
			fwp->header->name, 
			ntohl(fwp->header->index),
			ntohl(fwp->header->baseaddr),
			ntohl(fwp->header->part_size),
			ntohl(fwp->header->memaddr),
			ntohl(fwp->header->entryaddr),
			prefix, fwp->header->name);

	}
	fclose(f);

	INFO("Creating partition data files: \n");

	for (i = 0; i < fw->part_count; ++i) {
		fwp = &fw->parts[i];

		snprintf(filename, sizeof(filename), "%s.%s",
			 prefix, fwp->header->name);
		f = fopen(filename, "w");
		if (f == NULL) {
			ERROR("Failed opening file '%s' for writing: %s\n",
			      filename, strerror(errno));
			continue;
		}
		
		INFO("\t%s\n", filename);

		if (fwrite(fwp->data, fwp->data_size, 1, f) != 1) {
			ERROR("Failed writing to file '%s': %s\n",
			      filename, strerror(errno));
			fclose(f);
			continue;
		}
		fclose(f);
	}

	return 0;
}

static void
usage(const char* progname) {
	INFO("Version %s\n"
             "Usage: %s [options] <firmware file> [<fw file2> ... <fw fileN>]\n"
	     "\t-o <output file prefix>\t"
	     		" - output file prefix, default: firmware version\n"
	     "\t-d\t\t\t - turn debug output on\n"
	     "\t-h\t\t\t - this help\n", VERSION, progname);
}


static int
do_fwsplit(const char* filename) {
	int rc;
	int fd;
	struct stat st;
	fw_t fw;
	unsigned char* addr;

	INFO("Firmware file: '%s'\n", filename);

	rc = stat(filename, &st);
	if (rc) {
		ERROR("Couldn't stat() file '%s': %s\n", 
		      filename, strerror(errno));
		return -2;
	}

	if (st.st_size < sizeof(header_t) + sizeof(signature_t)) {
		ERROR("File '%s' is too short\n", filename);
		return -3;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		ERROR("Couldn't open file '%s': %s\n",
		      filename, strerror(errno));
		return -4;
	}

	addr=(unsigned char*)mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		ERROR("Failed mmaping memory for file '%s'\n", filename);
		close(fd);
		return -5;
	}

	// parse & validate fw
	rc = fw_parse(addr, st.st_size, &fw);
	if (rc) {
		ERROR("Invalid firmware file '%s'!\n", filename);
		munmap(addr, st.st_size);
		close(fd);
		return -6;
	}

	if (strlen(prefix) == 0) {
		strncpy(prefix, fw.version, sizeof(prefix));
	}
	fw_split(&fw, prefix);
	
	munmap(addr, st.st_size);
	close(fd);

	return 0;
}

int
main(int argc, char* argv[]) {
	int o, i;

	memset(prefix, 0, sizeof(prefix));

	while ((o = getopt(argc, argv, "hdo:")) != -1) {
		switch (o) {
		case 'd':
			debug++;
			break;
		case 'o':
			if (optarg) {
				strncpy(prefix, optarg, sizeof(prefix));
			}
			break;
		case 'h':
			usage(argv[0]);
			return -1;
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
		return -1;
	}

	if (strlen(prefix) != 0 && (optind + 1) < argc) {
		WARN("Prefix overridden - will process only the first firmware file\n");
		do_fwsplit(argv[optind]);
	} else {
		for (i = optind; i < argc; ++i) {
			do_fwsplit(argv[i]);
		}
	}

	return 0;
}

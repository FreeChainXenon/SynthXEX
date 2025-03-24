// This file is part of SynthXEX, one component of the
// FreeChainXenon development toolchain
//
// Copyright (c) 2024-25 Aiden Isik
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <arpa/inet.h> // For endian-swapping
#include "common.h"

// Endian test
#if 'AB' == 0b100000101000010
#define LITTLE_ENDIAN_SYSTEM
#else
#define BIG_ENDIAN_SYSTEM
#endif

// Preprocessor definitions
// For simplicity, only flags supported by SynthXEX are defined here

// Image flags
#define XEX_IMG_FLAG_4KIB_PAGES  0x10000000
#define XEX_IMG_FLAG_REGION_FREE 0x20000000

// Module flags
#define XEX_MOD_FLAG_TITLE 0x00000001

// Region flags
#define XEX_REG_FLAG_REGION_FREE 0xFFFFFFFF

// Optional header IDs
#define XEX_OPT_ID_BASEFILE_FORMAT 0x003FF
#define XEX_OPT_ID_ENTRYPOINT      0x10100
#define XEX_OPT_ID_IMPORT_LIBS     0x103FF
#define XEX_OPT_ID_TLS_INFO        0x20104
#define XEX_OPT_ID_SYS_FLAGS       0x30000

// System flags  
#define XEX_SYS_GAMEPAD_DISCONNECT 0x00000020
#define XEX_SYS_INSECURE_SOCKETS   0x00000040
#define XEX_SYS_XAM_HOOKS          0x00001000
#define XEX_SYS_BACKGROUND_DL      0x00080000
#define XEX_SYS_ALLOW_CONTROL_SWAP 0x40000000

// RWX flags (PE)
#define PE_SECTION_FLAG_DISCARDABLE 0x02000000
#define PE_SECTION_FLAG_EXECUTE     0x20000000
#define PE_SECTION_FLAG_READ        0x40000000
#define PE_SECTION_FLAG_WRITE       0x80000000

// RWX flags (ELF)
#define ELF_SECTION_FLAG_WRITE      0x1
#define ELF_SECTION_FLAG_ALLOC      0x2
#define ELF_SECTION_FLAG_EXECUTABLE 0x4

// RWX flags (XEX)
#define XEX_SECTION_CODE   0x1
#define XEX_SECTION_RWDATA 0x2
#define XEX_SECTION_RODATA 0x3

// Page RWX flags
struct sections
{
  uint32_t count;
  struct sectionPerms *sectionPerms;
};

struct sectionPerms
{
  uint8_t permFlag;
  uint32_t rva;
};

// Data structs
struct peData
{
  uint32_t size;
  uint32_t baseAddr;
  uint32_t entryPoint;
  uint32_t tlsAddr;
  uint32_t tlsSize;
  uint32_t peHeaderOffset;
  uint16_t numberOfSections;
  uint32_t sectionTableSize;
  uint32_t headerSize;
  uint32_t pageSize;
  struct sections sections;
};

// Most of these are 8-byte aligned. Basefile is 4KiB aligned.
// In order of appearance
struct offsets
{
  uint32_t xexHeader;
  uint32_t optHeaderEntries;
  uint32_t secInfoHeader;
  uint32_t *optHeaders;
  uint32_t basefile;
};

struct __attribute__((packed)) xexHeader
{
  char magic[4];
  uint32_t moduleFlags;
  uint32_t peOffset;
  uint32_t reserved;
  uint32_t secInfoOffset;
  uint32_t optHeaderCount;
};

struct __attribute__((packed)) pageDescriptor
{
  uint32_t sizeAndInfo; // First 28 bits == size, last 4 == info (RO/RW/X)
  uint8_t sha1[0x14];
};

struct __attribute__((packed)) secInfoHeader
{
  uint32_t headerSize;
  uint32_t peSize;
  // - IMAGE INFO -
  uint8_t signature[0x100];
  uint32_t imageInfoSize;
  uint32_t imageFlags;
  uint32_t baseAddr;
  uint8_t imageSha1[0x14];
  uint32_t importTableCount;
  uint8_t importTableSha1[0x14];
  uint8_t mediaID[0x10];
  uint8_t aesKey[0x10];
  uint32_t exportTableAddr;
  uint8_t headersHash[0x14];
  uint32_t gameRegion;
  // - IMAGE INFO -
  uint32_t mediaTypes;
  uint32_t pageDescCount;
  struct pageDescriptor *descriptors;
};

struct __attribute__((packed)) basefileFormat
{
  uint32_t size;
  uint16_t encType;
  uint16_t compType;
  uint32_t dataSize;
  uint32_t zeroSize;
};

struct __attribute__((packed)) importLibraries
{
  uint32_t size;
  uint8_t *data;
};

struct __attribute__((packed)) tlsInfo
{
  uint32_t slotCount;
  uint32_t rawDataAddr;
  uint32_t dataSize;
  uint32_t rawDataSize;
};

struct optHeaderEntries
{
  uint32_t count;
  struct optHeaderEntry *optHeaderEntry;
};

struct __attribute__((packed)) optHeaderEntry
{
  uint32_t id;
  uint32_t dataOrOffset;
};

struct optHeaders
{
  struct basefileFormat basefileFormat;
  struct importLibraries importLibraries;
  struct tlsInfo tlsInfo;
};

// Functions used for file data manipulation

uint32_t getNextAligned(uint32_t offset, uint32_t alignment);

// TODO: combine these into a single function
uint32_t get32BitFromPE(FILE *pe);
uint16_t get16BitFromPE(FILE *pe);
uint32_t get32BitFromXEX(FILE *xex);
uint16_t get16BitFromXEX(FILE *xex);

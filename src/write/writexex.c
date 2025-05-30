// This file is part of SynthXEX, one component of the
// FreeChainXenon development toolchain
//
// Copyright (c) 2024 Aiden Isik
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

#include "writexex.h"

// TEMPORARY WRITE TESTING
int writeXEX(struct xexHeader *xexHeader, struct optHeaderEntries *optHeaderEntries, struct secInfoHeader *secInfoHeader, struct optHeaders *optHeaders, struct offsets *offsets, FILE *pe, FILE *xex)
{
  // XEX Header
#ifdef LITTLE_ENDIAN_SYSTEM
  // Endian-swap XEX header before writing
  xexHeader->moduleFlags = __builtin_bswap32(xexHeader->moduleFlags);
  xexHeader->peOffset = __builtin_bswap32(xexHeader->peOffset);
  xexHeader->secInfoOffset = __builtin_bswap32(xexHeader->secInfoOffset);
  xexHeader->optHeaderCount = __builtin_bswap32(xexHeader->optHeaderCount);
#endif
  
  fseek(xex, offsets->xexHeader, SEEK_SET);
  fwrite(xexHeader, sizeof(uint8_t), sizeof(struct xexHeader), xex);

  // Optional header entries
#ifdef LITTLE_ENDIAN_SYSTEM
  // Endian swap opt header entries
  for(uint32_t i = 0; i < optHeaderEntries->count; i++)
    {
      optHeaderEntries->optHeaderEntry[i].id = __builtin_bswap32(optHeaderEntries->optHeaderEntry[i].id);
      optHeaderEntries->optHeaderEntry[i].dataOrOffset = __builtin_bswap32(optHeaderEntries->optHeaderEntry[i].dataOrOffset);
    }
#endif

  fseek(xex, offsets->optHeaderEntries, SEEK_SET);

  for(int i = 0; i < optHeaderEntries->count; i++)
    {
      fwrite(&(optHeaderEntries->optHeaderEntry[i]), sizeof(uint8_t), sizeof(struct optHeaderEntry), xex);
    }

  free(optHeaderEntries->optHeaderEntry); // Alloc'd in setdata (optheaders). Now we're done with it.
  
  // Page descriptors
  fseek(xex, offsets->secInfoHeader + sizeof(struct secInfoHeader) - sizeof(void*), SEEK_SET);
  
  for(int i = 0; i < secInfoHeader->pageDescCount; i++)
    {
#ifdef LITTLE_ENDIAN_SYSTEM
      secInfoHeader->descriptors[i].sizeAndInfo = __builtin_bswap32(secInfoHeader->descriptors[i].sizeAndInfo);
#endif
      
      // Writing out current descriptor...
      fwrite(&(secInfoHeader->descriptors[i].sizeAndInfo), sizeof(uint32_t), 0x1, xex);
      fwrite(secInfoHeader->descriptors[i].sha1, sizeof(uint8_t), 0x14, xex);
    }

  free(secInfoHeader->descriptors); // calloc'd elsewhere, freeing now

  // Basefile
  fseek(pe, 0x0, SEEK_SET);
  fseek(xex, offsets->basefile, SEEK_SET);
  
  uint16_t readBufSize = 0x1000; // Reading in 4KiB at a time to avoid excessive memory usage
  uint8_t *buffer = malloc(readBufSize * sizeof(uint8_t));
  if(buffer == NULL) {return ERR_OUT_OF_MEM;}
  
  for(uint32_t i = 0; i < secInfoHeader->peSize; i += readBufSize)
    {
      fread(buffer, sizeof(uint8_t), readBufSize, pe);
      fwrite(buffer, sizeof(uint8_t), readBufSize, xex);
    }

  free(buffer);
  
  // Security Info
#ifdef LITTLE_ENDIAN_SYSTEM
  // Endian-swap secinfo header
  secInfoHeader->headerSize = __builtin_bswap32(secInfoHeader->headerSize);
  secInfoHeader->peSize = __builtin_bswap32(secInfoHeader->peSize);
  secInfoHeader->imageInfoSize = __builtin_bswap32(secInfoHeader->imageInfoSize);
  secInfoHeader->imageFlags = __builtin_bswap32(secInfoHeader->imageFlags);
  secInfoHeader->baseAddr = __builtin_bswap32(secInfoHeader->baseAddr);
  secInfoHeader->importTableCount = __builtin_bswap32(secInfoHeader->importTableCount);
  secInfoHeader->exportTableAddr = __builtin_bswap32(secInfoHeader->exportTableAddr);
  secInfoHeader->gameRegion = __builtin_bswap32(secInfoHeader->gameRegion);
  secInfoHeader->mediaTypes = __builtin_bswap32(secInfoHeader->mediaTypes);
  secInfoHeader->pageDescCount = __builtin_bswap32(secInfoHeader->pageDescCount);
#endif

  fseek(xex, offsets->secInfoHeader, SEEK_SET);
  fwrite(secInfoHeader, sizeof(uint8_t), sizeof(struct secInfoHeader) - sizeof(void*), xex); // sizeof(void*) == size of page descriptor pointer at end
  
  // Optional headers
  uint32_t currentHeader = 0;
  
  if(optHeaders->basefileFormat.size != 0) // If not 0, it has data. Write it.
    {
      fseek(xex, offsets->optHeaders[currentHeader], SEEK_SET);

#ifdef LITTLE_ENDIAN_SYSTEM
      optHeaders->basefileFormat.size = __builtin_bswap32(optHeaders->basefileFormat.size);
      optHeaders->basefileFormat.encType = __builtin_bswap16(optHeaders->basefileFormat.encType);
      optHeaders->basefileFormat.compType = __builtin_bswap16(optHeaders->basefileFormat.compType);
      optHeaders->basefileFormat.dataSize = __builtin_bswap32(optHeaders->basefileFormat.dataSize);
      optHeaders->basefileFormat.zeroSize = __builtin_bswap32(optHeaders->basefileFormat.zeroSize);
#endif
      
      fwrite(&(optHeaders->basefileFormat), sizeof(uint8_t), sizeof(struct basefileFormat), xex);
      currentHeader++;
    }

  if(optHeaders->importLibraries.size != 0)
    {
      fseek(xex, offsets->optHeaders[currentHeader], SEEK_SET);
      uint32_t importLibsSize = optHeaders->importLibraries.size; // Need to use this value, so save it before we endian-swap
      
#ifdef LITTLE_ENDIAN_SYSTEM
      optHeaders->importLibraries.size = __builtin_bswap32(optHeaders->importLibraries.size);
#endif

      fwrite(&(optHeaders->importLibraries.size), sizeof(uint8_t), 0x4, xex);
      fwrite(optHeaders->importLibraries.data, sizeof(uint8_t), importLibsSize - 0x4, xex);
      currentHeader++;
    }

  if(optHeaders->tlsInfo.slotCount != 0)
    {
      fseek(xex, offsets->optHeaders[currentHeader], SEEK_SET);

#ifdef LITTLE_ENDIAN_SYSTEM
      optHeaders->tlsInfo.slotCount = __builtin_bswap32(optHeaders->tlsInfo.slotCount);
      optHeaders->tlsInfo.rawDataAddr = __builtin_bswap32(optHeaders->tlsInfo.rawDataAddr);
      optHeaders->tlsInfo.dataSize = __builtin_bswap32(optHeaders->tlsInfo.dataSize);
      optHeaders->tlsInfo.rawDataSize = __builtin_bswap32(optHeaders->tlsInfo.rawDataSize);
#endif

      fwrite(&(optHeaders->tlsInfo), sizeof(uint8_t), sizeof(struct tlsInfo), xex);
    }
  
  free(offsets->optHeaders); // Alloc'd in placer.
  return SUCCESS;
}

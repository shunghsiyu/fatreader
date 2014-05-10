//
//  main.h
//  fatreader
//
//  Created by Shung-Hsi Yu on 5/5/14.
//  Copyright (c) 2014 opersys. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <wchar.h>

#define BUFFER_SIZE (256)
#define FILEPATH "/Users/shunghsiyu/Xcode/fatreader/fatreader/fat16.fs"
#define OFFSET_BYTEPERBLOCK (0x0b)
#define OFFSET_SIZE_BYTEPERBLOCK (2)
#define OFFSET_FATNUM (0x10)
#define OFFSET_SIZE_FATNUM (1)
#define OFFSET_ROOTENTRIES (0x11)
#define OFFSET_SIZE_ROOTENTRIES (2)
#define OFFSET_FATSIZE (0x16)
#define OFFSET_SIZE_FATSIZE (2)
#define OFFSET_NUMRESERVEBLOCK (0x0e)
#define OFFSET_SIZE_NUMRESERVEBLOCK (2)
#define OFFSET_ATTRI_FROM_DOSNAME (0x0b)

#define FAT16_ENTRY_SIZE (0x20)
#define DOSNAME_LENGTH (8)
#define DOSEXT_LENGTH (3)
#define DOSWHOLENAME_LENGTH (12)
#define ORDINAL_MIN (0x41)
#define ORDINAL_MAX (0x54)
#define LFN_LENGTH1 (10)
#define OFFSET_LFN1FROM2 (3)
#define LFN_LENGTH2 (12)
#define OFFSET_LFN2FROM3 (2)
#define LFN_LENGTH3 (2)
#define LFN_ENTRY_SIZE (24)

#define FAT_ATTRI_DIRECTORY (0x10)

#ifndef fatreader_main_h
#define fatreader_main_h

int main(int argc, const char * argv[]);
int read_lfn_bytes(const uint8_t * src, int offset, size_t num_byte, wchar_t * dst, size_t dst_size);
int read_le_bytes_int(const uint8_t * src, int offset, size_t num_bytes, int32_t * dst);
int read_doswholename(const uint8_t * src, int offset, char * name, size_t name_size);
int read_dosname(const uint8_t * src, int offset, char * name, size_t name_size);
int read_dosext(const uint8_t * src, int offset, char * ext, size_t ext_size);
int read_lfn_whole_entry(const uint8_t * src, int offset, wchar_t * lfn, size_t lfn_size);
int read_lfn_single_entry(const uint8_t * src, int offset, wchar_t * lfn, size_t lfn_size);
int get_ordinal(const uint8_t * src, int offset);
int get_entry_size(const uint8_t * src, int offset);
int get_dosname_offset(const uint8_t * src, int offset);
int is_directory(const uint8_t * src, int offset);
int trim(char * str, size_t str_size);
#endif
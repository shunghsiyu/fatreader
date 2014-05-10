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
#define FILEPATH "/home/yus/NetBeansProjects/fatreader/fat16.f"
#define OFFSET_BYTEPERBLOCK (0x0b)
#define OFFSET_SIZE_BYTEPERBLOCK (2)
#define OFFSET_FATNUM (0x10)
#define OFFSET_SIZE_FATNUM (1)
#define OFFSET_FATSIZE (0x16)
#define OFFSET_SIZE_FATSIZE (2)
#define OFFSET_NUMRESERVEBLOCK (0x0e)
#define OFFSET_SIZE_NUMRESERVEBLOCK (2)
#define OFFSET_ATTRI_FROM_DOSNAME (0x0b)

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
int get_bytes(const unsigned char * src, int offset, int size);
int read_le_bytes(const unsigned char * src, int offset, int num_byte, int * dst, int dst_size);
int read_doswholename(const unsigned char * src, int offset, char * name, int name_size);
int read_dosname(const unsigned char * src, int offset, char * name, int name_size);
int read_dosext(const unsigned char * src, int offset, char * ext, int ext_size);
int read_lfn_entry(const unsigned char * src, int offset, int * lfn, int lfn_size);
int get_ordinal(const unsigned char * src, int offset);
int get_entry_size(const unsigned char * src, int offset);
int get_dosname_offset(const unsigned char * src, int offset);
int is_directory(const unsigned char * src, int offset);
int trim(char * str, int str_size);
#endif

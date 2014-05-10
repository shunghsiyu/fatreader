//
//  main.c
//  fatreader
//
//  Created by Shung-Hsi Yu on 5/5/14.
//  Copyright (c) 2014 opersys. All rights reserved.
//

#include "main.h"

int main(int argc, const char * argv[])
{
    char * tab = "    ";
 
    char filepath[BUFFER_SIZE];
    if (argv[1] == 0) {
        strcpy(filepath, FILEPATH);
    } else {
        strcpy(filepath, argv[1]);
    }
    
    int fdin;
    fdin = -1;
    while (fdin < 0) {
        fdin = open(filepath, O_RDONLY);
        if (fdin < 0) {
            perror("Failed to read file");
            printf("Please type the complete path to file: ");
            fflush(stdout);
            scanf("%s", filepath);
        }
    }
    

    printf("Opened file %s\n", filepath);
    printf("\n");
          
    struct stat fstatbuf;
    int fstat_status;
    fstat_status = fstat(fdin, &fstatbuf);
    if (fstat_status < 0) {
        perror("fstat failed");
        exit(EXIT_FAILURE);
    }
    
    uint8_t * mmap_file;
    mmap_file = mmap(NULL, fstatbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0);
    if (mmap_file == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Information about the partition:\n");
    
    int num_byteperblock;
    num_byteperblock = get_bytes(mmap_file, OFFSET_BYTEPERBLOCK, OFFSET_SIZE_BYTEPERBLOCK);
    printf("%s%d bytes per block\n", tab,num_byteperblock);
    
    int num_reservedblock;
    num_reservedblock = get_bytes(mmap_file, OFFSET_NUMRESERVEBLOCK, OFFSET_SIZE_NUMRESERVEBLOCK);
    printf("%s%d blocks reserved\n", tab, num_reservedblock);
    
    int num_fat;
    num_fat = get_bytes(mmap_file, OFFSET_FATNUM, OFFSET_SIZE_FATNUM);
    printf("%s%d file allocation tables (FAT)\n", tab, num_fat);
    
    int fat_block_size;
    fat_block_size = get_bytes(mmap_file, OFFSET_FATSIZE, OFFSET_SIZE_FATSIZE);
    printf("%s%d blocks per FAT\n", tab, fat_block_size);
    printf("\n");
    
    int root_block;
    root_block = num_reservedblock + num_fat*fat_block_size;
    printf("The root directory at block %d contains:\n", root_block);
    
    int root_byte;
    root_byte = root_block*num_byteperblock;
    
    int num_entry;
    num_entry = 0;
    char wholename[DOSWHOLENAME_LENGTH+1];
    int lfn[LFN_ENTRY_SIZE/2];
    int offset;
    offset = root_byte;
    while (get_ordinal(mmap_file, offset) >= 0) {
        printf("%sEntry %d:\n", tab, num_entry+1);
        printf("%s%sType: ", tab, tab);
        if (is_directory(mmap_file, offset)) {
            printf("directory\n");
        } else {
            printf("file\n");
        }
        read_doswholename(mmap_file, offset, wholename, DOSWHOLENAME_LENGTH);
        printf("%s%sDOS Name: %s\n", tab, tab, wholename);
        read_lfn_entry(mmap_file, offset, lfn, LFN_ENTRY_SIZE);
        freopen(NULL, "w", stdout);
        wprintf(L"%s%sLong Name: %ls\n", tab, tab, lfn);
        freopen(NULL, "w", stdout);
        offset = offset + get_entry_size(mmap_file, offset);
        num_entry = num_entry + 1;
    }
    
    int unmap_status;
    unmap_status = munmap(mmap_file, fstatbuf.st_size);
    if (unmap_status < 0) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
    
    close(fdin);
    
    exit(EXIT_SUCCESS);
}

int get_bytes(const uint8_t * src, int offset, size_t size) {
    if (size > 4) {
        perror("read_bytes size error");
        exit(EXIT_FAILURE);
    }
    int value;
    value = 0;
    
    int i;
    for (i = (int) size-1; i >= 0; i--) {
        value = (value << 8) + src[offset+i];
    }
    
    return value;
}

int read_le_bytes(const uint8_t * src, int offset, size_t num_byte, int * dst, size_t dst_size) {
    if ((num_byte % 2) > 0) {
        perror("number of bytes being read should be multiple of 2");
        return -1;
    }
    
    int pos;
    pos = 0;    int i;
    for (i = 0; i < num_byte; i = i + 2) {
        if ((pos+2) > dst_size*2) {
            perror("dst's length is too short");
            return -1;
        }
        dst[i/2] = (src[offset+i+1] << 8) + src[offset+i];
    }
    
    return 1;
}

int read_doswholename(const uint8_t * src, int offset, char * name, size_t name_size) {
    int pos;
    pos = 0;
    
    if (name_size < DOSNAME_LENGTH + DOSEXT_LENGTH + 1) {
        perror("name_size should be greater than 8+3");
        return -1;
    }
    
    char dosname[DOSNAME_LENGTH+1];
    read_dosname(src, offset, dosname, DOSNAME_LENGTH);
    strncpy(name, dosname, DOSNAME_LENGTH);
    pos = pos + DOSNAME_LENGTH;
    
    if (is_directory(src, offset) > 0) {
        int i;
        for (i = pos; i < name_size; i++) {
            name[i] = '\0';
        }
    } else {
        char dosext[DOSEXT_LENGTH+1];
        read_dosext(src, offset, dosext, DOSEXT_LENGTH);
        strncat(name, ".", 1);
        strncat(name, dosext, DOSEXT_LENGTH);
    }
    
    return 1;
}

int read_dosname(const uint8_t * src, int offset, char * name, size_t name_size) {
    int new_offset;
    new_offset = get_dosname_offset(src, offset);
    
    const char * dos_name = (const char *) &src[new_offset];
    strncpy(name, (const char *) dos_name, name_size);
    
    trim(name, name_size);
    
    return 1;
}

int read_dosext(const uint8_t * src, int offset, char * ext, size_t ext_size) {
    int new_offset;
    new_offset = get_dosname_offset(src, offset) + DOSNAME_LENGTH;
    
    const char * dos_ext = (const char *) &src[new_offset];
    strncpy(ext, (const char *) dos_ext, ext_size);
    
    trim(ext, ext_size);
    
    return 1;
}

int read_lfn_entry(const uint8_t * src, int offset, int * lfn, size_t lfn_size) {
    if (lfn_size < LFN_LENGTH1 + LFN_LENGTH2 + LFN_LENGTH3) {
        perror("lfn's length is too short");
        return -1;
    }
    
    int new_offset;
    new_offset = offset + 1;
    
    int len;
    len = 0;
    
    read_le_bytes(src, new_offset, LFN_LENGTH1, lfn+len, lfn_size-len);
    len = len + LFN_LENGTH1/2;
    new_offset = new_offset + LFN_LENGTH1 + OFFSET_LFN1FROM2;
    read_le_bytes(src, new_offset, LFN_LENGTH2, lfn+len, lfn_size-len);
    len = len + LFN_LENGTH2/2;
    new_offset = new_offset + LFN_LENGTH2 + OFFSET_LFN2FROM3;
    read_le_bytes(src, new_offset, LFN_LENGTH3, lfn+len, lfn_size-len);
    len = len + LFN_LENGTH3/2;
    
    int i;
    for (i = (int) lfn_size-1; i > len-1 && i >= 0; i--) {
        lfn[i] = 0;
    }
    
    return 1;
}

int get_ordinal(const uint8_t * src, int offset) {
    int ordinal = src[offset];
    if (ordinal > ORDINAL_MAX || ordinal < ORDINAL_MIN) {
        return -1;
    } else {
        return ordinal;
    }
}

int get_entry_size(const uint8_t * src, int offset) {
    int size;
    size = (get_ordinal(src, offset) - ORDINAL_MIN + 1) * 2 * 0x10 + 0x20;
    return size;
}

int get_dosname_offset(const uint8_t * src, int offset) {
    int ordinal;
    ordinal = get_ordinal(src, offset);
    
    int i;
    int new_offset;
    new_offset = offset;
    for (i = 0; i < ordinal-ORDINAL_MIN; i++) {
        new_offset = new_offset + 0x20;
    }
    new_offset = new_offset + 0x20;
    
    return new_offset;
}

int is_directory(const uint8_t * src, int offset) {
    int new_offset;
    new_offset = get_dosname_offset(src, offset) + OFFSET_ATTRI_FROM_DOSNAME;
    
    uint8_t attri_block;
    attri_block = src[new_offset];
    
    uint8_t directory_bit;
    directory_bit = attri_block & FAT_ATTRI_DIRECTORY;
    if (directory_bit > 0) {
        return 1;
    } else {
        return 0;
    }
}

int trim(char * str, size_t str_size) {
    
    int i;
    for (i = (int) str_size-1; i >= 0; i--) {
        if (str[i] == ' ') {
            str[i] = '\0';
        } else {
            break;
        }
    }
    
    return 0;
}
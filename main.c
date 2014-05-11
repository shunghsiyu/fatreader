//
//  main.c
//  fatreader
//
//  Created by Shung-Hsi Yu on 5/5/14.
//  Copyright (c) 2014 opersys. All rights reserved.
//

#include "main.h"

const char TAB[5] = "    ";

int main(int argc, const char * argv[])
{
 
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
    

    printf("Opened file '%s'\n", filepath);
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
    read_le_bytes_int(mmap_file, OFFSET_BYTEPERBLOCK, OFFSET_SIZE_BYTEPERBLOCK, &num_byteperblock);
    printf("%s%d bytes per block\n", TAB, num_byteperblock);
    
    int num_blockpercluster;
    read_le_bytes_int(mmap_file, OFFSET_BLOCKPERCLUSTER, OFFSET_SIZE_BLOCKPERCLUSTER, &num_blockpercluster);
    printf("%s%d blocks per cluster\n", TAB, num_blockpercluster);
    
    int num_reservedblock;
    read_le_bytes_int(mmap_file, OFFSET_NUMRESERVEBLOCK, OFFSET_SIZE_NUMRESERVEBLOCK, &num_reservedblock);
    printf("%s%d blocks reserved\n", TAB, num_reservedblock);
    
    int num_fat;
    read_le_bytes_int(mmap_file, OFFSET_FATNUM, OFFSET_SIZE_FATNUM, &num_fat);
    printf("%s%d file allocation tables (FAT)\n", TAB, num_fat);
    
    int fat_block_size;
    read_le_bytes_int(mmap_file, OFFSET_FATSIZE, OFFSET_SIZE_FATSIZE, &fat_block_size);
    printf("%s%d blocks per FAT\n", TAB, fat_block_size);
    printf("\n");
    
    int num_root_entries;
    read_le_bytes_int(mmap_file, OFFSET_ROOTENTRIES, OFFSET_SIZE_ROOTENTRIES, &num_root_entries);
    
    int root_block;
    root_block = num_reservedblock + num_fat*fat_block_size;
    printf("The root directory at block %d can hold up to %d entries, currently contains:\n", root_block, num_root_entries);
    
    int root_offset;
    root_offset = root_block*num_byteperblock;
    
    int num_entry;
    num_entry = 0;
    char wholename[DOSWHOLENAME_LENGTH+1];
    wchar_t lfn[LFN_ENTRY_SIZE/2];
    int offset;
    offset = root_offset;
    while (get_ordinal(mmap_file, offset) >= 0) {
        printf("%sEntry %d:\n", TAB, num_entry+1);
        printf("%s%sType: ", TAB, TAB);
        if (is_directory(mmap_file, offset)) {
            printf("directory\n");
        } else {
            printf("file\n");
        }
        read_doswholename(mmap_file, offset, wholename, DOSWHOLENAME_LENGTH);
        printf("%s%sDOS Name: %s\n", TAB, TAB, wholename);

        read_lfn_whole_entry(mmap_file, offset, lfn, LFN_ENTRY_SIZE/2);
        freopen(NULL, "w", stdout);
        wprintf(L"%s%sLong Name: %ls\n", TAB, TAB, lfn);
        freopen(NULL, "w", stdout);
        offset = offset + get_entry_size(mmap_file, offset);
        num_entry = num_entry + 1;
    }
    printf("\n");
    
    int data_offset;
    data_offset = root_offset + num_root_entries*FAT16_ENTRY_SIZE;
    printf("The data section start at byte %d\n", data_offset);
    
    int file_offset;
    file_offset = -1;
    char file_name[BUFFER_SIZE];
    int name_offset;
    name_offset = 0;
    if (argv[2] == 0) {
        
    } else {
        printf("%sLooking for '%s'\n", TAB, argv[2]);
        char extractpath[BUFFER_SIZE];
        strcpy(extractpath, argv[2]);
        offset = root_offset;
        char * name_to_search = strtok(extractpath, "/");
        while (name_to_search) {
            file_offset = -1;
            name_offset = find_entry_in_directory(mmap_file, offset, num_byteperblock, name_to_search);
            if (name_offset < 0) {
                break;
            } else {
                offset = get_entry_location(mmap_file, name_offset, data_offset, num_byteperblock*num_blockpercluster);
                if (!is_directory(mmap_file, offset)) {
                    file_offset = offset;
                    strncpy(file_name, name_to_search, BUFFER_SIZE);
                } else {
                    file_offset = 0;
                }
                name_to_search = strtok(NULL, "/");
            }
        }
        
        int fdout;
        if (argv[2] != 0 && file_offset > 0) {
            printf("%sFile is found at byte offset %d\n", TAB, file_offset);
            fdout = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (fdout < 0) {
                perror("can't open file to write");
                exit(EXIT_FAILURE);
            }
            
            off_t lseek_stat;
            int file_size;
            file_size = get_entry_file_size(mmap_file, name_offset);
            lseek_stat = lseek(fdout, file_size-1, SEEK_SET);
            if (lseek_stat < 0) {
                perror("lseek error");
                exit(EXIT_FAILURE);
            }
            
            int write_stat;
            write_stat = (int) write(fdout, "", 1);
            if (write_stat != 1) {
                perror("write error");
                exit(EXIT_FAILURE);
            }
            
            uint8_t * dst;
            dst = mmap(NULL, file_size-1, PROT_READ | PROT_WRITE, MAP_SHARED, fdout, 0);
            if (dst == MAP_FAILED) {
                perror("mmap dst failed");
                exit(EXIT_FAILURE);
            }
            
            int num_cluster;
            num_cluster = get_cluster_num(file_offset, data_offset, num_byteperblock*num_blockpercluster);
            
            copy_data(mmap_file, num_reservedblock*num_byteperblock, data_offset, num_cluster, file_size, dst, file_size, num_byteperblock*num_blockpercluster);
            printf("\nFinished copying '%s' from '%s'\n", file_name, filepath);
            
            int unmap_status;
            unmap_status = munmap(dst, file_size);
            if (unmap_status < 0) {
                perror("munmap failed");
                exit(EXIT_FAILURE);
            }
            
            close(fdout);
        } else if (file_offset == 0) {
            printf("%sError: '%s' is a directory\n", TAB, argv[2]);
        } else {
            printf("%sError: '%s' is not found\n", TAB, argv[2]);
        }
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
                       
int find_entry_in_directory(const uint8_t * src, int offset, size_t directory_size, const char * name) {
    int new_offset;
    new_offset = offset;
    int current_size;
    current_size = new_offset - offset;
    char wholename[DOSWHOLENAME_LENGTH];
    wchar_t lfn[LFN_ENTRY_SIZE/2];
    char lfn_char[LFN_ENTRY_SIZE/2];
    if (src[new_offset] == 0x2e && src[new_offset+FAT16_ENTRY_SIZE] == 0x2e && src[new_offset+FAT16_ENTRY_SIZE+1] == 0x2e) {
        new_offset = new_offset + FAT16_ENTRY_SIZE*2;
    }
    while (get_ordinal(src, new_offset) >= 0 && (current_size < directory_size)) {
        read_doswholename(src, new_offset, wholename, DOSWHOLENAME_LENGTH);
        read_lfn_whole_entry(src, new_offset, lfn, LFN_ENTRY_SIZE/2);
        wcstombs(lfn_char, lfn, LFN_ENTRY_SIZE/2);
        if (strcmp(name, wholename) == 0 || strcmp(name, lfn_char) == 0) {
            return new_offset;
        }
        new_offset = new_offset + get_entry_size(src, new_offset);
        current_size = new_offset - offset;
    }
    return -1;
}

int get_entry_location(const uint8_t * src, int offset, int data_offset, int cluster_size) {
    int new_offset;
    new_offset = get_dosname_offset(src, offset) + OFFSET_LOCATION_FROM_DOSNAME;
    
    int location;
    read_le_bytes_int(src, new_offset, OFFSET_SIZE_LOCATION, &location);
    
    int file_offset;
    file_offset = (location-2)*cluster_size + data_offset;
    
    return file_offset;
}

int get_entry_file_size(const uint8_t * src, int offset) {
    int size;
    int length_offset;
    length_offset = get_dosname_offset(src, offset) + OFFSET_LENGTH_FROM_DOSNAME;
    read_le_bytes_int(src, length_offset, OFFSET_SIZE_LENGTH, &size);
    return size;
}

int get_cluster_num(int offset, int data_offset, int cluster_size) {
    int new_offset;
    new_offset = (offset - data_offset);
    if (new_offset < 0) {
        return -1;
    }
    int num;
    num = new_offset/cluster_size;
    return num+2;
}

int copy_data(const uint8_t * src, int fat_offset, int data_offset, int start_cluster, int size, uint8_t * dst, int dst_size, int cluster_size) {
    int dst_pos;
    dst_pos = 0;
    int new_size;
    new_size = size;
    int offset;
    offset = (start_cluster-2)*cluster_size + data_offset;
    int cluster;
    cluster = start_cluster;
    while (new_size > cluster_size) {
        memcpy(&dst[dst_pos], &src[offset], cluster_size);
        dst_pos = dst_pos + cluster_size;
        read_le_bytes_int(src, fat_offset+cluster*2, 2, &cluster);
        offset = (cluster-2)*cluster_size + data_offset;
        new_size = new_size - cluster_size;
        if (dst_pos > dst_size) {
            perror("dst cannot hold the data");
        }
    }
    memcpy(&dst[dst_pos], &src[offset], new_size);
    dst_pos = dst_pos + new_size;
    return 0;
}

int read_lfn_bytes(const uint8_t * src, int offset, size_t num_byte, wchar_t * dst, size_t dst_size) {
    if ((num_byte % 2) > 0) {
        perror("number of bytes being read should be multiple of 2");
        return -1;
    }
    
    int pos;
    pos = 0;
    int i;
    for (i = 0; i < num_byte; i = i + 2) {
        if ((pos+2) > dst_size*2) {
            perror("dst's length is too short");
            return -1;
        }
        read_le_bytes_int(src, offset+i, 2, (int *) &dst[i/2]);
    }
    
    return 1;
}

int read_le_bytes_int(const uint8_t * src, int offset, size_t num_bytes, int32_t * dst) {
    if (num_bytes > 4) {
        perror("num_bytes can be greater than 4");
        return -1;
    }
    
    long i;
    *dst = 0;
    for (i = num_bytes-1; i >= 0; i--) {
        *dst = (*dst << 8) + src[offset+i];
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

int read_lfn_whole_entry(const uint8_t * src, int offset, wchar_t * lfn, size_t lfn_size) {
    size_t entry_num;
    entry_num = get_ordinal(src, offset) - ORDINAL_MIN + 1;
    
    long i;
    int entry_offset;
    int pos;
    pos = 0;
    int status;
    status = 0;
    for (i = entry_num-1; i >= 0 && status >=0; i--) {
        entry_offset = (int) i * FAT16_ENTRY_SIZE;
        status = read_lfn_single_entry(src, offset+entry_offset, lfn+pos, lfn_size-pos);
        pos = pos + LFN_ENTRY_SIZE/2;
    }
    
    for (i = lfn_size-1; i > pos-1 && i >= 0; i--) {
        lfn[i] = 0;
    }
    
    return 0;
}

int read_lfn_single_entry(const uint8_t * src, int offset, wchar_t * lfn, size_t lfn_size) {
    if (lfn_size*2 < LFN_LENGTH1 + LFN_LENGTH2 + LFN_LENGTH3) {
        perror("lfn's length is too short");
        return -1;
    }
    
    int new_offset;
    new_offset = offset + 1;
    
    int len;
    len = 0;
    
    read_lfn_bytes(src, new_offset, LFN_LENGTH1, lfn+len, lfn_size-len);
    len = len + LFN_LENGTH1/2;
    new_offset = new_offset + LFN_LENGTH1 + OFFSET_LFN1FROM2;
    read_lfn_bytes(src, new_offset, LFN_LENGTH2, lfn+len, lfn_size-len);
    len = len + LFN_LENGTH2/2;
    new_offset = new_offset + LFN_LENGTH2 + OFFSET_LFN2FROM3;
    read_lfn_bytes(src, new_offset, LFN_LENGTH3, lfn+len, lfn_size-len);
    len = len + LFN_LENGTH3/2;
    
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
    
    long i;
    for (i = str_size-1; i >= 0; i--) {
        if (str[i] == ' ') {
            str[i] = '\0';
        } else {
            break;
        }
    }
    
    return 0;
}
/* 
 * File:   packelf.cpp
 * Author: srl3gx@gmail.com
 *
 * Packing and unpacking boot image of sony mobile devices
 * 
 * Thanks to sony for providing boot image format in packelf.py
 * 
 */

#include <iostream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "elfboot.h"

using namespace std;

void handlePackElf(int, char**);
void handleUnpackElf(int, char**);
void usage();
unsigned int getAddressFromHeader(string);

long getFileSize(FILE*);
void writeElfHeader(FILE*, unsigned int, int);

struct elfphdr part_headers[5];

struct file {
    string file_path;
    unsigned int address;
    unsigned int size;
    string flag;
    unsigned int offset;
};

void writeElfPHeader(FILE*, struct file);

int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("Invalid format...EXIT\n");
        usage();
    }
    char* arg = argv[1];
    if (strcmp(arg, "pack") == 0) {
        printf("Packing elf file.....\n");
        handlePackElf(argc, argv);
    } else if (strcmp(arg, "unpack") == 0) {
        printf("Unpacking elf file.....\n");
        handleUnpackElf(argc, argv);
    } else {
        printf("Invalid format....EXIT");
        usage();
    }
    return 0;
}

void handlePackElf(int argc, char** argv) {
    struct file files[argc - 1];
    string output_path;
    FILE *header;
    FILE *temp_file;
    int parts = 0;
    int offset = 4096;
    for (int i = 2; i < argc; i++) {
        char* arg = argv[i];
        string arguments[3];
        if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
            i++;
            output_path = argv[i];
            continue;
        }
        arg = strtok(arg, "@,=");
        int k = 0;
        while (arg != NULL) {
            arguments[k] = arg;
            arg = strtok(NULL, "@,");
            k++;
        }
        long address;
        if (arguments[1].compare("cmdline") == 0) {
            address = 0L;
            arguments[2] = "cmdline";
        } else if (arguments[0].compare("header") == 0) {
            header = fopen(arguments[1].c_str(), "rb");
            struct elf_header boot_header;
            fseek(header, 0, SEEK_SET);
            fread(&boot_header, elf_header_size, 1, header);
            if (memcmp(boot_header.e_ident, elf_magic, 8) != 0) {
                printf("Header file is not a valid elf image header...Exit");
                exit(EXIT_FAILURE);
            }
            int offset = 52;
            for (int i = 0; i < boot_header.e_phnum; i++) {
                struct elfphdr part_header;
                fseek(header, offset, SEEK_SET);
                fread(&part_header, elf_p_header_size, 1, header);
                part_headers[i] = part_header;
                offset += 32;
            }
            fclose(header);
            continue;
        } else {
            address = strtol(arguments[1].c_str(), NULL, 16);
            if (address == 0) {
                if (arguments[1].empty()){
                    arguments[1] = "kernel";
                }
                arguments[2] = arguments[1];
                address = getAddressFromHeader(arguments[1]);
            }
        }
        printf("Reading file %s\n", arguments[0].c_str());
        temp_file = fopen(arguments[0].c_str(), "r");
        if (temp_file == NULL) {
            printf("Failed to open %s\n", arguments[0].c_str());
            exit(EXIT_FAILURE);
        }
        long size = getFileSize(temp_file);
        struct file f = {arguments[0], address, size, arguments[2], offset};
        offset += size;
        fclose(temp_file);
        files[parts] = f;
        parts++;
    }

    if (parts < 2) {
        printf("Kernel and ramdisk must be specified....Error\n");
        usage();
    }

    if (output_path.empty()) {
        printf("Output path must be specified....Error\n");
        usage();
    }

    FILE* output_file = fopen(output_path.c_str(), "wb");

    if (output_file == NULL) {
        printf("Invalid path %s\n", output_path.c_str());
    }

    writeElfHeader(output_file, files[0].address, parts);

    for (int i = 0; i < parts; i++) {
        writeElfPHeader(output_file, files[i]);
    }
    for (int i = 0; i < parts; i++) {
        struct file current_file = files[i];
        fseek(output_file, current_file.offset, SEEK_SET);
        printf("Writing file : %s to elf image\n", current_file.file_path.c_str());
        temp_file = fopen(current_file.file_path.c_str(), "r");
        if (temp_file == NULL) {
            printf("Cannot read file %s\n", current_file.file_path.c_str());
            exit(EXIT_FAILURE);
        }
        int size = getFileSize(temp_file);
        unsigned char data[size];
        fseek(temp_file, 0, SEEK_SET);
        fread(data, size, 1, temp_file);
        fclose(temp_file);
        fwrite(data, size, 1, output_file);
    }
    fclose(output_file);
}

void writeElfHeader(FILE* file, unsigned int address, int number) {
    printf("Writing elf header\t address : %x \t number : %i\n", address, number);
    struct elf_header header = {
        {0x7F, 0x45, 0x4C, 0x46, 0x01, 0x01, 0x01, 0x61},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        2,
        40,
        1,
        address,
        52,
        0,
        0,
        52,
        32,
        number,
        0,
        0,
        0
    };
    fwrite((char *) &header, sizeof (header), 1, file);
}

void writeElfPHeader(FILE* file, struct file f) {
    printf("Writing part header for %s\t", f.file_path.c_str());
    long flags;
    long type = 1;
    if (f.flag.compare("ramdisk") == 0) {
        printf("Found ramdisk\n");
        flags = 0x80000000;
    } else if (f.flag.compare("ipl") == 0) {
        printf("Found ipl\n");
        flags = 0x40000000;
    } else if (f.flag.compare("cmdline") == 0) {
        printf("Found cmdline\n");
        flags = 0x20000000;
        type = 4;
    } else if (f.flag.compare("rpm") == 0) {
        printf("Found rpm\n");
        flags = 0x01000000;
    } else {
        printf("Using zero flag\n");
        flags = 0;
    }
    printf("Write part header part:%s offset:%i address:%x, size:%i\n", f.flag.c_str(), f.offset, f.address, f.size);
    struct elfphdr phdr = {
        type,
        f.offset,
        f.address,
        f.address,
        f.size,
        f.size,
        flags,
        0
    };
    fwrite((char *) &phdr, sizeof (phdr), 1, file);
}

long getFileSize(FILE* file) {
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

void handleUnpackElf(int argc, char** argv) {

    string input_path;
    string output_path;

    for (int i = 2; i < argc; i += 2) {
        char* arg = argv[i];
        if (strcmp(arg, "-i") == 0 || strcmp(arg, "--input") == 0) {
            input_path = argv[i + 1];
        } else if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
            output_path = argv[i + 1];
        }
    }

    if (input_path.empty()) {
        printf("Input path not provided...Exit\n");
        usage();
    }
    if (output_path.empty()) {
        printf("Output path must be specified....Exit\n");
        usage();
    }

    int offset = 0;
    FILE* temp;
    string tempstr;

    FILE* input_file = fopen(input_path.c_str(), "rb");

    if (input_file == NULL) {
        printf("Cannot read input boot image %s....Exit", input_path.c_str());
    }

    fseek(input_file, offset, SEEK_SET);
    struct elf_header header;
    fread(&header, elf_header_size, 1, input_file);
    offset += elf_header_size;

    if (memcmp(elf_magic, header.e_ident, elf_magic_size) != 0) {
        printf("ELF magic not found....EXIT");
        exit(EXIT_FAILURE);
    }

    int number_of_parts = header.e_phnum;
    printf("Found %i parts in elf image\n", number_of_parts);

    struct elfphdr pheaders[number_of_parts];

    for (int i = 0; i < number_of_parts; i++) {
        fseek(input_file, offset, SEEK_SET);
        fread(&pheaders[i], elf_p_header_size, 1, input_file);
        offset += elf_p_header_size;
    }

    //dump header
    printf("Writing header....\n");
    unsigned char* fileBuffer = (unsigned char*) malloc(4096);
    fseek(input_file, 0, SEEK_SET);
    fread(fileBuffer, 4096, 1, input_file);
    tempstr = output_path + "/header";
    temp = fopen(tempstr.c_str(), "w+");
    fwrite(fileBuffer, 4096, 1, temp);
    fclose(temp);
    free(fileBuffer);
    printf("Done...\n");

    for (int i = 0; i < number_of_parts; i++) {
        struct elfphdr pheader = pheaders[i];
        printf("flag : %u\n", pheader.p_flags);
        printf("offset : %i\n", pheader.p_offset);
        printf("size : %i\n", pheader.p_memsz);
        string name;
        if (pheader.p_flags < 0) {
            pheader.p_flags = -pheader.p_flags;
        }
        switch (pheader.p_flags) {
            case p_flags_cmdline:
                name = "cmdline";
                break;
            case p_flags_kernel:
                name = "kernel";
                break;
            case p_flags_ipl:
                name = "ipl";
                break;
            case p_flags_rpm:
                name = "rpm";
            default:
                name = "ramdisk";
        }
        tempstr = output_path + "/" + name;
        printf("%s found at offset %i with size %i\n", name.c_str(), pheader.p_offset, pheader.p_memsz);
        temp = fopen(tempstr.c_str(), "w+");
        unsigned char* buffer = (unsigned char*) malloc(pheader.p_memsz);
        fseek(input_file, pheader.p_offset, SEEK_SET);
        fread(buffer, pheader.p_memsz, 1, input_file);
        fwrite(buffer, pheader.p_memsz, 1, temp);
        fclose(temp);
        free(buffer);
    }
    fclose(input_file);
}

unsigned int getAddressFromHeader(string flag) {
    unsigned int part_flag;
    if (flag.compare("cmdline") == 0) {
        part_flag = p_flags_cmdline;
    } else if (flag.compare("kernel") == 0) {
        part_flag = p_flags_kernel;
    } else if (flag.compare("ramdisk") == 0) {
        part_flag = p_flags_ramdisk;
    } else if (flag.compare("rpm") == 0) {
        part_flag = p_flags_rpm;
    } else if (flag.compare("ipl") == 0) {
        part_flag = p_flags_ipl;
    } else {
        printf("Unknown flag : %s\n", flag.c_str());
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 5; i++) {
        struct elfphdr part_header;
        part_header = part_headers[i];
        if (part_header.p_flags == part_flag) {
            return part_header.p_paddr;
        }
    }
    printf("Address of %x cannot found from header file", part_flag);
    exit(EXIT_FAILURE);
}

void usage() {
    printf("Usage:\n\n");
    printf("For packing\n");
    printf("If you have header file containing address of kernel, ramdisk etc..\n");
    printf("elftool pack -o output-path header=path/to/header kernel-path "
            "ramdisk-path,ramdisk ipl-path,ipl "
            "rpm-path,rpm cmdline-path@cmdline\n\n");
    printf("elftool pack -o output-path kernel-path@address "
            "ramdisk-path@address,ramdisk ipl-path@address,ipl "
            "rpm-path@address,rpm cmdline-path@cmdline\n\n");
    printf("For unpacking\n");
    printf("elftool unpack -i input-path -o output-path\n");
    exit(EXIT_FAILURE);
}


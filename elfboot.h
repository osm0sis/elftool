/* 
 * File:   elfboot.h
 * Author: srl3gx@gmail.com
 *
 * 
 */

#ifndef ELFBOOT_H
#define	ELFBOOT_H

#ifdef	__cplusplus
extern "C" {
#endif

    struct elf_header {
        char e_ident[8];
        char padding[8];
        unsigned short int e_type;
        unsigned short int e_machine;
        unsigned int e_version;
        unsigned int e_entry;
        unsigned int e_phoff;
        unsigned int e_shoff;
        unsigned int e_flags;
        unsigned short int e_ehsize;
        unsigned short int e_phnetsize;
        unsigned short int e_phnum;
        unsigned short int e_shentsize;
        unsigned short int e_shnum;
        unsigned short int e_shstrndx;
    };

    struct elfphdr {
        unsigned int p_type;
        unsigned int p_offset;
        unsigned int p_vaddr;
        unsigned int p_paddr;
        unsigned int p_filesz;
        unsigned int p_memsz;
        unsigned int p_flags;
        unsigned int p_align;
    };

    const char elf_magic[8] = {0x7F, 0x45, 0x4C, 0x46, 0x01, 0x01, 0x01, 0x61};
    const int elf_header_size = 52;
    const int elf_p_header_size = 32;
    const int elf_magic_size = 8;
    
    const unsigned int p_flags_ramdisk = 0x80000000;
    const unsigned int p_flags_ipl = 0x40000000;
    const unsigned int p_flags_cmdline = 0x20000000;
    const unsigned int p_flags_rpm = 0x01000000;
    const unsigned int p_flags_kernel = 0;


#ifdef	__cplusplus
}
#endif

#endif	/* ELFBOOT_H */


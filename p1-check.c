/*
 * CS 261 PA1: Mini-ELF header verifier
 *
 * Name: Ben McCray
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "p1-check.h"

const uint32_t magic_num = 0x00464c45;

void usage_p1 ()
{
    printf("Usage: y86 <option(s)> mini-elf-file\n");
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("Options must not be repeated neither explicitly nor implicitly.\n");
}

bool parse_command_line_p1 (int argc, char **argv, bool *header, char **fileName)
{
    // Implement this function
    int opt;

    
    opterr = 0 ;  /* Prevent getopt() from printing error messages */
    char *optionStr = "+hH" ; 

    while ( ( opt = getopt(argc, argv, optionStr ) ) != -1)  
    {
        switch(opt) 
        {
            case 'h':   usage_p1();
                        *header = false;
                        *fileName = NULL;
                        return true;
                        break;

            case 'H':   if (*header == true) 
                        {
                            *header = false;
                            *fileName = NULL;
                            usage_p1();
                            return false;
                        }
                       *header = true;
                        continue;
            
            case '?':   *header = false;
                        *fileName = NULL;
                        usage_p1();
                        return false;
            
            default:    *header = false;
                        break;


        }
    }
    if (optind != argc - 1) 
    {
        usage_p1();
        return false;
    } 
    
    *fileName = argv[optind];
    return true;
    
}

bool read_header (FILE *file, elf_hdr_t *hdr)
{
    // Implement this function
    if(file != NULL && hdr != NULL && fread( hdr, 16, 1, file) == 1 && hdr -> magic == magic_num) 
    {
        return true;  
    }
    return false;
}

void dump_header (elf_hdr_t hdr)
{
    // Implement this function
    uint8_t *copy = (uint8_t *) &hdr.e_version;
    printf("00000000  ");
    for (int i = 0; i < sizeof(elf_hdr_t); i++) 
    {
        printf("%02x ", copy[i]);
        if (i == 7) printf(" ");
    }
    printf("\nMini-ELF version %d\n", hdr.e_version);
    printf("Entry point 0x%x\n", hdr.e_entry);
    printf("There are %d program headers, starting at offset %d (0x%x)\n", 
                    hdr.e_num_phdr, hdr.e_phdr_start, hdr.e_phdr_start);
    if (hdr.e_symtab == 0) 
    {
        printf("There is no symbol table present\n");
    } 
    else 
    {
        printf("There is a symbol table starting at offset %d (0x%x)\n", hdr.e_symtab, hdr.e_symtab);
    }
    if (hdr.e_strtab == 0)
    {
        printf("There is no string table present\n");
    }
    else
    {
        printf("There is a string table starting at offset %d (0x%x)\n", hdr.e_strtab, hdr.e_strtab);
    }
    printf("\n");
}


/*
 * CS 261 PA2: Mini-ELF loader
 *
 * Name: Ben McCray
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "p2-load.h"

void usage_p2 ()
{
    printf("Usage: y86 <option(s)> mini-elf-file\n");
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("  -a      Show all with brief memory\n");
    printf("  -f      Show all with full memory\n");
    printf("  -s      Show the program headers\n");
    printf("  -m      Show the memory contents (brief)\n");
    printf("  -M      Show the memory contents (full)\n");

    printf("Options must not be repeated neither explicitly nor implicitly.\n");
}

bool parse_command_line_p2 (int argc, char **argv,
        bool *header, bool *segments, bool *membrief, bool *memfull,
        char **fileName)
{
    // Implement this function
    int opt;

    opterr = 0 ;  /* Prevent getopt() from printing error messages */
    char *optionStr = "+hHafsmM" ; 

    while ( ( opt = getopt(argc, argv, optionStr ) ) != -1)  
    {
        switch(opt) 
        {
            case 'h':   usage_p2();
                        *header = *segments = *membrief = *memfull = false;
                        *fileName = NULL;
                        return true;
                        break;

            case 'H':   if (*header) 
                        {      
                            *header = *segments = *membrief = *memfull = false;
                            *fileName = NULL;
                            usage_p2();
                            return false;
                        } 
                        *header = true;
                        continue;

            case 'm':   if (*membrief || *memfull)
                        {  
                            *header = *segments = *membrief = *memfull = false;
                            usage_p2();
                            return false;
                        }
                        *membrief = true;
                        continue;

            case 'M':   if (*memfull || *membrief)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p2();
                            return false;
                        }
                        *memfull = true;
                        continue;

            case 's': if (*segments)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p2();
                            return false;               
                        }
                        *segments = true;
                        continue;

            
            case 'a':   if (*header || *segments || *membrief || *memfull)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p2();
                            return false;
                        }
                        *header = *segments = *membrief = true;
                        continue;

            case 'f':   if (*header || *segments || *memfull || *membrief)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p2();
                            return false;
                        }
                        *header = *segments = *memfull = true;
                        continue;
        
            case '?':   *header = *segments = *membrief = *memfull = false;
                        usage_p2();
                        return false;
            
            default:    *header = *segments = *membrief = *memfull = false;
                        break;


        }
    }
    if (optind != argc - 1) 
    {
        usage_p2();
        return false;
    } 
    
    *fileName = argv[optind];
    return true;
}

bool read_phdr (FILE *file, uint16_t offset, elf_phdr_t *phdr)
{
    
    // Check NULL
    if (file == NULL || phdr == NULL)
    {
        return false;
    }
    // Check file position and read
    if (fseek(file, offset, SEEK_SET) == 0 && fread(phdr, sizeof(elf_phdr_t), 1, file) == 1)
    {
        // Check phdr validation
        return phdr -> magic == 0xDEADBEEF;
    }
    return false;
}

void dump_phdrs (uint16_t numphdrs, elf_phdr_t phdr[])
{
    printf("Segment   Offset    VirtAddr  FileSize  Type      Flag \n");
    for (int i = 0; i < numphdrs; i++)
    {
        char* type;
        switch (phdr[i].p_type) 
        {
            case 0:
                type = "DATA   ";
                break;
            case 1:
                type = "CODE   ";
                break;
            case 2:
                type = "STACK  ";
                break;
            case 3:
                type = "HEAP   ";
                break;
            case 4:
                type = "UNKNOWN";
                break;
        }
        char* flag;
        switch (phdr[i].p_flag)
        {
            case 4:
                flag = "R ";
                break;
            case 5:
                flag = "R X";
                break;
            case 6:
                flag = "RW ";
                break;
        }
        printf("  %02d      0x%04x    0x%04x    0x%04x    %s   %s\n", i, phdr[i].p_offset, phdr[i].p_vaddr, 
                                phdr[i].p_filesz, type, flag);
    }
    printf("\n");
}

bool load_segment (FILE *file, memory_t memory, elf_phdr_t phdr)
{

    // Check NULL
    if (file == NULL || memory == NULL || phdr.p_vaddr + phdr.p_filesz > MEMSIZE) 
    {
        return false;
    }

    // Validate or return true if zero-sized segment
    return (fseek(file, phdr.p_offset, SEEK_SET) == 0 
            && fread(&memory[phdr.p_vaddr], phdr.p_filesz, 1, file) == 1) || phdr.p_filesz == 0;

}

void dump_memory (memory_t memory, uint16_t start, uint16_t end)
{
    int count = 0;
    printf("Contents of memory from %04x to %04x:\n", start, end);
    for (int i = start; i < end; i++)
    {
        if (count == 8)
        {
            printf(" ");
        }
        if (count == 16) 
        {
            printf("\n");
            count = 0;
        }
        if (count == 0)
        {
            printf("  %04x  ", i);
        }
        printf("%02x", memory[i]);
        if (count != 15) {
            printf(" ");
        }
        count++;
    }
    printf("\n\n");
}

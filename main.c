/*
 * CS 261: P4 Mini-ELF Interpreter Main driver
 *
 * Name: Ben Mccray
 *    or risk losing points, and delays when you seek my help during office hours 
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "p1-check.h"
#include "p2-load.h"
#include "p3-disas.h"
#include "p4-interp.h"
#include "y86.h"
#include "elf.h"


/*-------------------------------------------------------------------------*/

int main (int argc, char **argv)
{
    // Parse command line and check for failure
    bool header, segments, membrief, memfull, disas_code, disas_data, exec_normal, exec_debug;
    header = segments = membrief = memfull = disas_code = disas_data = exec_normal = exec_debug = false;
    char *fileName = NULL;
    if ( ! parse_command_line_p4 ( argc, argv, &header, &segments, &membrief, &memfull, &disas_code, &disas_data, &exec_normal, &exec_debug, &fileName ) )
        exit( EXIT_FAILURE );

    if (!fileName)
    {
        exit(EXIT_SUCCESS);
    }

    FILE *file = fopen(fileName, "r");
    if (!file) 
    {
        printf("Failed to open File\n");
        exit(EXIT_FAILURE);
    }


    elf_hdr_t hdr;
    if (!read_header(file, &hdr)) 
    {
        printf("Failed to Read ELF Header");
        exit( EXIT_FAILURE );
    }

    if (header) 
    {
        dump_header(hdr);
    }
    elf_phdr_t *phdr = malloc(sizeof(elf_phdr_t) * hdr.e_num_phdr);
    memory_t memory = calloc(MEMSIZE, sizeof(memory_t));
    
    for (int i = 0; i < hdr.e_num_phdr; i++) 
    {
        if (!read_phdr(file, hdr.e_phdr_start + (i * sizeof(elf_phdr_t)), &phdr[i]))
        {
            free(phdr);
            free(memory);
            printf("Failed to Read Program Header");
            exit( EXIT_FAILURE );
        }
        if (!load_segment(file, memory, phdr[i]))
        {
            printf("Failed to Load Segment");
            exit(EXIT_FAILURE);
        }
        
    }  
    
    if (segments)
    {
        dump_phdrs(hdr.e_num_phdr, phdr);
    }


    if(membrief)
    {
        for (int i = 0; i < hdr.e_num_phdr; i++) 
        {
            if (phdr[i].p_filesz > 0)
            {
                int addr = phdr[i].p_vaddr - (phdr[i].p_vaddr % 16);
                dump_memory(memory, addr, phdr[i].p_vaddr + phdr[i].p_filesz);
            }
        }
        
    }


    if (memfull)
    {
        dump_memory(memory, 0, MEMSIZE);
    }

    if (disas_code) {
        printf("Disassembly of executable contents:\n");
        for (int i = 0; i < hdr.e_num_phdr; i++) 
        {
            if (phdr[i].p_type == 1)
            {
                disassemble_code(memory, &phdr[i], &hdr);
                printf("\n");
            }
        }
    }
    if (disas_data) {
        printf("Disassembly of data contents:\n");
        for (int i = 0; i < hdr.e_num_phdr; i++) {
            if (phdr[i].p_type == 0) 
            {
                if (phdr[i].p_flag == 4) 
                {
                    disassemble_rodata(memory, &phdr[i]);
                } 
                else if (phdr[i].p_flag == 6) 
                {
                    disassemble_data(memory, &phdr[i]);
                }
                 printf("\n");
            }
        }

    }
    
    
    // ==== Begin Project 4 Solution  =====
  
    if ( exec_normal || exec_debug ) 
    {
        y86_t          cpu ;
        y86_inst_t     ins ;
        // other variables as needed
        bool cond = false;
        y86_register_t valA = 0, valE = 0;
        int count = 0;

        // initialize CPU state
        // ....
        memset(&cpu, 0, sizeof(y86_t));
        cpu.pc = hdr.e_entry;
        cpu.stat = AOK;

        printf("Entry execution point at 0x%04lx\n", cpu.pc );
        printf( "Initial " ); 
        dump_cpu(&cpu);
        // Start the von Neumann cycle       
        while ( cpu.stat == AOK ) 
        {
            count++;

            // Fetch next instruction
            ins = fetch(&cpu, memory); 
            if( ins.type == INVALID) 
            {
                printf("Corrupt Instruction (opcode 0x%x) at address 0x%04x\n", ins.opcode , (uint16_t) cpu.pc  );
                printf("Post-Fetch ");
                dump_cpu(&cpu);
                break ;
            }
            if ( exec_debug )
            { 
                printf("Executing: ");   
                disassemble(ins);
                printf("\n");
            }

            // Decode-Execute
            valE = decode_execute( &cpu, &cond, &ins, &valA );

            // memory writeback & pc update
            memory_wb_pc( &cpu, memory, cond, &ins, valE, valA );
            if ( cpu.stat != AOK || exec_debug )
            {
                printf( "Post-Exec " ); 
                dump_cpu(&cpu); 
            }

        }
        printf("Total execution count: %ld instructions\n\n", (uint64_t) count );

        if ( exec_debug )
        {
            dump_memory(memory, 0, 4096);
        }
    }

    
    // Release the dynamic memory back to the Heap and Close any open files
    free(memory);
    free(phdr);
    
    return EXIT_SUCCESS;    

}

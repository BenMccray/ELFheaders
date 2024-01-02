/*
 * CS 261 PA3: Mini-ELF disassembler
 *
 * Name: Ben Mccray
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p3-disas.h"

//============================================================================
void usage_p3 ()
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
    printf("  -d      Disassemble code contents\n");
    printf("  -D      Disassemble data contents\n");
    printf("Options must not be repeated neither explicitly nor implicitly.");
}

//============================================================================
bool parse_command_line_p3 (int argc, char **argv,
        bool *header, bool *segments, bool *membrief, bool *memfull,
        bool *disas_code, bool *disas_data, char **fileName)
{
    int opt;

    opterr = 0 ;  /* Prevent getopt() from printing error messages */
    char *optionStr = "+hHafsmMdD" ; 

    while ( ( opt = getopt(argc, argv, optionStr ) ) != -1)  
    {
        switch(opt) 
        {
            case 'h':   usage_p3();
                        *header = *segments = *membrief = *memfull = false;
                        *fileName = NULL;
                        return true;
                        break;

            case 'H':   if (*header) 
                        {      
                            *header = *segments = *membrief = *memfull = false;
                            *fileName = NULL;
                            usage_p3();
                            return false;
                        } 
                        *header = true;
                        continue;

            case 'm':   if (*membrief || *memfull)
                        {  
                            *header = *segments = *membrief = *memfull = false;
                            usage_p3();
                            return false;
                        }
                        *membrief = true;
                        continue;

            case 'M':   if (*memfull || *membrief)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p3();
                            return false;
                        }
                        *memfull = true;
                        continue;

            case 's': if (*segments)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p3();
                            return false;               
                        }
                        *segments = true;
                        continue;

            
            case 'a':   if (*header || *segments || *membrief || *memfull)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p3();
                            return false;
                        }
                        *header = *segments = *membrief = true;
                        continue;

            case 'f':   if (*header || *segments || *memfull || *membrief)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p3();
                            return false;
                        }
                        *header = *segments = *memfull = true;
                        continue;

            case 'd':   if (*disas_code)
                        {
                            *disas_code = false;
                            usage_p3();
                            return false;
                        }
                        *disas_code = true;
                        continue;

            case 'D':   if (*disas_data)
                        {
                            *disas_data = false;
                            usage_p3();
                            return false;
                        }
                        *disas_data = true;
                        continue;
        
            case '?':   *header = *segments = *membrief = *memfull = false;
                        usage_p3();
                        return false;
            
            default:    *header = *segments = *membrief = *memfull = false;
                        break;


        }
    }
    if (optind != argc - 1) 
    {
        usage_p3();
        return false;
    } 
    
    *fileName = argv[optind];
    return true;
}

//============================================================================
y86_inst_t fetch (y86_t *cpu, memory_t memory)
{

    
    y86_inst_t ins;
    
    // Initialize the instruction
    memset( &ins , 0 , sizeof(y86_inst_t) );  // Clear all fields i=on instr.    
   // ins.type = INVALID;   // Invalid instruction until proven otherwise
    if (memory == NULL)
    {
        return ins;
    }
    
    address_t  instAddr, *pa; 
    uint64_t   *pv ;
    int64_t    *pd ;
    uint8_t    byte0 , byte1 ;
    y86_rnum_t ra , rb ;
    size_t     size ;    // size of the instruction being fetched

    instAddr = cpu->pc ;      // Where to fetch instruction from
    if( instAddr >= MEMSIZE || instAddr < 0)    // Is it outside the  Address Space?
    {
        cpu->stat = ADR ;    // an invalid fetch address
        return ins ;        // an INVALID instruction
    }
    byte0 = memory[ instAddr ] ; // fetch 1st byte of instr.
    ins.opcode = byte0 ;

    switch( byte0  )       // Inspect the opcode
    {
        case 0x00:
            ins.type = HALT;
            cpu->stat = HLT;
            ins.size = 1 ; 
            break;
    
        case 0x10:
            ins.type = NOP ;
            ins.size = 1 ; 
            break;
    
        case 0x20:  // cmovXX
        case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: 
            size = 2 ;
            // Is instr too big to fit in remaining memory
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            byte1  = memory[ instAddr + 1 ] ; // fetch 2nd byte
            ra = (byte1 & 0xF0) >> 4 ;  
            rb     =  byte1 & 0x0F ;
            if( ra >= BADREG     ||     rb >= BADREG )  // Invalid registers ?
            {
                cpu->stat = INS ;   // invalid instruction encountered
                return ins ;       // INVALID instruction 
            }

            ins.size = size ;
            ins.type = CMOV ;
            ins.cmov = byte0 & 0x0F ;
            ins.ra   = ra ;    ins.rb   = rb ;
            break;
    
        case 0x30:  // irmovq
            size = 10 ;
            // Is instr too big to fit in remaining memory
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            byte1  = memory[ instAddr + 1 ] ; // fetch 2nd byte
            rb     =  byte1 & 0x0F ;
            if( ((byte1 & 0xF0) >> 4) != 0x0F || rb >= BADREG )  // Invalid registers ?
            {
                cpu->stat = INS ;   // invalid instruction encountered
                return ins ;       // INVALID instruction 
            }

            ins.size = size ;
            ins.type = IRMOVQ ;
            pv = (uint64_t *) &memory[instAddr + 2];
            ins.value = *pv;
            ins.rb   = rb ;
            break;
    
        case 0x40:  // rmmovq
            size = 10 ;
            // Is instr too big to fit in remaining memory
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            byte1  = memory[ instAddr + 1 ] ; // fetch 2nd byte
            ra = (byte1 & 0xF0) >> 4;
            rb =  byte1 & 0x0F ;
            if( ra >= BADREG )  // Invalid registers ?
            {
                cpu->stat = INS ;   // invalid instruction encountered
                return ins ;       // INVALID instruction 
            }

            ins.size = size ;
            ins.type = RMMOVQ ;
            pd       = (int64_t *) &memory[instAddr + 2];   // &of 64-bit offset
            ins.d    = *pd ;
            ins.ra = ra;
            ins.rb   = rb ;
            break;
    
        case 0x50:  // mrmovq
            size = 10 ;
            // Is instr too big to fit in remaining memory
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            byte1  = memory[ instAddr + 1 ] ; // fetch 2nd byte
            ra = (byte1 & 0xF0) >> 4;
            rb =  byte1 & 0x0F ;
            // if( ra >= BADREG || rb >= BADREG )  // Invalid registers ?
            // {
            //     cpu->stat = INS ;   // invalid instruction encountered
            //     return ins ;       // INVALID instruction 
            // }

            ins.size = size ;
            ins.type = MRMOVQ ;
            pd       = (int64_t *) &memory[instAddr + 2] ;   // &of 64-bit offset
            ins.d    = *pd ;
            ins.ra = ra;
            ins.rb   = rb ;
            break;
    
        case 0x60:  // addq , subq  , andq  , xorq
        case 0x61:  case 0x62:  case 0x63:  
            size = 2 ;
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            byte1  = memory[ instAddr + 1 ] ; // fetch 2nd byte
            ra = (byte1 & 0xF0) >> 4 ;  
            rb     =  byte1 & 0x0F ;
            ins.op = byte0 & 0x0F;
            if( ins.op >= BADOP || ra >= BADREG     ||     rb >= BADREG )  // Invalid registers ?
            {
                cpu->stat = INS ;   // invalid instruction encountered
                return ins ;       // INVALID instruction 
            }

            ins.size = size ;
            ins.type = OPQ ;
            ins.ra   = ra ;    ins.rb   = rb ;

            break;
    
        case 0x70:  // jXX
        case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: 
            size = 9 ;
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            
            ins.size = size ;
            ins.type = JUMP ;
            ins.jump = byte0 & 0x0F ;
            pa       = (uint64_t *) &memory[instAddr + 1] ;   // &of 64-bit offset
            ins.dest    = *pa ;
            break;
    
        case 0x80:  // call
            size = 9 ;
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            
            ins.size = size ;
            ins.type = CALL ;
            pa       = (uint64_t *) &memory[instAddr + 1] ;   // &of 64-bit offset
            ins.dest    = *pa ;
            break;
    
        case 0x90:  // ret
            ins.type = RET;
            ins.size = 1;
            break;
    
        case 0xA0:  // pushq
            size = 2 ;
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            byte1  = memory[ instAddr + 1 ] ; // fetch 2nd byte
            ra = (byte1 & 0xF0) >> 4 ;  
            if( ra >= BADREG || (byte1 & 0x0F) != 0x0F)  // Invalid registers ?
            {
                cpu->stat = INS ;   // invalid instruction encountered
                return ins ;       // INVALID instruction 
            }

            ins.size = size ;
            ins.type = PUSHQ;
            ins.ra = ra;

            break;
    
        case 0xB0:  // popq
            size = 2 ;
            if( ( instAddr + size ) >= MEMSIZE )
            {
                cpu->stat = ADR ;   // an invalid fetch address
                return ins ;       // INVALID instruction 
            }
            byte1  = memory[ instAddr + 1 ] ; // fetch 2nd byte
            ra = (byte1 & 0xF0) >> 4 ;  
            if( ra >= BADREG || (byte1 & 0x0F) != 0x0F)  // Invalid registers ?
            {
                cpu->stat = INS ;   // invalid instruction encountered
                return ins ;       // INVALID instruction 
            }

            ins.size = size ;
            ins.type = POPQ;
            ins.ra = ra;
            break;
            
        default:
            ins.type  = INVALID ; // INVALID instruction
            cpu->stat = INS ;
            break ;
    
    }
    
    return ins;
}


void printRegNames(y86_rnum_t r)
{
    if (r >= BADREG)
    {
        return;
    }
    switch (r)
    {
        case 0:
            printf("%%rax");
            break;
        case 1:
            printf("%%rcx");
            break;
        case 2:
            printf("%%rdx");
            break;
        case 3:
            printf("%%rbx");
            break;
        case 4:
            printf("%%rsp");
            break;
        case 5:
            printf("%%rbp");
            break;
        case 6:
            printf("%%rsi");
            break;
        case 7:
            printf("%%rdi");
            break;
        case 8:
            printf("%%r8");
            break;
        case 9:
            printf("%%r9"); break;
        case 10:
            printf("%%r10");
            break;
        case 11:
            printf("%%r11");
            break;
        case 12:
            printf("%%r12");
            break;
        case 13:
            printf("%%r13");
            break;
        case 14:
            printf("%%r14");
            break;
        case BADREG:
            return;

    }
}
//============================================================================
void disassemble (y86_inst_t inst)
{
    switch (inst.type)
    {
        case HALT:
            printf ("halt");
            break;
        case NOP:
            printf("nop");
            break;
        case CMOV:
            switch (inst.cmov)
            {
                case RRMOVQ:
                    printf("rrmovq ");
                    break;
                case CMOVLE:
                    printf("cmovle ");
                    break;
                case CMOVL:
                    printf("cmovl ");
                    break;
                case CMOVE:
                    printf("cmove ");
                    break;
                case CMOVNE:
                    printf("cmovne ");
                    break;
                case CMOVGE:
                    printf("cmovge ");
                    break;
                case CMOVG:
                    printf("cmovg ");
                    break;
                case BADCMOV:
                    return;
            
            
            }
            printRegNames(inst.ra);
            printf(", ");
            printRegNames(inst.rb);
            break;
        case IRMOVQ:
            printf("irmovq ");
            printf("0x%lx, ", (uint64_t) inst.value);
            printRegNames(inst.rb);
            break;
        case RMMOVQ:
            printf("rmmovq ");
            printRegNames(inst.ra);
           
            printf(", 0x%lx", inst.d);
            if (inst.rb < BADREG) {
                printf("(");
                printRegNames(inst.rb);
                printf(")");
            } else
            printRegNames(inst.rb);
           
            
            break;
        case MRMOVQ:
            printf("mrmovq ");
            printf("0x%lx", inst.d);
            if (inst.rb < BADREG) {
                printf("(");
                printRegNames(inst.rb);
                printf(")");
            } else
            printRegNames(inst.rb);
            printf(", ");
            printRegNames(inst.ra);
            break;
        case OPQ:
            switch (inst.op)
            {
                case ADD:
                    printf("addq ");
                    break;
                case SUB:
                    printf("subq ");
                    break;
                case AND:
                    printf("andq ");
                    break;
                case XOR:
                    printf("xorq ");
                    break;
                case BADOP:
                    return;
            }
            printRegNames(inst.ra);
            printf(", ");
            printRegNames(inst.rb);
            break;
        case JUMP:
            switch (inst.jump)
            {
                case JMP:
                    printf("jmp ");
                    break;
                case JLE:
                    printf("jle ");
                    break;
                case JL:
                    printf("jl ");
                    break;
                case JE:
                    printf("je ");
                    break;
                case JNE:
                    printf("jne ");
                    break;
                case JGE:
                    printf("jge ");
                    break;
                case JG:
                    printf("jg ");
                    break;
                case BADJUMP:
                    return;

                
            }
            printf("0x%lx", (uint64_t) inst.dest);
            break;
        case CALL:
            printf("call ");
            printf("0x%lx", (uint64_t) inst.dest);
            break;
        case RET:
            printf("ret");
            break;
        case PUSHQ:
            printf("pushq ");
            printRegNames(inst.ra);
            break;
        case POPQ:
            printf("popq ");
            printRegNames(inst.ra);
            break;
        case INVALID:
            
            return;
    }
}

//============================================================================
void disassemble_code (memory_t memory, elf_phdr_t *phdr, elf_hdr_t *hdr)
{

    if (memory == NULL || phdr == NULL || hdr == NULL)
    {
        return;
    }
    y86_t cpu; // CPU struct to store "fake" PC
    y86_inst_t ins; // struct to hold fetched instruction

    // start at beginning of the segment
    cpu.pc = phdr->p_vaddr;
    printf("  0x%03lx:%21s ", (uint64_t) cpu.pc, " ");
    printf("| .pos 0x%03lx code\n", cpu.pc);
        
    // iterate through the segment one instruction at a time
    while ( cpu.pc < phdr->p_vaddr + phdr->p_filesz )
    {
        ins = fetch ( &cpu , memory ); // fetch instruction

        
        // print current address and raw hex bytes of instruction
       
        if (cpu.pc == hdr->e_entry)
        {
            printf("  0x%03lx:%31s", (uint64_t) cpu.pc, "| _start:");
            printf("\n");
        }
        if (ins.type == INVALID)
        {
            printf("Invalid opcode: 0x%02x\n", ins.opcode);
            cpu.pc += ins.size;
            return;
        }
        
        printf("  0x%03lx: ", (uint64_t) cpu.pc);
        int padding = 10 - ins.size;
        for (int i = cpu.pc; i < cpu.pc + ins.size; i++)
        {
            printf ("%02x", memory[i]);
        }
        for (int i = 0; i < padding; i++)
        {
            printf("  ");
        }
        printf(" |   ");
        disassemble ( ins ); // print assembly code
        printf("\n");
        cpu.pc += ins.size; // update PC (next instruction)
    }   
}

//============================================================================
void disassemble_data (memory_t memory, elf_phdr_t *phdr)
{
    if (memory == NULL || phdr == NULL)
    {
        return;
    }
    y86_t cpu; // CPU struct to store "fake" PC

    // start at beginning of the segment
    cpu.pc = phdr->p_vaddr;

    // iterate through the segment one instruction at a time
    while ( cpu.pc < phdr->p_vaddr + phdr->p_filesz )
    {

        // print current address and raw hex bytes of instruction
        if (cpu.pc == phdr->p_vaddr)
        {
            printf("  0x%03lx:%21s ", (uint64_t) cpu.pc, " ");
            printf("| .pos 0x%lx data", cpu.pc);
            printf("\n");
        }
    

        printf("  0x%03lx: ", (uint64_t) cpu.pc);
        for (int i = cpu.pc; i < cpu.pc + 8; i++)
        {
            printf ("%02x", memory[i]);
        }
        printf("     |   .quad 0x");
        
        uint64_t *memPtr = (uint64_t*) &memory[cpu.pc];
        printf("%lx\n", *memPtr);
        cpu.pc += 8;
    }
}

//============================================================================
void disassemble_rodata (memory_t memory, elf_phdr_t *phdr)
{
    if (memory == NULL || phdr == NULL)
    {
        return;
    }
    y86_t cpu; // CPU struct to store "fake" PC

    // start at beginning of the segment
    cpu.pc = phdr->p_vaddr;
    int numBytes = 0;

    // iterate through the segment one instruction at a time
    while ( cpu.pc < phdr->p_vaddr + phdr->p_filesz )
    {

        // print current address and raw hex bytes of instruction
        if (cpu.pc == phdr->p_vaddr)
        {
            printf("  0x%03lx:%21s ", (uint64_t) cpu.pc, " ");
            printf("| .pos 0x%lx rodata", cpu.pc);
            printf("\n");
        }
    
        int padding = 10;
        printf("  0x%03lx: ", (uint64_t) cpu.pc);
        bool complete = false;
        int i;
        for (i = cpu.pc; i < cpu.pc + 10; i++) {
            printf("%02x", memory[i]);
            padding--;
            if (memory[i] == 0x00) {
                complete = true;
                for (int j = 0; j < padding; j++) {
                    printf("  ");
                }
                break;
            }
        }
        
        printf (" |   .string ");
        i = cpu.pc;
        printf("\"");
        while (memory[i] != 0x00) {
            printf("%c", memory[i]);
            i++;
        }
        printf("\"");
        
        
        numBytes = (i - cpu.pc) + 1;
        
        if (!complete) {
            padding = 10;
            printf("\n");
            i = cpu.pc + 10;
            int j = 0;
            printf("  0x%03lx: ", (uint64_t) i);
            while (memory[i] != 0x00) {
                printf ("%02x", memory[i++]);
                padding--;
                j++;
                if (j % 10 == 0) {
                    printf(" | \n  0x%03lx: ", (uint64_t) i);
                    padding = 10;
                }
            }
            printf("%02x", memory[i++]);
            for (i = 0; i < padding - 1; i++) {
                printf("  ");
            }
            printf(" |");
        }
        
        
        printf("\n");
        cpu.pc += numBytes;
    }
    return;
}
//============================================================================


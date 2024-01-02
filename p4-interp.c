/*
 * CS 261 PA4: Mini-ELF interpreter
 *
 * Name: Ben Mccray
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4-interp.h"
y86_register_t registerFile(y86_t *cpu, y86_rnum_t rX);
bool cmovCond(y86_cmov_t cmov, y86_t *cpu);
bool jmpCond(y86_jump_t jmp, y86_t *cpu);
y86_register_t setOpCond(y86_register_t valB, y86_op_t op, y86_register_t *valA, y86_t *cpu);
void writeBack(y86_t *cpu, y86_rnum_t rX, y86_register_t valX);

//=======================================================================

void usage_p4 ()
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
    printf("  -e      Execute program\n");
    printf("  -E      Execute program (debug trace mode)\n");
    printf("Options must not be repeated neither explicitly nor implicitly.\n");
}

//=======================================================================

bool parse_command_line_p4 
    (   int argc         , char **argv      ,
        bool *header     , bool *segments   , bool *membrief , 
        bool *memfull    , bool *disas_code , bool *disas_data ,
        bool *exec_normal, bool *exec_debug , char **fileName 
    )
{
    int opt;

    opterr = 0 ;  /* Prevent getopt() from printing error messages */
    char *optionStr = "+hHafsmMdDeE" ; 

    while ( ( opt = getopt(argc, argv, optionStr ) ) != -1)  
    {
        switch(opt) 
        {
            case 'h':   usage_p4();
                        *header = *segments = *membrief = *memfull = false;
                        *fileName = NULL;
                        return true;
                        break;

            case 'H':   if (*header) 
                        {      
                            *header = *segments = *membrief = *memfull = false;
                            *fileName = NULL;
                            usage_p4();
                            return false;
                        } 
                        *header = true;
                        continue;

            case 'm':   if (*membrief || *memfull)
                        {  
                            *header = *segments = *membrief = *memfull = false;
                            usage_p4();
                            return false;
                        }
                        *membrief = true;
                        continue;

            case 'M':   if (*memfull || *membrief)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p4();
                            return false;
                        }
                        *memfull = true;
                        continue;

            case 's': if (*segments)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p4();
                            return false;               
                        }
                        *segments = true;
                        continue;

            
            case 'a':   if (*header || *segments || *membrief || *memfull)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p4();
                            return false;
                        }
                        *header = *segments = *membrief = true;
                        continue;

            case 'f':   if (*header || *segments || *memfull || *membrief)
                        {
                            *header = *segments = *membrief = *memfull = false;
                            usage_p4();
                            return false;
                        }
                        *header = *segments = *memfull = true;
                        continue;

            case 'd':   if (*disas_code)
                        {
                            *disas_code = false;
                            usage_p4();
                            return false;
                        }
                        *disas_code = true;
                        continue;

            case 'D':   if (*disas_data)
                        {
                            *disas_data = false;
                            usage_p4();
                            return false;
                        }
                        *disas_data = true;
                        continue;
            
            case 'e':   if (*exec_debug) {
                            continue;
                        }
                        *exec_normal = true;
                        continue;
            
            case 'E':   if (*exec_normal) {
                            *exec_normal = false;
                        }
                        *exec_debug = true;
                        continue;
        
            case '?':   *header = *segments = *membrief = *memfull = false;
                        usage_p4();
                        return false;
            
            default:    *header = *segments = *membrief = *memfull = false;
                        break;


        }
    }
    if (optind != argc - 1) 
    {
        
        usage_p4();
        return false;
    } 
    
    *fileName = argv[optind];
    return true;
}

//=======================================================================

void dump_cpu( const y86_t *cpu ) 
{
    printf("dump of Y86 CPU:\n");
    printf("  %%rip: %016lx   flags: SF%d ZF%d OF%d  ", cpu->pc, cpu->sf, cpu->zf, cpu->of);
    switch(cpu->stat) {
        case INS: printf("INS\n"); break;
        case AOK: printf("AOK\n"); break;
        case ADR: printf("ADR\n"); break;
        case HLT: printf("HLT\n"); break;
    }
    printf("  %%rax: %016lx    %%rcx: %016lx\n", cpu->rax, cpu->rcx);
    printf("  %%rdx: %016lx    %%rbx: %016lx\n", cpu->rdx, cpu->rbx);
    printf("  %%rsp: %016lx    %%rbp: %016lx\n", cpu->rsp, cpu->rbp);
    printf("  %%rsi: %016lx    %%rdi: %016lx\n", cpu->rsi, cpu->rdi);
    printf("   %%r8: %016lx     %%r9: %016lx\n", cpu->r8, cpu->r9);
    printf("  %%r10: %016lx    %%r11: %016lx\n", cpu->r10, cpu->r11);
    printf("  %%r12: %016lx    %%r13: %016lx\n", cpu->r12, cpu->r13);
    printf("  %%r14: %016lx\n\n", cpu->r14);
}

//=======================================================================

y86_register_t decode_execute(  y86_t *cpu , bool *cond , const y86_inst_t *inst ,
                                y86_register_t *valA 
                             )
{
    if (cpu == NULL || inst == NULL || cond == NULL || valA == NULL || cpu->stat == INS) {
        
        cpu->stat = INS;
        return 0;
    }
    if (cpu->pc >= MEMSIZE || cpu->pc < 0) {
        cpu->stat = ADR;
        return 0;
    }
    y86_register_t valE = 0, valB = 0;
    switch (inst->type) {

        case HALT:
            cpu->stat = HLT;
            break;

        case NOP:
            break;

        case CMOV:
            //decode
            *valA = registerFile(cpu, inst->ra);
            //exec
            valE = *valA;
            *cond = cmovCond(inst->cmov, cpu);
            break;
        
        case IRMOVQ:
            valE = inst->value;
            break;
        
        case RMMOVQ:
            *valA = registerFile(cpu, inst->ra);
            if (inst->rb < BADREG) {
                valB = registerFile(cpu, inst->rb);
            }

            valE = (uint64_t) valB + inst->d;
            break;

        case MRMOVQ:
            if (inst->rb < BADREG) {
                valB = registerFile(cpu, inst->rb);
            }

            valE = valB + inst->d;
            break;
        
        case OPQ:
            *valA = registerFile(cpu, inst->ra);
            valB = registerFile(cpu, inst->rb);

            valE = setOpCond(valB, inst->op, valA, cpu);
            break;
        
        case JUMP:
            *cond = jmpCond(inst->jump, cpu);
            break;

        case CALL:
            valB = cpu->rsp;

            valE = valB - 8;
            break;
        
        case RET:
            *valA = cpu->rsp;
            valB = cpu->rsp;

            valE = valB + 8;
            break;
        
        case PUSHQ:
            *valA = registerFile(cpu, inst->ra);
            valB = cpu-> rsp;

            valE = valB - 8;
            break;
        
        case POPQ:
            *valA = cpu->rsp;
            valB = cpu->rsp;

            valE = valB + 8;
            break;
        
        case INVALID:
            cpu->stat = INS;
            break;

        
    }
    return valE;
}

//=======================================================================

void memory_wb_pc(  y86_t *cpu , memory_t memory , bool cond , 
                    const y86_inst_t *inst , y86_register_t  valE , 
                    y86_register_t  valA 
                 )
{

    if (cpu->pc >= MEMSIZE || cpu->pc < 0 || memory == NULL || valE < 0 || valA < 0) {
        cpu->stat = ADR;
        return;
    }

    
    y86_register_t valM;
    uint64_t *memPtr;

    switch (inst->type) {

        case HALT:
            cpu->of = false;
            cpu->sf = false;
            cpu->zf = false;

            cpu->pc += inst->size;
            break;

        case NOP:
            cpu->pc += inst->size;
            break;

        case CMOV:
            if (cond) {
                writeBack(cpu, inst->rb, valE);
            }
            cpu->pc += inst->size;
            break;
        
        case IRMOVQ:
            writeBack(cpu, inst->rb, valE);
            cpu->pc += inst->size;
            break;
        
        case RMMOVQ:
            if (valE >= MEMSIZE) {
                cpu->stat = ADR;
                break;
            }
            memPtr = (uint64_t*) &memory[valE];
            *memPtr = valA;
            //printf("here");
            printf("Memory write to 0x%04lx: 0x%lx\n", valE, valA);

            cpu->pc += inst->size;
            break;

        case MRMOVQ:
            if (valE >= MEMSIZE) {
                cpu->stat = ADR;
                cpu->pc += inst->size;
                break;
            }
            memPtr = (uint64_t*) &memory[valE];
            valM = *memPtr;

            writeBack(cpu, inst->ra, valM);
        //    printf("---%d---", inst->size);
            cpu->pc += inst->size;
            break;
        
        case OPQ:
            writeBack(cpu, inst->rb, valE);
            cpu->pc += inst->size;
            break;
        
        case JUMP:
            if (cond) {
                cpu->pc = inst->dest;
                break;
            }
            cpu->pc += inst->size;
            break;

        case CALL:
            if (valE >= MEMSIZE) {
                cpu->stat = ADR;
                cpu->pc = inst->dest;
                break;
            }
            memPtr = (uint64_t*) &memory[valE];
            *memPtr = cpu->pc + inst->size;

            cpu->rsp = valE;
            
            cpu->pc = inst->dest;
            printf("Memory write to 0x%04lx: 0x%lx\n", valE, *memPtr);
            break;
        
        case RET:
            if (valA >= MEMSIZE) {
                cpu->stat = ADR;
                break;
            }
            memPtr = (uint64_t*) &memory[valA];
            valM = *memPtr;

            cpu->rsp = valE;

            cpu->pc = valM;
            break;
        
        case PUSHQ:
            if (valE >= MEMSIZE) {
                cpu->stat = ADR;
                break;
            }
            memPtr = (uint64_t*) &memory[valE];
            *memPtr = valA;

            cpu->rsp = valE;

            printf("Memory write to 0x%04lx: 0x%lx\n", valE, *memPtr);
            cpu->pc += inst->size;
            break;
        
        case POPQ:
            if (valA >= MEMSIZE) {
                cpu->stat = ADR;
                break;
            }
            memPtr = (uint64_t*) &memory[valA];
            valM = *memPtr;

            cpu->rsp = valE;
            writeBack(cpu, inst->ra, valM);
            cpu->pc += inst->size;
            break;
        
        case INVALID:
                printf("here");

            cpu->stat = INS;
            break;

        
    }
}


// HELPERS -------------------------------------------------------------

bool cmovCond(y86_cmov_t cmov, y86_t *cpu) {
    bool cc = false;

    switch (cmov) {
        case RRMOVQ:
            cc = true;
            break;
        
        case CMOVLE:
            if (cpu->zf || cpu->sf ^ cpu->of) {
                cc = true;
            }
            break;
        case CMOVL:
            if (cpu->sf ^ cpu->of) {
                cc = true;
            }
            break;
        
        case CMOVE:
            if (cpu->zf) {
                cc = true;
            }
            break;
        case CMOVNE:
            if (!cpu->zf) {
                cc = true;
            }
            break;
        case CMOVGE:
            if (!(cpu->sf ^ cpu->of)) {
                cc = true;
            }
            break;

        case CMOVG:
            if (!cpu->zf && !(cpu->sf ^ cpu->of)) {
                cc = true;
            }
            break;
        case BADCMOV:
            cpu->stat = INS;
            break;
    }
    return cc;
}

bool jmpCond(y86_jump_t jmp, y86_t *cpu) {
    bool cc = false;

    switch (jmp) {
        case JMP:
            cc = true;
            break;
        
        case JLE:
            if (cpu->zf || (cpu->sf ^ cpu->of)) {
                cc = true;
            }
            break;
        case JL:
            if ((cpu->sf ^ cpu->of)) {
                cc = true;
            }
            break;
        
        case JE:
            if (cpu->zf) {
                cc = true;
            }
            break;
        case JNE:
            if (!cpu->zf) {
                cc = true;
            }
            break;
        case JGE:
            if (!(cpu->sf ^ cpu->of)) {
                cc = true;
            }
            break;

        case JG:
            if (!cpu->zf && !(cpu->sf ^ cpu->of)) {
                cc = true;
            }
            break;
        case BADJUMP:
            cpu->stat = INS;
            break;
    }
    return cc;
}

y86_register_t setOpCond(y86_register_t valB, y86_op_t op, y86_register_t *valA, y86_t *cpu) {
    
    y86_register_t valE = 0;
    int64_t sE, sA = *valA, sB = valB;

    switch (op) {
        case ADD:
            sE = sA + sB;
            cpu->of = ((sE > 0 && sB < 0 && sA < 0) || (sE < 0 && sB > 0 && sA > 0));
            valE = sE;
            break;
        
        case SUB:
            sE = sB - sA;
            cpu->of = ((sE > 0 && sB < 0 && sA > 0) || (sE < 0 && sB > 0 && sA < 0));
            valE = sE;
            break;

        case AND:
            valE = valB & *valA;
            cpu->of = 0;
            break;

        case XOR:
            valE = valB ^ *valA;
            cpu->of = 0;
            break;
        
        case BADOP:
            cpu->stat = INS;
            break;
        default: 
            cpu->stat = INS;
            break;
    }
    cpu->zf = valE == 0;
    cpu->sf = valE >> 63 == 1;
    return valE;
}

 y86_register_t registerFile(y86_t *cpu, y86_rnum_t rX) {

    y86_register_t valX = 0;

    switch (rX) {
        case RAX:
            valX = cpu->rax;
            break;
        case RCX:
            valX = cpu->rcx;
            break;
        case RDX:
            valX = cpu->rdx;
            break;
        case RBX:
            valX = cpu->rbx;
            break;
        case RSP:
            valX = cpu->rsp;
            break;
        case RBP:
            valX = cpu->rbp;
            break;
        case RSI:
            valX = cpu->rsi;
            break;
        case RDI:
            valX = cpu->rdi;
            break;
        case R8:
            valX = cpu->r8;
            break;
        case R9:
            valX = cpu->r9;
            break;
        case R10:
            valX = cpu->r10;
            break;
        case R11:
            valX = cpu->r11;
            break;
        case R12:
            valX = cpu->r12;
            break;
        case R13:
            valX = cpu->r13;
            break;
        case R14:
            valX = cpu->r14;
            break;
        case BADREG:
            cpu->stat = INS;
            return 0;
        default:
            return 0;
        
    }
    return valX;
}

void writeBack(y86_t *cpu, y86_rnum_t rX, y86_register_t valX) {
    switch (rX) {
        case RAX:
            cpu->rax = valX;
            break;
        case RCX:
            cpu->rcx = valX;
            break;
        case RDX:
            cpu->rdx = valX;
            break;
        case RBX:
            cpu->rbx = valX;
            break;
        case RSP:
            cpu->rsp = valX;
            break;
        case RBP:
            cpu->rbp = valX;
            break;
        case RSI:
            cpu->rsi = valX;
            break;
        case RDI:
            cpu->rdi = valX;
            break;
        case R8:
            cpu->r8 = valX;
            break;
        case R9:
            cpu->r9 = valX;
            break;
        case R10:
            cpu->r10 = valX;
            break;
        case R11:
            cpu->r11 = valX;
            break;
        case R12:
            cpu->r12 = valX;
            break;
        case R13:
            cpu->r13 = valX;
            break;
        case R14:
            cpu->r14 = valX;
            break;
        case BADREG:
            cpu->stat = INS;
        default: break;
    }
}
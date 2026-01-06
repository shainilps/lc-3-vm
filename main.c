#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MEMORY_MAX (1 << 16)

uint16_t memory[MEMORY_MAX]; // 65536

enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT,
};

uint16_t reg[R_COUNT];

enum {
    OP_BR = 0, // branch
    OP_ADD,    // add
    OP_LD,     // load
    OP_ST,     // store
    OP_JSR,    // jump register
    OP_AND,    // and
    OP_LDR,    // load register
    OP_STR,    // store register
    OP_RTI,    // unsused
    OP_NOT,    // bitwise not
    OP_LDI,    // load indirect
    OP_STI,    // store indirect
    OP_JMP,    // jump
    OP_RES,    // reserved (unused)
    OP_LEA,    // load effective address
    OP_TRAP,   // execute trap
};

enum {
    FL_POS = 1 << 0, // P (1)
    FL_ZRO = 1 << 1, // Z (2)
    FL_NEG = 1 << 2, // N (4)
};

enum {
    TRAP_GETC = 0x20,  // get character from keyboard, not echoed onto terminal
    TRAP_OUT = 0x21,   // output a character
    TRAP_PUTS = 0x22,  // output a word string
    TRAP_IN = 0x23,    // get character from keyboard, echoed onto terminal
    TRAP_PUTSP = 0x24, // output a byte string
    TRAP_HALT = 0x25   // halt the program
};

uint16_t sign_extend(uint16_t x, int bit_count) {

    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }

    return x;
}

void update_flags(uint16_t r) {
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZRO;
    } else if (reg[r] >> 15) {
        reg[R_COND] = FL_NEG;
    } else {
        reg[R_COND] = FL_POS;
    }
}
uint16_t swap16(uint16_t x) { return (x << 8) | (x >> 8); }

void read_image_file(FILE *file) {
    // the origin where in the memory to place the image
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    // we know the max file size so we ony need one fread
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t *p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    // swap to little endian
    while (read-- > 0) {
        *p = swap16(*p);
        ++p;
    }
}

int read_image(const char *image_path) {
    FILE *file = fopen(image_path, "rb");
    if (!file) {
        return 0;
    }
    read_image_file(file);
    fclose(file);
    return 1;
}

void mem_write(uint16_t address, uint16_t val) { memory[address] = val; }

uint16_t mem_read(uint16_t address) {
    // if (address == MR_KBSR) {
    //     if (check_key()) {
    //     }
    // }
    //
    return memory[address];
}

int main(int argc, const char *argv[]) {

    if (argc < 2) {
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j) {
        if (!read_image(argv[j])) {
            printf("failed to load iamge: %s\n", argv[j]);
            exit(1);
        }
    }

    // @{Setup}

    // since exactly one codition  flag should be set at a time, set the Z flag
    reg[R_COND] = FL_ZRO;

    // set the PC to starting postition
    // 0x3000 is the default
    // lower addresses are used for traps
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;

    while (running) {

        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;

        switch (op) {
        case OP_ADD: {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag) {
                uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                reg[r0] = reg[r1] + imm5;
            } else {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] + reg[r2];
            }
            update_flags(r0);
            break;
        }
        case OP_AND: {
            uint16_t r0 = (instr >> 9) & 0x7;
            uint16_t r1 = (instr >> 6) & 0x7;
            uint16_t imm_flag = (instr >> 5) & 0x1;

            if (imm_flag) {
                uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                reg[r0] = reg[r1] & imm5;
            } else {
                uint16_t r2 = instr & 0x7;
                reg[r0] = reg[r1] & reg[r2];
            }
            update_flags(r0);
            break;
        }

        case OP_NOT: {
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t sr = (instr >> 6) & 0x7;

            reg[dr] = ~reg[sr];

            update_flags(dr);
            break;
        }

        case OP_BR: {

            uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);
            uint16_t cond_flag = (instr >> 9) & 0x7;

            // uint16_t n = (instr >> 11) & 0x1;
            // uint16_t z = (instr >> 10) & 0x1;
            // uint16_t p = (instr >> 9) & 0x1;

            // if ((p && (reg[R_COND] & FL_POS)) ||
            //     (z && (reg[R_COND] & FL_ZRO)) ||
            //     (n && (reg[R_COND] & FL_NEG))) {
            //     reg[R_PC] += pc_offset9;
            // }

            if (cond_flag & reg[R_COND]) {
                reg[R_PC] += pc_offset9;
            }
            break;
        }
        case OP_JMP: {
            uint16_t baseR = (instr >> 6) & 0x7;
            reg[R_PC] = reg[baseR];
            break;
        }
        case OP_JSR: {
            uint16_t m = (instr >> 11) & 0x1;
            reg[R_R7] = reg[R_PC];
            if (m) {
                uint16_t pcOffset11 = sign_extend(instr & 0x7FF, 11);
                reg[R_PC] += pcOffset11;
            } else {
                uint16_t baseR = (instr >> 6) & 0x7;
                reg[R_PC] = reg[baseR];
            }
            break;
        }
        case OP_LD: {
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);

            reg[dr] = mem_read(reg[R_PC] + pc_offset9);
            update_flags(dr);
            break;
        }
        case OP_LDI: {
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);

            reg[dr] = mem_read(mem_read(reg[R_PC] + pc_offset9));
            update_flags(dr);
            break;
        }
        case OP_LDR: {
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t baseR = (instr >> 6) & 0x7;
            uint16_t pc_offset6 = sign_extend(instr & 0x3F, 6);

            reg[dr] = mem_read(reg[baseR] + pc_offset6);
            update_flags(dr);
            break;
        }
        case OP_LEA: {
            uint16_t dr = (instr >> 9) & 0x7;
            uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);

            reg[dr] = reg[R_PC] + pc_offset9;
            update_flags(dr);
            break;
        }
        case OP_ST: {
            uint16_t sr = (instr >> 9) & 0x7;
            uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);

            mem_write(reg[R_PC] + pc_offset9, sr);
            break;
        }
        case OP_STI: {
            uint16_t sr = (instr >> 9) & 0x7;
            uint16_t pc_offset9 = sign_extend(instr & 0x1FF, 9);

            mem_write(mem_read(reg[R_PC] + pc_offset9), sr);
            break;
        }
        case OP_STR: {
            uint16_t sr = (instr >> 9) & 0x7;
            uint16_t baseR = (instr >> 6) & 0x7;
            uint16_t pc_offset6 = sign_extend(instr & 0x3F, 6);

            mem_write(reg[baseR] + pc_offset6, sr);
            break;
        }
        case OP_TRAP: {
            reg[R_R7] = reg[R_PC];

            switch (instr & 0xFF) {
            case TRAP_GETC:
                reg[R_R0] = (uint16_t)getchar();
                update_flags(R_R0);
                break;
            case TRAP_OUT: {
                putc((char)reg[R_R0], stdout);
                fflush(stdout);
                break;
            }
            case TRAP_PUTS: {
                uint16_t *c = memory + reg[R_R0];
                while (*c) {
                    putc((char)*c, stdout);
                    ++c;
                }
                fflush(stdout);
                break;
            }
            case TRAP_IN: {
                char c = getchar();
                putc(c, stdout);
                fflush(stdout);
                reg[R_R0] = (uint16_t)c;
                update_flags(R_R0);
                break;
            }
            case TRAP_PUTSP: {

                uint16_t *c = memory + reg[R_R0];
                while (*c) {
                    char char1 = (*c) & 0xFF;
                    putc(char1, stdout);
                    char char2 = (*c) >> 8;
                    if (char2)
                        putc(char2, stdout);
                    ++c;
                }
                fflush(stdout);
                break;
            }
            case TRAP_HALT:
                puts("HALT");
                fflush(stdout);
                break;
            }
            break;
        }

        case OP_RES:
        case OP_RTI:
            abort();
        default:
            // @{BAD OPCODE}
            break;
        }
    }

    return 0;
}

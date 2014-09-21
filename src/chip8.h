#ifndef _CHIP8_H_
#define _CHIP8_H_

typedef uint8_t  u8;
typedef uint16_t u16;

#define MEMSIZ 4096
#define STKSIZ 16
#define REGCNT 16
#define SCRWTH 64
#define SCRHGT 32
#define NUMKEY 16

#define fatal(msg, op)\
    do {\
        endwin();\
        fprintf(stderr,"fatal: " msg "\n"\
                "Opcode: 0x%04X\n", op);\
        exit(1);\
    } while (0)

#define stack_push(x)\
    do {\
        if (sp < STKSIZ)\
            stack[sp++] = (x);\
        else\
            fatal("Stack overflow", op);\
    } while (0)

#define stack_pop(x)\
    do {\
        if (sp > 0)\
            x = stack[--sp];\
        else\
            fatal("Stack underflow", op);\
    } while (0)

#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

#define MEMSIZ 4096
#define STKSIZ 16
#define REGCNT 16
#define SCRWTH 64
#define SCRHGT 32
#define NUMKEY 16
#define u8     uint8_t
#define u16    uint16_t

/* Intepreter globals */
u8  mem[MEMSIZ]    = { 0 };
u16 stack[STKSIZ]  = { 0 };     /* Memory for stack */
u8  rV[REGCNT]     = { 0 };     /* General 'V' registers */
u8  rS = 0;                    /* Special Sound register */
u8  rT = 0;                    /* Special Timer register */
u16 rI = 0;                    /* Special 16bit 'I' register */
u8  sp = 0;                    /* Stack Pointer */
u16 pc = 0x200;                /* Program Counter */

/* Sprites to load into memory */
u8 fontset[80] = 
{
    0xf0, 0x90, 0x90, 0x90, 0xf0, /* 0 */
    0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
    0xf0, 0x10, 0xf0, 0x80, 0xf0, /* 2 */
    0xf0, 0x10, 0xf0, 0x10, 0xf0, /* 3 */
    0x90, 0x90, 0xf0, 0x10, 0x10, /* 4 */
    0xf0, 0x80, 0xf0, 0x10, 0xf0, /* 5 */
    0xf0, 0x80, 0xf0, 0x90, 0xf0, /* 6 */
    0xf0, 0x10, 0x20, 0x40, 0x40, /* 7 */
    0xf0, 0x90, 0xf0, 0x90, 0xf0, /* 8 */
    0xf0, 0x90, 0xf0, 0x10, 0xf0, /* 9 */
    0xf0, 0x90, 0xf0, 0x90, 0x90, /* A */
    0xe0, 0x90, 0xe0, 0x90, 0xe0, /* B */
    0xf0, 0x80, 0x80, 0x80, 0xf0, /* C */
    0xe0, 0x90, 0x90, 0x90, 0xe0, /* D */
    0xf0, 0x80, 0xf0, 0x80, 0xf0, /* E */
    0xf0, 0x80, 0xf0, 0x80, 0x80  /* F */
};

/* keymap */
u8 keymap[NUMKEY] =
{
    'x', '1', '2', '3',
    'q', 'w', 'e', 'a',
    's', 'd', 'z', 'c',
    '4', 'r', 'f', 'v'
};

/* ncurses globals */
WINDOW *win;

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

void update_timer(int signal)
{
    (void)signal;

    if (rT > 0)
        rT--;
    if (rS > 0)
        rS--;
}

u8 random255(void)
{
    return rand() & 255;
}

/* Adjust 16 bit value for appropriate input */
u16 endian(u16 in)
{
    return (*(u16*)"\0\xff" < 0x100) ? in : ((in >> 8) & 0xff) | ((in & 0xff) << 8);
}

#define ___ 31 /* Wildcard for match fn */
int match(const u16 val, const int d1, const int d2, const int d3, const int d4)
{
    if (  (d4 != ___ && d4 != ((val >> 0)  & 0xf))
       || (d3 != ___ && d3 != ((val >> 4)  & 0xf))
       || (d2 != ___ && d2 != ((val >> 8)  & 0xf))
       || (d1 != ___ && d1 != ((val >> 12) & 0xf))
       )
        return 0;
    else
        return 1;
}

void parse_and_exec(u16 op)
#define mp(a,b,c,d) match(op,a,b,c,d)
{

    /* Chip-8 instructions */
         if (mp( 0x0,0x0,0xe,0x0 )) { /* 00E0 - CLS */
        wclear(win);
        wrefresh(win);
    }
    else if (mp( 0x0,0x0,0xe,0xe )) { /* 00EE - RET */
        stack_pop(pc);
        return;
    }
    else if (mp( 0x0,___,___,___ )) { /* 0nnn - SYS addr */
        /* nop */
    }
    else if (mp( 0x1,___,___,___ )) { /* 1nnn - JP addr */
        pc = op & 0xfff;
        return;
    }
    else if (mp( 0x2,___,___,___ )) { /* 2nnn - CALL addr */
        stack_push(pc);
        pc = op & 0xfff;
        return;
    }
    else if (mp( 0x3,___,___,___ )) { /* 3xkk - SE Vx, byte */
        if (rV[(op >> 8) & 0xf] == (op & 0xff))
            pc+=2;
    }
    else if (mp( 0x4,___,___,___ )) { /* 4xkk - SNE Vx, byte */
        if (rV[(op >> 8) & 0xf] != (op & 0xff))
            pc+=2;
    }
    else if (mp( 0x5,___,___,0x0 )) { /* 5xk0 - SE Vx, Vy */
        if (rV[(op >> 8) & 0xf] == rV[(op >> 4) & 0xf])
            pc+=2;
    }
    else if (mp( 0x6,___,___,___ )) { /* 6xkk - LD Vx, byte */
        rV[(op >> 8) & 0xf] = op & 0xff;
    } 
    else if (mp( 0x7,___,___,___ )) { /* 7xkk - ADD Vx, byte */
        rV[(op >> 8) & 0xf] += op & 0xff;
    }
    else if (mp( 0x8,___,___,0x0 )) { /* 8xy0 - LD Vx, Vy */
        rV[(op >> 8) & 0xf] = rV[(op >> 4) & 0xf];
    }
    else if (mp( 0x8,___,___,0x1 )) { /* 8xy1 - OR Vx, Vy */
        rV[(op >> 8) & 0xf] |= rV[(op >> 4) & 0xf];
    }
    else if (mp( 0x8,___,___,0x2 )) { /* 8xy2 - AND Vx, Vy */
        rV[(op >> 8) & 0xf] &= rV[(op >> 4) & 0xf];
    }
    else if (mp( 0x8,___,___,0x3 )) { /* 8xy3 - XOR Vx, Vy */
        rV[(op >> 8) & 0xf] ^= rV[(op >> 4) & 0xf];
    }
    else if (mp( 0x8,___,___,0x4 )) { /* 8xy4 - ADD Vx, Vy */
        u8 tmp = rV[(op >> 8) & 0xf];
        rV[(op >> 8) & 0xf] += rV[(op >> 4) & 0xf];
        rV[0xf] = rV[(op >> 8) & 0xf] < tmp ? 1 : 0;
    }
    else if (mp( 0x8,___,___,0x5 )) { /* 8xy5 - SUB Vx, Vy */
        u8 tmp = rV[(op >> 8) & 0xf];
        rV[(op >> 8) & 0xf] -= rV[(op >> 4) & 0xf];
        rV[0xf] = rV[(op >> 8) & 0xf] > tmp ? 0 : 1;
    }
    else if (mp( 0x8,___,___,0x6 )) { /* 8xy6 - SHR Vx {, Vy} */
        rV[0xf] = rV[(op >> 8) & 0xf] & 1;
        rV[(op >> 8) & 0xf] >>= 1;
    }
    else if (mp( 0x8,___,___,0x7 )) { /* 8xy7 - SUBN Vx, Vy */
        u8 tmp = rV[(op >> 8) & 0xf];
        rV[(op >> 8) & 0xf] = rV[(op >> 4) & 0xf] - rV[(op >> 8) & 0xf];
        rV[0xf] = rV[(op >> 8) & 0xf] > tmp ? 0 : 1;
    }
    else if (mp( 0x8,___,___,0xe )) { /* 8xyE - SHL Vx {, Vy} */
        rV[0xf] = rV[(op >> 8) & 0xf] & 0x80;
        rV[(op >> 8) & 0xf] <<= 1;
    }
    else if (mp( 0x9,___,___,0x0 )) { /* 9xy0 - SNE Vx, Vy */
        if (rV[(op >> 8) & 0xf] != rV[(op >> 4) & 0xf])
            pc+=2;
    }
    else if (mp( 0xa,___,___,___ )) { /* Annn - LD I, addr */
        rI = op & 0xfff;
    }
    else if (mp( 0xb,___,___,___ )) { /* Bnnn - JP V0, addr */
        pc = (op & 0xfff) + rV[0];
    }
    else if (mp( 0xc,___,___,___ )) { /* Cxkk - RND Vx, byte */
        rV[(op >> 8) & 0xf] = random255() & (op & 0xff);
    }
    else if (mp( 0xd,___,___,___ )) { /* Dxyn - DRW Vx, Vy, nibble */
        u8 nib = op & 0xf;
        u8 x   = (op >> 8) & 0xf;
        u8 y   = (op >> 4) & 0xf;

        /* How to display sprite */
        int i, j;
        for (i = 0; i < nib; ++i) {
            for (j = 7; j >= 0; --j) {
                int yy =  y + i;
                int xx = (x + (7 - j)) % SCRWTH;
                int curchar = mvwinch(win, yy, xx) & A_CHARTEXT;
                int toggle  = 0;

                if (mem[rI + i] & (1 << j)) {
                    toggle = 1;
                    if (curchar == '*')
                        rV[0xf] = 1;
                }

                if (toggle)
                    mvwaddch(win, yy, xx, curchar == '*' ? ' ' : '*');
            }
        }
        wrefresh(win);
    }
    else if (mp( 0xe,___,0x9,0xe )) { /* Ex9E - SKP Vx */
        int ch = getch();
        if (ch != ERR) {
            u8 index = rV[(op >> 8) & 0xf];
            if (index < NUMKEY && ch == keymap[index])
                pc+=2;
        }
    }
    else if (mp( 0xe,___,0xa,0x1 )) { /* ExA1 - SKNP Vx */
        int ch = getch();
        if (ch != ERR) {
            u8 index = rV[(op >> 8) & 0xf];
            if (index < NUMKEY && ch == keymap[index])
                pc+=2;
        }
    }
    else if (mp( 0xf,___,0x0,0x7 )) { /* Fx07 - LD Vx, DT */
        rV[(op >> 8) & 0xf] = rT;
    }
    else if (mp( 0xf,___,0x0,0xa )) { /* Fx0A - LD Vx, K */
        nodelay(win, FALSE);

        /* Check input for valid key from keypress */
        int i;
        for (;;) {
            int ch = getch(); 
            if (('a' <= ch && ch <= 'f') || ('0' <= ch && ch <= '9')) {
                for (i = 0; i < NUMKEY; ++i) {
                    if (ch == keymap[i])
                        goto found_char;
                }
            }
        }

    found_char:
        rV[(op >> 8) & 0xf] = i;
        nodelay(win, TRUE);
    }
    else if (mp( 0xf,___,0x1,0x5 )) { /* Fx15 - LD DT, Vx */
        rT = rV[(op >> 8) & 0xf];
    }
    else if (mp( 0xf,___,0x1,0x8 )) { /* Fx18 - LD ST, Vx */
        rS = rV[(op >> 8) & 0xf];
    }
    else if (mp( 0xf,___,0x1,0xe )) { /* Fx1E - ADD I, Vx */
        rI += rV[(op >> 8) & 0xf];  /* Check that we don't need to set carry */
    }
    else if (mp( 0xf,___,0x2,0x9 )) { /* Fx29 - LD F, Vx */
        /* Set up table */
        rI = mem[((op >> 8) & 0xf) * 5];
    }
    else if (mp( 0xf,___,0x3,0x3 )) { /* Fx33 - LD B, Vx */
        u8 bcd = rV[(op >> 8) & 0xf];
        mem[rI + 2] = bcd % 10; bcd /= 10;
        mem[rI + 1] = bcd % 10; bcd /= 10;
        mem[rI + 0] = bcd % 10;
    }
    else if (mp( 0xf,___,0x5,0x5 )) { /* Fx55 - LD [I], Vx */
        int i;
        for (i = 0; i < ((op >> 8) & 0xf); ++i)
            mem[rI + i] = rV[i];
    }
    else if (mp( 0xf,___,0x6,0x5 )) { /* Fx65 - LD Vx, [I] */
        int i;
        for (i = 0; i < ((op >> 8) & 0xf); ++i)
            rV[i] = mem[rI + i];
    }
    /* Super chip-48 instructions */
    else if (mp( 0x0,0x0,0xc,___ )) { /* SCD nibble */
    }
    else if (mp( 0x0,0x0,0xf,0xb )) { /* SCR */
    }
    else if (mp( 0x0,0x0,0xf,0xc )) { /* SCL */
    }
    else if (mp( 0x0,0x0,0xf,0xd )) { /* EXIT */
    }
    else if (mp( 0x0,0x0,0xf,0xe )) { /* LOW */
    }
    else if (mp( 0x0,0x0,0xf,0xf )) { /* HIGH */
    }
    else if (mp( 0xd,___,___,0x0 )) { /* DRW Vx, Vy, 0 */
    }
    else if (mp( 0xf,___,0x3,0x0 )) { /* LD HF, Vx */
    }
    else if (mp( 0xf,___,0x7,0x5 )) { /* LD R, Vx */
    }
    else if (mp( 0xf,___,0x8,0x5 )) { /* LD Vx, R */
    }
    else {
        fatal("Invalid opcode", op);
    }

    pc+=2;   /* Update instruction count */
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage %s <Input>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));
    signal(SIGALRM, update_timer); 
    memcpy(mem + 0x50, fontset, 80 * sizeof(u8));

    /* Init curses */
    initscr();
    cbreak();
    noecho();
    nonl();
    curs_set(FALSE);
    win = newwin(SCRHGT, SCRWTH, 0, 0);
    nodelay(win, TRUE);
    wrefresh(win);

    /* Open file in binary mode */
    FILE *fd = fopen(argv[1], "rb");
    if (fd == NULL) {
        perror("Failed to open file");
        return 1;
    }

    /* Load file into memory */
    long len;
    fseek(fd, 0, SEEK_END);
    len = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    /* Ensure have even number of bytes */
    if (len & 1) return 1;

    /* Store program in 16 bit array */
    fread(mem + 0x200, len, sizeof(u8), fd);
    fclose(fd);

    /* Continue to exit until we run off edge or encounter exit */
    //ualarm(1000000, 1000000);
    for (;;) {
        parse_and_exec(mem[pc] << 8 | mem[pc + 1]);
        if (rT > 0) rT--;
        if (rS > 0) rS--;
    }
    
    wclear(win);
    endwin();
    return 0;
}

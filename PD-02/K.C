#include <conio.h>
#include <dos.h>
#include <stdio.h>

#define _NOCURSOR     0x3200
#define _NORMALCURSOR 0x0607
#define _SOLIDCURSOR  0x0007

unsigned char normal_keys[0x60];
unsigned char extended_keys[0x60];


static void _setcursortype(int type) {
    union REGS in,out;
    in.x.cx=type;
    in.h.ah=1;
    int86(0x10,&in,&out);
}



static void interrupt
keyb_int() {
    static unsigned char buffer;
    unsigned char rawcode;
    unsigned char make_break;
    int scancode;

    rawcode = inp(0x60); /* read scancode from keyboard controller */
    make_break = !(rawcode & 0x80); /* bit 7: 0 = make, 1 = break */
    scancode = rawcode & 0x7F;

    if (buffer == 0xE0) { /* second byte of an extended key */
        if (scancode < 0x60) {
            extended_keys[scancode] = make_break;
        }
        buffer = 0;
    } else if (buffer >= 0xE1 && buffer <= 0xE2) {
        buffer = 0; /* ingore these extended keys */
    } else if (rawcode >= 0xE0 && rawcode <= 0xE2) {
        buffer = rawcode; /* first byte of an extended key */
    } else if (scancode < 0x60) {
        normal_keys[scancode] = make_break;
    }

    outp(0x20, 0x20); /* must send EOI to finish interrupt */
}

static void interrupt (*old_keyb_int)();

void
hook_keyb_int(void) {
    old_keyb_int = getvect(0x09);
    setvect(0x09, keyb_int);
}

void
unhook_keyb_int(void) {
    if (old_keyb_int != NULL) {
        setvect(0x09, old_keyb_int);
        old_keyb_int = NULL;
    }
}

int
ctrlbrk_handler(void) {
    unhook_keyb_int();
    _setcursortype(_NORMALCURSOR);
    return 0;
}

static
putkeys(int y, unsigned char const *keys) {
    int i;
    gotoxy(1, y);
    for (i = 0; i < 0x30; i++) {
        putch(keys[i] + '0');
    }
}

void
game(void) {
    _setcursortype(_NOCURSOR);
    clrscr();
    while(!normal_keys[1]) {
        putkeys(1, normal_keys);
        putkeys(2, normal_keys + 0x30);
        putkeys(4, extended_keys);
        putkeys(5, extended_keys + 0x30);
    }
    gotoxy(1, 6);
    _setcursortype(_NORMALCURSOR);
}

int
main() {
    ctrlbrk(ctrlbrk_handler);
    hook_keyb_int();
    game();
    unhook_keyb_int();
    return 0;
}   

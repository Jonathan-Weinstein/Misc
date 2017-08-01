/*
    some methods for psuedo-random generating a permutation of [0, n)
    dont want to shuffle array: uses memory and not as cool

    -lcg with mod = n
    -lcg with mod = ceil_log2(n), skip outputs >= n
    -galois lfsr of bit width ceil_log2(n+1), sub 1 then skip outputs >= n

    considerations:
    -quality of outputs
    -multiple streams

    For this I went with a lfsr for some reason. I also decided I wanted multiple "streams"
    like how pcg describes some of their rngs by having the lcg add not be hardcoded.
    Since an lfsr has a period of w-1 (were w is output bit width, state cant be zero),
    my idea for multiple streams is to xor the state with a mask < (1<<w). This trades not
    genrating zero for not generating the mask value, so I pick the first random value
    as the mask value. Cant do [randval = (lfsr-1) ^ id] and start anywhere because then lfsr would have to be
    1<<w to generate some value after the xor, which is not a possible state.

    pcg says their insecure generators that dont have repetions have good statistical output,
    but only come in 8, 16, 32, and 64 bit varieties.

    This could have been useful for that percolation assignment. However, the quality of
    output for a monte carlo sim like that is important. For this dumb fancy cls it doesnt matter
    if the output isnt statistically good.
*/

/*
    winapi console notes

    -buffer: can be greater than window demensions, scrolls
    -window: actual dimension console takes up on screen,
    resizing it changes buffer dimensions in way you think it'd work?
    bottom right of the SMALL_RECT thing is inclusive
*/

//xxx: black out rest of BUFFER,
//this just clears (0, 0) to SCREEN bottom right

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <stdint.h>

#ifdef __GNUC__
    #include <x86intrin.h>//__rdtsc()
    inline
    int bsr(uint32_t v)
    {
        return __builtin_clz(v)^31;
    }
#else
    //extern "C" unsigned __int64 __rdtsc(void);
    //#pragma intrinsic(__rdtsc)
    #include <intrin.h>
    inline
    int bsr(uint32_t v)
    {
        unsigned long r;
        _BitScanReverse(&r, v);
        return (int)r;
    }
#endif

inline
int ceil_log2_above_one(uint32_t v)
{
    return bsr(v-1u) + 1;
}

HANDLE gHout;

unsigned galois_taps(unsigned x);
void system_perror(const char *str);

inline
void set_color(unsigned colorCode)
{
    SetConsoleTextAttribute(gHout, colorCode);
}

inline
void set_xy(unsigned x, unsigned y)
{
    SetConsoleCursorPosition(gHout, COORD{(short)x, (short)y});
}

inline
int write_console(const char *p, int n)
{
    DWORD nwrit;
    return WriteConsole(gHout, p, n, &nwrit, NULL) ? int(nwrit) : -1;
}

BOOL init(CONSOLE_SCREEN_BUFFER_INFO *p)
{
    gHout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (gHout==INVALID_HANDLE_VALUE)
        return false;

    return GetConsoleScreenBufferInfo(gHout, p);
}

void do_it(unsigned const rowlen, unsigned const tiles, unsigned const p2m1, unsigned const taps)
{
    const char blank[1]={0};

    uint64_t const ent = __rdtsc();
    uint32_t xor_id = uint32_t(ent>>32) * 2654435761u;
    uint32_t lfsr = uint32_t(ent) | 1u;//cant be zero

    xor_id &= p2m1;
    lfsr   &= p2m1;

    unsigned randval = xor_id;
    unsigned unfilled = tiles;

    goto L_enter;

    do{
        do{
            //advance lsfr
            lfsr = lfsr>>1u ^ (taps & -(lfsr&1u));//msvc might warn -(unsigned)
            randval = lfsr ^ xor_id;
        L_enter: (void)0;
        }while(randval>=tiles);//no more than about 50% chance of repeating
        //can do some a preprocessing to replace division by multiplication then shift,
        //but doing some sleeping anyway
        unsigned y=randval/rowlen, x=randval%rowlen;

        set_xy(x, y);
        write_console(blank, 1);

        if ((unfilled&63u)==0u)
            Sleep(1);
    }while(--unfilled);
}

#ifndef NOCRT
int main(void)
#else
extern "C" int WINAPI mainCRTStartup(void)
//g++ -DNOCRT -Wextra -Wall -nostdlib -O2 -o fcls.exe main.cpp -lkernel32 -s
#endif
{
    CONSOLE_SCREEN_BUFFER_INFO oldinfo;
    DWORD oldmode;

    if (!init(&oldinfo))
        return -1;

    //would advance cursor to next line
    //writing at rowlen-1.
    //at bottom right corner, would cause to scroll down
    GetConsoleMode(gHout, &oldmode);
    SetConsoleMode(gHout, 0);
    //@TODO: check if console too small or Y coord to close to end of Y buffer length
    unsigned const rowlen= unsigned(oldinfo.srWindow.Right - oldinfo.srWindow.Left + 1);
    unsigned const tiles = rowlen * unsigned(oldinfo.srWindow.Bottom - oldinfo.srWindow.Top + 1);
    unsigned const power = ceil_log2_above_one(tiles+1);//inc b/c lfsr period is (1<<w)-1, not (1<<w)
    unsigned const p2m1  = (1u<<power) - 1;
    unsigned const taps  = galois_taps(power);
    //pick a randomish color
    {
        unsigned colcode = unsigned(__rdtsc())<<4;
        colcode &= BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
        if (!colcode) colcode = BACKGROUND_RED | BACKGROUND_INTENSITY;
        set_color(colcode);
    }

    for (int n=2;;)
    {
        do_it(rowlen, tiles, p2m1, taps);
        set_xy(0, 0);
        if (--n==0)
            break;
        Sleep(100);
        set_color(oldinfo.wAttributes);
    }

    SetConsoleMode(gHout, oldmode);
    return 0;
}

unsigned galois_taps(unsigned x)
{
    switch (x)
    {
        case  1: return 0x1u;//always output 1
        case  2: return 0x3u;//2,1
        case  3: return 0x6u;//3,2
        case  4: return 0xcu;//4,3
        case  5: return 0x1eu;//5,4,3,2
        case  6: return 0x36u;//6,5,3,2
        case  7: return 0x78u;//7,6,5,4
        case  8: return 0xb8u;//8,6,5,4
        case  9: return 0x1b0u;//9,8,6,5
        case 10: return 0x360u;//10,9,7,6
        case 11: return 0x740u;//11,10,9,7
        case 12: return 0xca0u;//12,11,8,6
        case 13: return 0x1b00u;//13,12,10,9
        case 14: return 0x3500u;//14,13,11,9
        case 15: return 0x7400u;//15,14,13,11
        case 16: return 0xb400u;//16,14,13,11
        case 17: return 0x1e000u;//17,16,15,14
        case 18: return 0x39000u;//18,17,16,13
        case 19: return 0x72000u;//19,18,17,14
        case 20: return 0xca000u;//20,19,16,14
        case 21: return 0x1c8000u;//21,20,19,16
        case 22: return 0x270000u;//22,19,18,17
        case 23: return 0x6a0000u;//23,22,20,18
        case 24: return 0xd80000u;//24,23,21,20
        case 25: return 0x1e00000u;//25,24,23,22
        case 26: return 0x3880000u;//26,25,24,20
        case 27: return 0x7200000u;//27,26,25,22
        case 28: return 0xca00000u;//28,27,24,22
        case 29: return 0x1d000000u;//29,28,27,25
        case 30: return 0x32800000u;//30,29,26,24
        case 31: return 0x78000000u;//31,30,29,28
        case 32: return 0xa3000000u;//32,30,26,25
    }

    return 0;//compiler warn
}

//nothing else below here neede for program

/*
#include <stdio.h>

void system_perror(const char *str)
{
    static_assert(sizeof(TCHAR)==1, "");

    fputs(str, stderr);
    fputs("\r\nError code: ", stderr);

    const unsigned ErrCode=GetLastError();
    unsigned u=ErrCode;

    char buf[256];//avoid fprintf code size
    char *p;
    p=buf+20;

    while (u>=10u) *p-- = (u%10u)+'0', u/=10u;

    *p=u+'0';
    fwrite(p, 1, (buf+21)-p, stderr);
    fputs(".\r\nSystem message: ", stderr);

    DWORD const len=FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        ErrCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf,
        sizeof buf,
        NULL);

    if (len)
        fwrite(buf, 1, len, stderr);
}
*/

/*
BOOL set_console_window_size(COORD tl, COORD br)//inclusive
{
    SMALL_RECT ab;
    ab.Left     = tl.X;
    ab.Top      = tl.Y;
    ab.Right    = br.X;
    ab.Bottom   = br.Y;

    return SetConsoleWindowInfo(gHout, TRUE, &ab);
}
*/

#if 0
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
//https://stackoverflow.com/questions/1937163/drawing-in-a-win32-console-on-c
int main(void)//link with Gdi32
{
    //Get window handle to console, and device context
    HWND console_hwnd = GetConsoleWindow();
    HDC device_context = GetDC(console_hwnd);

    //Here's a 5 pixels wide RED line [from initial 0,0] to 300,300
    HPEN pen = CreatePen(PS_SOLID, 5, RGB(255,0,0));
    SelectObject(device_context, pen);
    LineTo(device_context, 300, 300);

    DeleteObject(pen);
    ReleaseDC(console_hwnd, device_context);
    return 0;
}
#endif

#if 0
#include <stdint.h>

uint_fast32_t galois_taps(unsigned x)
{
    switch (x)
    {
        case  1: return 0x1u;//makes no sense
        case  2: return 0x3u;//2,1
        case  3: return 0x6u;//3,2
        case  4: return 0xcu;//4,3
        case  5: return 0x1eu;//5,4,3,2
        case  6: return 0x36u;//6,5,3,2
        case  7: return 0x78u;//7,6,5,4
        case  8: return 0xb8u;//8,6,5,4
        case  9: return 0x1b0u;//9,8,6,5
        case 10: return 0x360u;//10,9,7,6
        case 11: return 0x740u;//11,10,9,7
        case 12: return 0xca0u;//12,11,8,6
        case 13: return 0x1b00u;//13,12,10,9
        case 14: return 0x3500u;//14,13,11,9
        case 15: return 0x7400u;//15,14,13,11
        case 16: return 0xb400u;//16,14,13,11
        case 17: return 0x1e000u;//17,16,15,14
        case 18: return 0x39000u;//18,17,16,13
        case 19: return 0x72000u;//19,18,17,14
        case 20: return 0xca000u;//20,19,16,14
        case 21: return 0x1c8000u;//21,20,19,16
        case 22: return 0x270000u;//22,19,18,17
        case 23: return 0x6a0000u;//23,22,20,18
        case 24: return 0xd80000u;//24,23,21,20
        case 25: return 0x1e00000u;//25,24,23,22
        case 26: return 0x3880000u;//26,25,24,20
        case 27: return 0x7200000u;//27,26,25,22
        case 28: return 0xca00000u;//28,27,24,22
        case 29: return 0x1d000000u;//29,28,27,25
        case 30: return 0x32800000u;//30,29,26,24
        case 31: return 0x78000000u;//31,30,29,28
        case 32: return 0xa3000000u;//32,30,26,25
    }

    return 0;//compiler warn
}

uint32_t gAu32[1ul << (32-5)];//134 million dwords -> 536 MB

uint32_t get_then_set(uint32_t i)
{
    uint32_t const outerIndex = i>>5;// i/32u
    uint32_t const bit = (uint32_t)1u << (i&31u);// i%32u

    uint32_t const res = gAu32[outerIndex] & bit;
    gAu32[outerIndex] |= bit;
    return res;
}

#include <string.h>

void zero_bits(uint32_t n)
{
    if (n)
        memset(gAu32, 0, ((n-1u)/32u + 1u)*sizeof(uint32_t));
}

#include <stdio.h>

int main(void)
{
    for (int i=32; i>0; --i)
    {
        uint32_t const taps = galois_taps(i);
        uint32_t p2m1;

        if (i!=32)
            p2m1 = (1ul << i) - 1, zero_bits(p2m1);
        else
            p2m1 = ~0ul;

        uint32_t lfsr = 1u;

        while (p2m1--)
        {
            lfsr = (lfsr>>1) ^ (taps & -(lfsr&1u));
            if (get_then_set(lfsr-1))
            {
                printf("failed i = %d\n", i);
                break;
            }
        }

        printf("finished for i = %d\n", i);
    }

    return 0;
}
#endif

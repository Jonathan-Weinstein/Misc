#ifndef H_CONSOLE
#define H_CONSOLE

#include "shared.h"

//make a nicer file and include cmd and pause func
extern HANDLE const hin;
extern HANDLE const hout;

extern unsigned short gConBufWidth;

enum { kOtherMsgOffset=3, kPromptMsgOffset=6, };
enum { color_default=7, color_whisper=8 };



inline
bool con_set_color(unsigned code=color_default)
{
    return SetConsoleTextAttribute(hout, code);
}

inline
void con_write(const char* str, int len)
{
    DWORD x;
    WriteConsole(hout, str, len, &x, nullptr);
}

template<unsigned N>
void con_writelit(const char (&a)[N])
{
    DWORD x;
    WriteConsole(hout, a, N-1, &x, nullptr);
}

inline
void con_put(char c)
{
    DWORD x;
    WriteConsole(hout, &c, 1, &x, nullptr);
}

inline
bool con_set_cursor(COORD pos)
{
    return SetConsoleCursorPosition(hout, pos);
}

inline
COORD con_get_cursor()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    return GetConsoleScreenBufferInfo(hout, &info) ? info.dwCursorPosition : COORD{-1, -1};
}

inline
bool con_oldline(short up=1, short set_x=0)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(hout, &info))
    {
        COORD pos;//={0 info.dwCursorPosition.Y - up};//narrowing conversion?
        pos.X=set_x;
        pos.Y = info.dwCursorPosition.Y - up;
        if (pos.Y<0) pos.Y=0;
        return SetConsoleCursorPosition(hout, pos);
    }
    return false;
}

struct ConIo
{
    unsigned textLen;
    unsigned onlineUserBits;//not really worth it for 4 people
    uint8_t myIdx;
    Packet sendBox;

    enum { target_all=255 };

    ConIo()
    : textLen(0)
    , onlineUserBits(0)
    {
        sendBox.u4tag_u4user=0;//user unused when client sends to server, server knows
        sendBox.set_tag(tag_all_chat);
    }

    void setDataAfterConnect(InitConData dat)
    {
        myIdx=dat.s8idx;
        onlineUserBits = dat.u8onlinebits | (1u<<dat.s8idx);//server did this?
    }

    int printInitialUi();
    int handleConsoleInput(SOCKET);
    int processFullPacket(Packet*);
};

#endif//not H_CONSOLE

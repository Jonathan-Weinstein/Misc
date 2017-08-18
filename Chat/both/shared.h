#ifndef H_SHARED
#define H_SHARED

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#if (_WIN32_WINNT<0x0501)
#warning Recommend compiling with -D_WIN32_WINNT=0x0501 or above.
#endif
#include <ws2tcpip.h>

#ifndef PORT_NETWORK_SHORT
#define PORT_NETWORK_SHORT ((unsigned short)0x8913)//5001 in decimal with bytes reversed
#endif

///could make a .cpp file

#include <stddef.h>
#include <stdint.h>


#if defined(__GNUC__)//clang defines this also
inline
int bsf(unsigned u)
{
    return __builtin_ffs(u)-1;
}

inline
int popcnt(unsigned u)
{
    return __builtin_popcount(u);
}
#elif defined(_MSC_VER)
#include <intrin.h>
inline
int bsf(unsigned u)
{
    unsigned long i;
    return _BitScanForward(&i, u) ? int(i) : -1;
}

inline
int popcnt(unsigned u)
{
    return __popcnt(u);
}
#else
#error unkown compiler
#endif

enum : uint8_t
{
    user_bits=4,
    user_mask = (1u<<user_bits)-1u,
};

enum
{
    kMaxUsers=4,
    kMaxTextLength=236,
    kMaxPacket=238
};

enum pack_tag : uint8_t
{
    tag_whisper_if_below=kMaxUsers, tag_all_chat=kMaxUsers, tag_user_join, tag_user_leave
};

static_assert(kMaxUsers<=8, "");

struct InitConData
{
    enum : int8_t {idx_failure=-1, idx_server_full=-2};

    int8_t s8idx;
    uint8_t u8onlinebits;
};

struct PacketHeader//user is
{
    uint8_t u8txtlen;
    uint8_t u4tag_u4user;//when client send to server, user indicates if whisper or not. user_all means not whisper

    pack_tag tag() const
    {
        return pack_tag(u4tag_u4user>>user_bits);
    }

    void set_tag(pack_tag id)
    {
        u4tag_u4user &= user_mask;//clear old tag
        u4tag_u4user |= id<<user_bits;
    }

    uint8_t user() const
    {
        return u4tag_u4user & user_mask;
    }

    void set_user(uint8_t index)
    {
        u4tag_u4user &= ~user_mask;//clear old user, preserve upper bits
        u4tag_u4user |= index;
    }
};

inline
unsigned packet_size(const void *start)
{
    return unsigned(*static_cast<const uint8_t*>(start))+2u;
}

struct Packet : PacketHeader
{
    char text[kMaxTextLength];
};

static_assert(sizeof(Packet)==kMaxPacket, "");

inline
int wsa_startup()
{
    WSAData d;
    return WSAStartup(0x0202, &d);
}

inline
bool send_all(SOCKET s, const void* data, int n)
{
    const char *p=static_cast<const char*>(data);

    while (n>0)//know will not send 0 bytes
    {
        const int ret=send(s, p, n, 0);
        if (ret<0) return false;//and equal to zero?
        p+=ret;
        n-=ret;
    }

    return true;
}

#include <stdio.h>

inline
void sys_perror_x(const char* str, unsigned u)
{
    static_assert(sizeof(TCHAR)==1, "");

    fputs(str ? str : "null", stderr); fputs(": error code = ", stderr);

    const unsigned ErrCode=u;

    char buf[208];//avoid fprintf code size
    char *p=buf+32;
    buf[31]='\n';
    p=buf+30;

    while (u>=10u) *p-- = (u%10u)+'0', u/=10u;

    *p=u+'0';
    fwrite(p, 1, (buf+32)-p, stderr);
    fputs("system message: ", stderr);

    const DWORD len=FormatMessage(
                  FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr,
                  ErrCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  buf,
                  sizeof buf,
                  nullptr);
    if (len)
        fwrite(buf, 1, len, stderr);
}

inline
void sys_perror(const char* str)
{
    sys_perror_x(str, GetLastError());
}

inline
void socket_perror(const char* str)
{
    sys_perror_x(str, WSAGetLastError());
}

#endif//not H_SHARED

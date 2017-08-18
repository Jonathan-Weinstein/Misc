#ifndef H_RECEIVER
#define H_RECEIVER

#include "shared.h"

struct ConIo;

class Receiver
{
    enum { bufcap=512 };

    char *mpBegin;//pointer to member
    char *mpEnd;
    SOCKET mSock;//closes in dtor
    WSAOVERLAPPED mOverlap;
    char mBuf[bufcap];
public:
    Receiver()
    : mpBegin(mBuf)
    , mpEnd(mBuf)
    , mSock(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
    , mOverlap{}//struct zero
    {
        //event can be null if not using things like WaitForMultipleEvents or GetOverlappedResult with wait argument to true
    }

    ~Receiver()
    {
        if (mSock!=INVALID_SOCKET)
            closesocket(mSock);
    }

    SOCKET sock() const
    {
        return mSock;
    }

    int startAsyncRecv()
    {
        WSABUF vbuf={bufcap, mBuf};
        DWORD flags=0;
        return WSARecv(mSock, &vbuf, 1, nullptr, &flags, &mOverlap, nullptr)==0 ? 0 : WSAGetLastError()-WSA_IO_PENDING;
    }

    bool recvHasUpdated()//const?
    {//just call WSAGetOverlappedResult often and check if error is WSA_IO_INCOMPLETE? Or use event object?
        return HasOverlappedIoCompleted(static_cast<volatile WSAOVERLAPPED*>(&mOverlap));//safe?
    }

    int handleRecvResult(ConIo*);
};

#endif//not H_RECEIVER

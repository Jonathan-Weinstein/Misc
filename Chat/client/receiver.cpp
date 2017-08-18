#include "receiver.h"
#include "console.h"

#include <string.h>

int Receiver::handleRecvResult(ConIo* con)
{
    DWORD xfer, flags;
    if (!WSAGetOverlappedResult(mSock, &mOverlap, &xfer, false, &flags))//already checked that io should be complete?
    {
        sys_perror("\nWSAGetOverlappedResult");
        return -1;
    }
    flags=0;//for next call

    if (!xfer)
    {
        con_writelit("\nthe server disconnected\n");
        return -1;
    }

    mpEnd+=xfer;

    unsigned pksz;//check packet size<=kMaxPacket
    unsigned nPartial=mpEnd-mpBegin;
    WSABUF vbuf;
    //for some reason I have reimplemented this using pointers from the index one in the server
    for (;;)
    {
        if ((pksz=packet_size(mpBegin)) > kMaxPacket)
        {
            con_writelit("got too large packet from supposed server");
            return -1;
        }

        if (nPartial<pksz)
            break;

        if (con->processFullPacket(reinterpret_cast<Packet*>(mpBegin))!=0)
            return -1;

        mpBegin += pksz;
        nPartial=unsigned(mpEnd-mpBegin);
        if (nPartial==0u)//dont call packet_size beyond
            break;
    }

    if (nPartial==0u)
    {
        vbuf.buf=mpEnd=mpBegin=mBuf;
        vbuf.len=bufcap;
    }
    else
    {
        const unsigned slack=(mBuf+bufcap)-mpEnd;
        if (slack < (pksz-nPartial))//if not enough slack at end to get full packet
        {
            memmove(mBuf, mpEnd, nPartial);
            mpEnd=mBuf+nPartial;

            vbuf.buf=mpBegin=mBuf;
            vbuf.len=bufcap-nPartial;
        }
        else//leave begin and end the same
        {
            vbuf.buf=mpEnd;
            vbuf.len=slack;
        }
    }

    if (WSARecv(mSock, &vbuf, 1, nullptr, &flags, &mOverlap, nullptr)==0 || WSAGetLastError()==WSA_IO_PENDING)
        return 0;
    else
    {
        sys_perror("\nSuccessive WSARecv failed");
        return -1;
    }
}

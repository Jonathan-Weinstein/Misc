#include "shared.h"

#include <string.h>
#include <conio.h>//press a key to end the server loop

struct RecvContext : WSAOVERLAPPED
{
    int iBegin;
    int iEnd;
    //hEvent unused in callback, will be the socket
    SOCKET sock() const { return uintptr_t(hEvent); }
    void assignSock(SOCKET s) { hEvent=reinterpret_cast<void*>(s); }
};

void CALLBACK firstRecvCompRout(DWORD, DWORD, WSAOVERLAPPED *, DWORD);

enum { kRecvBoxCap=512 };

struct RecvBlock
{
    char data[kRecvBoxCap];
};

static RecvBlock gRecvBlocks[kMaxUsers];

static_assert(int(kMaxPacket)<=kRecvBoxCap, "");//memcpy?

class AcceptColony//see notes on other data structure ideas
{
    uint32_t mOccupiedIndexes;//can't displace back unless change to ptr instead of whole
    RecvContext mContexts[kMaxUsers];

    enum:uint32_t { kIdxMask = (1ULL<<kMaxUsers)-1ULL };//so if Max is 4 this is 15 which is the first 4 bits on
public:
    AcceptColony()
    : mOccupiedIndexes(0u)
    {
        //overlapped struct and other data init on insert
    }

    ~AcceptColony();//close all sockets in occupied slots

    unsigned indexOf(const RecvContext* slot) const
    {
        return slot-mContexts;
    }

    bool full() const
    {
        return mOccupiedIndexes==kIdxMask;
    }

    const char* handleAcceptEvent();//call full() before this

    void disconnectAndRemove(unsigned i);

    void sendToAllClients(const void* data, int n);

    void processFullPacket(unsigned senderIdx, Packet* pack);
};

void server_loop();

bool initListener();

static AcceptColony gAccepts;

static SOCKET gListenSock;
static WSAEVENT gAcceptEvent;

int main(void)
{
    if (wsa_startup()==0)
    {
        if (initListener())
            server_loop();

        if (gListenSock!=INVALID_SOCKET) closesocket(gListenSock);
        if (gAcceptEvent!=WSA_INVALID_EVENT) WSACloseEvent(gAcceptEvent);
        WSACleanup();
    }
    else
        socket_perror("WSAStartup failed");

    if (strchr(GetCommandLine(), ':')!=nullptr)
    {
        fputs("press any key...", stdout);
        _getch();
    }

    return 0;
}

void server_loop()
{
    const char *err=nullptr;

    do
    {

        DWORD const r=WaitForSingleObject(gAcceptEvent, 0);//could combine alertable?

        if (r==WAIT_OBJECT_0)
        {
            err=gAccepts.handleAcceptEvent();
        }
        else if (r!=WAIT_TIMEOUT)
        {
            err="WaitForSingleObject";
        }

        SleepEx(1000, true);

        if (_kbhit())
            err=reinterpret_cast<const char*>(-1);
    }while (err==nullptr);//loop on int in x64 instead?

    if (err==reinterpret_cast<const char*>(-1))
    {
        puts("loop canceled from key hit");
        _getch();
    }
    else
        sys_perror(err);
}

bool initListener()
{
    const char* err;
    sockaddr_in adin;
    adin.sin_family=AF_INET;
    adin.sin_addr.s_addr=INADDR_ANY;
    adin.sin_port=PORT_NETWORK_SHORT;

    gListenSock=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    gAcceptEvent=WSACreateEvent();
    if (gListenSock==INVALID_SOCKET || gAcceptEvent==WSA_INVALID_EVENT)
    {
        err="socket or WSACreateEvent";
        goto Lerror;
    }
    //Will also set non-blocking, which
    //accepts will inherit.
    //Not sure if I want this for my use of send()

    if (bind(gListenSock, (const sockaddr*)&adin, sizeof adin)!=0)
    {
        err="bind";
        goto Lerror;
    }

    if (WSAEventSelect(gListenSock, gAcceptEvent, FD_ACCEPT)!=0)
    {
        err="WSAEventSelect";
        goto Lerror;
    }

    if (listen(gListenSock, kMaxUsers*2)!=0)
    {
        err="listen";
        goto Lerror;
    }

    return true;
Lerror:
    sys_perror(err);
    return false;
}

AcceptColony::~AcceptColony()//close all sockets in occupied slots
{
    unsigned bits=mOccupiedIndexes;

    if (bits)
    {
        do
        {
            const unsigned i=bsf(bits);
            closesocket(mContexts[i].sock());
            bits^=(1u<<i);
        }while (bits);
    }
}

void AcceptColony::disconnectAndRemove(unsigned i)
{
    closesocket(mContexts[i].sock());
    mOccupiedIndexes &= ~(1u<<i);//should be on, could ^
    if (mOccupiedIndexes)//sendToAllClients also checks if theres anyone
    {
        PacketHeader head;
        head.u8txtlen=0;
        head.set_tag(tag_user_leave);
        head.set_user(i);
        sendToAllClients(&head, 2);
    }
}

const char* AcceptColony::handleAcceptEvent()
{
    const SOCKET asock=accept(gListenSock, nullptr, nullptr);

    if (asock==INVALID_SOCKET)
        return "accept failed";

    WSAResetEvent(gAcceptEvent);//auto reset?

    if (!this->full())
    {
        puts("accepted and keeping");

        const unsigned i = bsf(~mOccupiedIndexes);//call full() before this

        PacketHeader head;
        head.u8txtlen=0;
        head.set_tag(tag_user_join);
        head.set_user(i);
        sendToAllClients(&head, 2);

        mContexts[i]={};//zero everything;
        mContexts[i].assignSock(asock);
        mOccupiedIndexes |= (1u<<i);

        WSAResetEvent(gAcceptEvent);//auto reset?

        InitConData dat;
        dat.u8onlinebits=mOccupiedIndexes;
        dat.s8idx=i;

        if (send(asock, reinterpret_cast<char*>(&dat), 2, 0)!=2)
            return "failed to send new accepted initial data";

        WSABUF vbuf = { kRecvBoxCap, gRecvBlocks[i].data };
        DWORD flags=0;//must supply zero or some valid value

        return WSARecv(asock, &vbuf, 1, nullptr, &flags, mContexts+i, firstRecvCompRout)==0
                || WSAGetLastError()==WSA_IO_PENDING ? nullptr : "First posted WSARecv failed";
    }
    else//full
    {
        puts("short accept because full");

        InitConData dat;
        dat.s8idx=InitConData::idx_server_full;
        const socklen_t len=sizeof(int);
        const int nobuf=0;

        setsockopt(asock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&nobuf), len);
        send(asock, reinterpret_cast<const char*>(&dat), 2, 0);
        closesocket(asock);
        return nullptr;
    }
}

void AcceptColony::sendToAllClients(const void* data, int n)
{
    unsigned bits=mOccupiedIndexes;
    if (!bits)
        return;

    puts("sending shared message to at least 1 client");

    do
    {
        const unsigned i=bsf(bits);
        if (!send_all(mContexts[i].sock(), data, n))
        {
            sys_perror("failed to send all data to a socket");
            closesocket(mContexts[i].sock());
            mOccupiedIndexes &= ~(1u<<i);
        }
        bits &= ~(1u<<i);
    }while (bits);
}

//heres where everything happens
//this is not a fast function do to the sends
//this may not be a good idea
//alertable IO: horrible idea?
//1-thread(main thread only) iocp?
void CALLBACK firstRecvCompRout(DWORD err, DWORD xfer, WSAOVERLAPPED* pOvl, DWORD)
{
    puts("completion routine called");

    RecvContext *const self=static_cast<RecvContext*>(pOvl);
    const int senderIdx=gAccepts.indexOf(self);

    if (err!=0)//???????
    {
        sys_perror("WSARecv fail notice in completion routine");
        gAccepts.disconnectAndRemove(senderIdx);
        return;
    }

    if (xfer==0)
    {
        puts("remote end disconnected");
        gAccepts.disconnectAndRemove(senderIdx);
        return;
    }

    self->iEnd += xfer;

    char *const pBufTrueBegin=gRecvBlocks[senderIdx].data;
    unsigned nPartial = self->iEnd - self->iBegin;
    unsigned packsz;
    WSABUF vbuf;

    for (;;)
    {
        packsz = packet_size(pBufTrueBegin + self->iBegin);
        if (packsz>kMaxPacket)
        {
            puts("got bad packet from user");
            gAccepts.disconnectAndRemove(senderIdx);
            return;
        }

        if (nPartial < packsz)
            break;

        gAccepts.processFullPacket(senderIdx, reinterpret_cast<Packet*>(pBufTrueBegin + self->iBegin));

        self->iBegin += packsz;
        nPartial = self->iEnd - self->iBegin;
        if (nPartial==0u)//dont call packet_size beyond recently filled memory
            break;
    }

    if (nPartial==0u)
    {
        self->iEnd = 0;
        self->iBegin = 0;
    }
    else if (unsigned(kRecvBoxCap - self->iEnd) < (packsz-nPartial))//if not enough slack at end
    {
        memmove(pBufTrueBegin, pBufTrueBegin+self->iBegin, nPartial);//circular buffering, scatter gather, memcpy, need contig anyway
        self->iEnd = nPartial;
        self->iBegin = 0;
    }
    //else//leave begin and end the same if slack at end

    vbuf.len = kRecvBoxCap - self->iEnd;
    vbuf.buf = pBufTrueBegin + self->iBegin;

    if (WSARecv(self->sock(), &vbuf, 1, nullptr, &err, self, firstRecvCompRout)!=0
        && WSAGetLastError()!=WSA_IO_PENDING)
    {
        sys_perror("WSARecv posted at end of completion routine failed, disconnecting and continuing.");
        gAccepts.disconnectAndRemove(senderIdx);
    }
}

void AcceptColony::processFullPacket(unsigned senderIdx, Packet* pack)
{
    const unsigned sz=packet_size(pack);
    const unsigned char whisperDest=pack->tag();
    pack->set_user(senderIdx);

    if (whisperDest>=tag_whisper_if_below)
    {
        pack->set_tag(tag_all_chat);

        mOccupiedIndexes &= ~(1u<<senderIdx);//dont send to sender
        gAccepts.sendToAllClients(pack, sz);
        mOccupiedIndexes |= (1u<<senderIdx);
    }
    else//is whisper
    {
        if ((mOccupiedIndexes>>whisperDest)!=0u)
        {
            if (!send_all(mContexts[whisperDest].sock(), pack, sz))
                sys_perror("send_all failed to send whisper");
        }
        else
            puts("was told to whisper to user not online");
    }
}

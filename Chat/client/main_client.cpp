#include "console.h"
#include "receiver.h"

#ifndef IPV4_HOST_LONG
#define IPV4_HOST_LONG 0xA00007Ful//10.0.0.127
#endif

int main_startup();
void main_loop();
void main_cleanup();
void pause_if_not_cmd();

InitConData contact_sever(SOCKET);

int main(void)
{
    if (main_startup()==0)
    {
        main_loop();
        main_cleanup();
    }

    pause_if_not_cmd();
    return 0;
}

int main_startup()
{
    if (hin!=INVALID_HANDLE_VALUE && hout!=INVALID_HANDLE_VALUE)
    {
        //Effects of next call include only recording keyboard and window resize console events,
        //also disables processed input, so CTRL-C has no immediate effect but is picked up as a key event.
        if (SetConsoleMode(hin, ENABLE_WINDOW_INPUT))//Don't think need to get old mode to reset on exit,
        {                                            //unlike something such as changing color or screen size.
            WSAData dc;
            if (WSAStartup(0x0202, &dc)==0)
                return 0;
            else
                sys_perror("WSAStartup");
        }
        else
            sys_perror("SetConsoleMode");
    }
    return -1;
}

void main_cleanup()
{
    WSACleanup();
    //reset console input or output modes?
    //color should be default
    //screen size?
}

#include <string.h>
void pause_if_not_cmd()
{
    if (strchr(GetCommandLine(), ':')!=nullptr)//I thought of this dumb thing, works on "cls & a.exe"
    {
        con_writelit("press any key...");
        FlushConsoleInputBuffer(hin);
        char c;
        DWORD len;
        ReadConsole(hin, &c, 1, &len, nullptr);//set console mode to take mostly key events and not wait for enter
    }//can also use WaitForSingleObject or _kbhit()/_getch()?
}

void main_loop()
{
    Receiver recver;
    ConIo con;
    if (recver.sock()==INVALID_SOCKET)
    {
        sys_perror("failed to create socket");
        return;
    }
    const InitConData dat=contact_sever(recver.sock());
    if (dat.s8idx==InitConData::idx_failure)
        return;
    if (dat.s8idx==InitConData::idx_server_full)
    {
        con_writelit("The server is currently at full capacity, try again later.\n");
        return;
    }
    con.setDataAfterConnect(dat);
    if (con.printInitialUi()!=0)
    {
        shutdown(recver.sock(), SD_BOTH);
        return;
    }

    if (recver.startAsyncRecv()!=0)
    {
        sys_perror("first WSARecv failed");
        return;
    }

    for (;;)
    {
        const unsigned start=GetTickCount();//seems only <16ms resolution

        if (con.handleConsoleInput(recver.sock())!=0)
            break;

        if (recver.recvHasUpdated())
        {
            if (recver.handleRecvResult(&con)!=0)
                break;
        }

        const unsigned elapsed = GetTickCount()-start;
        Sleep(16u>elapsed ? 16u-elapsed : 1u);
    }

    shutdown(recver.sock(), SD_BOTH);//dtor closes
}

const char* GetInitDataAfterConnect(SOCKET sock, WSAEVENT ev, InitConData& datref)
{
    WSAResetEvent(ev);//already set read event in first select along with connect?

    if (WaitForSingleObject(ev, 10*1000)!=WAIT_OBJECT_0)
        return "timed out waiting for initial server data";

    if (recv(sock, reinterpret_cast<char*>(&datref), 2, 0)!=2)
        return "receiving initial server data failed";

    if (datref.s8idx>=kMaxUsers)
        return "got bogus info from contacting supposed server";

    return nullptr;
}

const char* DoContactServer(SOCKET sock, WSAEVENT ev, InitConData& datref)
{
    sockaddr_in addr={};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr = htonl(IPV4_HOST_LONG);
    addr.sin_port=PORT_NETWORK_SHORT;

    if (WSAEventSelect(sock, ev, FD_CONNECT|FD_READ)!=0)
        return "WSAEventSelect failed";

    con_writelit("connecting to server... ");

    if (connect(sock, (sockaddr*)&addr, sizeof(sockaddr_in))==0)
    {
        con_writelit("connected immediately.\n");
        return GetInitDataAfterConnect(sock, ev, datref);
    }
    else if (WSAGetLastError()!=WSAEWOULDBLOCK)
        return "initial connect call failed";

    const DWORD r=WaitForSingleObject(ev, 10*1000);

    if (r==WAIT_OBJECT_0)//need to call getsockopt and check err?
    {
        int errcode = -1;
        socklen_t len=sizeof(int);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&errcode), &len);

        if (errcode==0)
        {
            con_writelit("connected.\n");
            return GetInitDataAfterConnect(sock, ev, datref);
        }
        else
            return reinterpret_cast<const char*>(-1);
    }
    else if (r==WAIT_TIMEOUT)
        return "timed out waiting for server response";
    else
        return "WaitForSingleObject failed";
}

InitConData contact_sever(SOCKET s)
{
    WSAEVENT const ev=WSACreateEvent();
    InitConData dat;
    dat.s8idx=InitConData::idx_failure;

    if (ev!=WSA_INVALID_EVENT)
    {
        const char *const emsg=DoContactServer(s, ev, dat);

        if (emsg==reinterpret_cast<const char*>(-1))
        {
            con_writelit("\nconnect call completed with an error, the server is probably not running.\n");
        }
        else if (emsg!=nullptr)
        {
            fputs("\nfailed to connect to server\n", stderr);
            sys_perror(emsg);
        }

        WSACloseEvent(ev);
    }
    else
    {
        sys_perror("WSACreateEventFailed");
    }
    return dat;
}

#include "console.h"

HANDLE const hin=GetStdHandle(STD_INPUT_HANDLE);
HANDLE const hout=GetStdHandle(STD_OUTPUT_HANDLE);

unsigned short gConBufWidth=80;

static const char blanks[256]={};

template<class T> T maxval(T a, T b) { return a>b ? a : b; }

void coverRemainingLine()
{

    CONSOLE_SCREEN_BUFFER_INFO info;
    DWORD dc;
    GetConsoleScreenBufferInfo(hout, &info);
    int n=gConBufWidth-info.dwCursorPosition.X;

    while (n>=256)//this sucks, don't need to erase whole row for a very large console since text length limited
    {
        WriteConsole(hout, blanks, 256, &dc, nullptr);
        n-=256;
    }
    WriteConsole(hout, blanks, n, &dc, nullptr);
}

unsigned rowsFromCurWrap(unsigned short len)
{
    return ((len+kPromptMsgOffset)/gConBufWidth)+1u;
}

unsigned colorFromIndex(unsigned idx)
{
    static_assert(kMaxUsers<=9u, "");
    const static unsigned char table[9]={10,11,14,13, 9,12, 2, 3, 4};
    return table[idx];
}

void putIdxSymbol(unsigned char idx)
{
    char c=idx+'A';
    DWORD x;
    SetConsoleTextAttribute(hout, colorFromIndex(idx));
    WriteConsole(hout, &c, 1, &x, nullptr);
    SetConsoleTextAttribute(hout, color_default);
}

void printUsersOnline(const unsigned onlinebits)
{
    char aa[2]={'\0', '\0'};
    unsigned char i=0u;
    const char nOnline=popcnt(onlinebits);
    DWORD dc;

    con_set_color();

    if (nOnline==1)
    {
        con_writelit(" [local]: you are the only one online");
    }
    else
    {
        char pre[]=" [local]: 0 users online: ";
        pre[10]+=nOnline;
        con_writelit(pre);
        do
        {
            if (onlinebits & (1u<<i))
            {
                con_set_color(colorFromIndex(i));
                aa[0]='A'+i;
            }

            WriteConsole(hout, aa, 2, &dc, nullptr);
            aa[0]='\0';
        }while (++i!=kMaxUsers);
        con_set_color();
    }

    coverRemainingLine();
}

void printPrompt(ConIo* con)
{
    static const char toall[]={26, 'a', 'l', 'l', '>'};
    static_assert((sizeof toall)==(kPromptMsgOffset-1), "");

    putIdxSymbol(con->myIdx);
    const unsigned char destIndex=con->sendBox.tag();
    if (destIndex>=tag_whisper_if_below)
    {
        con_write(toall, sizeof toall);
    }
    else//whisper
    {
        con_put(26);
        putIdxSymbol(destIndex);
        con_writelit(" w>");
    }
}

void printGlobalChatPrefix(unsigned char idx)
{
    const char aaa[3]={char(idx+'A'), ':', ' '};
    con_set_color(colorFromIndex(idx));
    con_write(aaa, 3);
    con_set_color();
}

int ConIo::printInitialUi()
{
    const char aa[2]={char(this->myIdx+'A'), '\n'};

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hout, &csbi))//I at least check this function once here
    {
        sys_perror("failed to get the console screen buffer size");
        return -1;
    }

    //check if either coords too small? I don't think any functions depend an a minimumum size except con_oldline
    if (csbi.dwSize.X<4 || csbi.dwSize.Y<12)
    {
        con_writelit("Your console screen buffer is too small,\nplease resize it and run again.\n");
        return -1;
    }

    gConBufWidth=csbi.dwSize.X;

    con_writelit("hit CTRL+Z to quit\nwelcome, your user icon is: ");
    con_set_color(colorFromIndex(myIdx));
    con_write(aa, 2);

    printUsersOnline(onlineUserBits);
    printPrompt(this);
    return 0;
}

const char* onEnterKey(ConIo* con, SOCKET sock)
{
    if (con->textLen==0u) return nullptr;//do outside? shared with bkspace

    con->sendBox.u8txtlen = con->textLen;

    if (!send_all(sock, &con->sendBox, con->textLen+2u))//make more general
        return "\nsend failed";

    const unsigned rows=rowsFromCurWrap(con->textLen);
    con_oldline(rows);

    printGlobalChatPrefix(con->myIdx);
    con_write(con->sendBox.text, con->textLen);
    coverRemainingLine();

    printUsersOnline(con->onlineUserBits);
    printPrompt(con);
    con->textLen=0;

    return nullptr;
}

void onBackSpace(ConIo* con)
{
    const static char aaa[3]={'\b','\0', '\b'};
    if (con->textLen==0u) return;//do outside? shared with enter

    --con->textLen;

    COORD cur = con_get_cursor();
    if (cur.X!=0)
        con_write(aaa, 3);
    else
    {
        cur.X=gConBufWidth-1;
        cur.Y-=1;
        con_set_cursor(cur);
        con_put('\0');
        con_set_cursor(cur);
    }
}

void onEscKey(ConIo* con)//wipe anything a user may have started to type
{
    if (!con->textLen)//check outside
        return;

    COORD start;
    start.Y = con_get_cursor().Y - (rowsFromCurWrap(con->textLen)-1);
    start.X = kPromptMsgOffset;

    con_set_cursor(start);
    con_write(blanks, con->textLen);
    con_set_cursor(start);

    con->textLen=0u;
}

const char* onConsoleResizeEvent(ConIo* con, COORD bufdim)//clear their message and set gConBufWidth
{
    (void)con;
    if (bufdim.X<4 || bufdim.Y<12)
        return "resized console buffer too small";

    //I dont think I'm going about this console stuff right.
    //instead I should store the insert Y position and make note if at bottom of buffer, where
    //writing more will bump it up.


    return "need to implement resizing";
    //return nullptr;
}

int ConIo::handleConsoleInput(SOCKET sock)
{
    INPUT_RECORD evbuf[32];
    const INPUT_RECORD *ev=evbuf;
    const char* emsg=nullptr;
    DWORD n=0;
    char ch;
    if (!GetNumberOfConsoleInputEvents(hin, &n))//or do kbhit() and getch() combo?
    {
        emsg="GetNumConsoleInputEvents";
        goto Lerror;
    }

    if (n==0)//no events
        return 0;

    n=0;
    ReadConsoleInput(hin, evbuf, 32, &n);
    if (n==0)
    {
        emsg="ReadConsoleInput";
        goto Lerror;
    }

    if (n>=32)
    {
        emsg="too many console inputs";
        goto Lerror;
    }

    do
    {
        if (ev->EventType==KEY_EVENT)
        {
            if (ev->Event.KeyEvent.bKeyDown)
            {
                ch=ev->Event.KeyEvent.uChar.AsciiChar;
                if (ch==26)
                    return -1;

                if (ch>=' ')
                {
                    if (textLen<kMaxTextLength)
                        con_put(sendBox.text[textLen++]=ch);
                }
                else if (ch=='\r')
                {
                    if ((emsg=onEnterKey(this, sock))!=nullptr)
                        goto Lerror;
                }
                else if (ch=='\b')
                    onBackSpace(this);
                else if (ch==27)//esc key? portable?
                    onEscKey(this);
            }
        }
        else if (ev->EventType==WINDOW_BUFFER_SIZE_EVENT)
        {
            if ((emsg=onConsoleResizeEvent(this, ev->Event.WindowBufferSizeEvent.dwSize))!=nullptr)
                goto Lerror;
        }

    }while (++ev, --n!=0);

    return 0;
Lerror:
    sys_perror(emsg);
    return -1;
}

int ConIo::processFullPacket(Packet* pack)
{
    const unsigned incomTextLen=pack->u8txtlen;
    const unsigned char fromIdx=pack->user();
    if (fromIdx>=kMaxUsers)
    {
        con_writelit("\ngot bogus sender index from supposed server\n");
        return -1;
    }

    con_oldline(rowsFromCurWrap(this->textLen));

    if (incomTextLen)
    {
        if (pack->tag()>=tag_whisper_if_below)//need sender idx
        {
            printGlobalChatPrefix(fromIdx);
            con_write(pack->text, incomTextLen);
        }
        else//whisper
        {

        }
    }
    else//user leave or join
    {
        con_writelit(" [server]: user ");
        putIdxSymbol(fromIdx);

        if (pack->tag()==tag_user_join)
        {
            //check not online already
            this->onlineUserBits |= 1u<<fromIdx;
            con_writelit(" joined");
        }
        else if (pack->tag()==tag_user_leave)
        {
            //check was online
            this->onlineUserBits &= ~(1u<<fromIdx);
            con_writelit(" left");
        }
    }
    coverRemainingLine();
    printUsersOnline(this->onlineUserBits);

    printPrompt(this);
    con_write(this->sendBox.text, this->textLen);

    return 0;
}

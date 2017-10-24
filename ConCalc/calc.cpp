/* A Precedence Calculator
 *
 * reads user-input lines from stdin and checks and reports all errors I could think of.
 *
    * I just wanted to write a first working version fast, so there is probably room for improvement
    *
    * Some things could be optimized/made more efficient, but just noting them and ther'e not a concern
    *  -some IO things, I think gethcar() can only return EOF if the user dares ^Z on the first character instead of checking it every time
    *  -arrays may be iterated with index references instead of local stack variables
    *
    * Theres also a global errno-ish variable, witch makes the error handling easy

    The algorithm is to have a stack of numbers and operators. Once a new operator is read, if the current top operator has equal or greater
    precedence, then it is popped and applied to the 1 or 2 topmost numbers. Then this is repeated until the top operator precedence is less than the incoming.

        while (ops.top().prec >= incomingPrec)//sentinel value so no need to check if empty
            nums.execOp(ops.pop().tag);

    Then finishing up or evaluating a subexpression is done by applying any remaining ops until a '(' is hit

        while ( (tag=ops.pop().tag) )//always a '(' because of sentinel
            nums.execOp(tag);
 */
#include "calc.h"//includes stdio

#include <cmath>//std:: pow, log, exp
#include <limits.h>//INT_MAX

struct op_t
{
    int tag, prec;
};

enum//operator tags
{
    leftparen, add, subtract, multiply, divide, negate, power, reciprocal, natlog, exponent
};

enum{oper, number};//the type of last token, held by member 'last' in Calc class
//precedence of operators
#define LPAREN op_t{}//sentinel value
#define ADD op_t{add,1}
#define SUB op_t{subtract,1}
#define MULT op_t{multiply,2}
#define DIV op_t{divide,2}
#define NEG op_t{negate,3}
#define POW op_t{power,4}
#define RECIP op_t{reciprocal, 4}
//all functions of precedence of INT_MAX
static const char *err;//having this out here is easy and works for now. its checked at the same time the end of line is checked, and set in various functions

struct NumStack
{
    enum{N=32};

    int cnt;
    double a[N];

    NumStack() : cnt(0) {}

    void scan()
    {
        if (cnt==N)
            err="exceeded number capacity";
        else if (scanf("%lf", a + cnt++)<=0)
            err="failed to read expected number";
    }

    void doRecip()
    {
        if (a[cnt-1]!=0.0)
            a[cnt-1]=1.0/a[cnt-1];
        else
            err="reciprocal of 0";
    }

    void execOp(int tag);
};

/* normally I'd implement a stack like above, where the index/pointer is one past the end,
 * but here 't' is the top element position, as It needs to be checked often.
 * there's also a sentinel at the very beginning, though its not really helping me a whole lot.
 *
 * since this is kinda goofy, Its abstracted away in this class
 */
struct OpStack
{
    enum{LastIdx=63};

    int t;
    op_t a[LastIdx+1];

    OpStack() : t(0)
    {
        a[0]=LPAREN;//sentinel
    }

    op_t top() { return a[t]; }
    op_t pop() { return a[t--]; }

    void push(op_t x)
    {
        if(t!=LastIdx)
            a[++t]=x;
        else
            err="exceeded operator capacity";
    }
};

struct Calc//things are stuffed in here to try and make run() compact
{
    int last;//either a number or operator
    int pbal;//paren balance
    OpStack ops;
    NumStack nums;

    Calc() : last(oper), pbal(0), ops(), nums() {}

    void scanNum()
    {
        nums.scan();
        last=number;
    }

    void pushLeftParen()
    {
        if (last==oper)
        {
            ops.push(LPAREN);
            ++pbal;
        }
        else
            err="expected operator before '('";
    }

    void pushFunction(int tag)//I treat these separate so nested functions are evaluated inner to outer without need for ()
    {
        if (last==oper)
            ops.push({tag, INT_MAX});
        else
            err="expected operator before function";
    }

    void pushRecip()
    {
        if (last!=oper)
        {
            while (ops.top().prec == INT_MAX)
                nums.execOp(ops.pop().tag);

            nums.doRecip();
        }
        else
            err="expected number before unary post operator";
    }

    void pushUnaryLeft(op_t op)
    {
        ops.push(op);
        last=oper;
    }

    void pushBinary(op_t op)
    {
        if (last==number)
        {
            const int incomingPrec=op.prec;

            while (ops.top().prec >= incomingPrec)//big part of algo
                nums.execOp(ops.pop().tag);

            ops.push(op);
            last=oper;
        }
        else
            err="expected number before operator";
    }

    void evalSubexpr()
    {
        int tag=ops.pop().tag;
        last=number;

        if (tag)
            do nums.execOp(tag); while ((tag=ops.pop().tag));
        else
            err="empty parenthesis";
    }

    void finish()
    {
        if (pbal)
            fputs("error - too many '('\n", stderr);
        else if (last==oper)
            fputs("error - trailing operator\n", stderr);
        else
        {
            int tag;
            while ((tag=ops.pop().tag))//always a '(' because of sentinel
                nums.execOp(tag);

            if (err)
                fprintf(stderr, "error - %s\n", err);
            else
                printf(" = %g\n", nums.a[0]);
        }
    }
};

void run(int ch)
{
    Calc calc;
    for (err=nullptr; !eol(ch) & !err; ch=getchar())
    {
        switch (ch)
        {
        case '\t': case ' ': continue;

        case '+': if (calc.last==number) { calc.pushBinary(ADD); continue; } else continue;
        case '-': if (calc.last==number) { calc.pushBinary(SUB); continue; } else { calc.pushUnaryLeft(NEG); continue; }
        case '*': calc.pushBinary(MULT); continue;
        case '/': calc.pushBinary(DIV); continue;
        case '^': calc.pushBinary(POW); continue;
        case 'R': case 'r': calc.pushRecip(); continue;

        case 'L': case 'l': calc.pushFunction(natlog); continue;
        case 'X': case 'x': calc.pushFunction(exponent); continue;

        case '(': calc.pushLeftParen(); continue;

        case ')': if (calc.pbal-- != 0) { calc.evalSubexpr(); continue; } else { fputs("error - too many ')'\n", stderr); ignore_stdin(); return; }

        case '.': if (calc.last) { fputs("error - '.' trailing after number\n", stderr); ignore_stdin(); return; }
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            ungetc(ch, stdin);
            calc.scanNum();
            continue;

        default:
            fprintf(stderr, "error - invalid character: '%c'\n", ch);
            ignore_stdin();
            return;
        }//switch
    }//loop
    if (eol(ch))
        calc.finish();
    else
    {
        fprintf(stderr, "error - %s\n", err);
        ignore_stdin();
    }
}

void NumStack::execOp(int tag)
{
    switch (tag)
    {
        case add:
            --cnt;
            a[cnt-1]+=a[cnt];
            break;

        case subtract:
            --cnt;
            a[cnt-1]-=a[cnt];
            break;

        case multiply:
            --cnt;
            a[cnt-1]*=a[cnt];
            break;

        case divide:
            if (a[--cnt]==0.0) err="division by zero";
            else a[cnt-1]/=a[cnt];
            break;

        case negate:
            a[cnt-1]= -a[cnt-1];
            break;

        case power:
            --cnt;
            a[cnt-1]=std::pow(a[cnt-1], a[cnt]);
            break;

        case natlog:
            if (a[cnt-1]<=0.0) err="logarithm argument out of domain";
            else a[cnt-1]=std::log(a[cnt-1]);
            break;

        case exponent:
            a[cnt-1]=std::exp(a[cnt-1]);
            break;
    }
}

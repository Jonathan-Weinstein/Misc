#include "calc.h"

void help()
{
    puts("\nenter an infix expression\n"
         "number format samples: 65 +77 -412 2.5 -.5 3e8\n"
         "operators grouped in increasing precedence:\n\n"

         " + add             3+2 = 5\n"
         " - subtract        3-2 = 1\n\n"

         " * multiply        4*2.5 = 8\n"
         " / divide          -3/6 = -0.5\n\n"

         " - negate          5*-2 = -10\n\n"

         " ^ power           2^3 = 8\n"
         " r reciprocal      1e2r = 0.01\n\n"

         " X raise e         X1  = 2.71828\n"
         " L natural log     LX1 = 1\n"

         "() parenthesis     2*(1+1) + X(2-1) = 6.71828");
}

int prompt()
{
    int c, tries=0;
    bool endline;

    do{
        fputs("\nConCalc>> ", stdout);
        do{
            c=getchar();
            endline=eol(c);
        }while (c<=' ' && !endline);
        if (c==EOF) ++tries;
    }while (endline && tries<4);

    return c;
}

int main(void)
{
    puts("+ - * / ( Console Calculator ) ^ r X L \n"
         "Enter 'h' for help, 'e' to exit, or an expression:");

    for (;;)
    {
        const int ch=prompt();

        switch (ch)
        {
            case 'e': case 'E':
                puts("Bye!");
                return 0;

            case 'h': case 'H':
                help();
                ignore_stdin();
                continue;

            case EOF:
                fputs("error - too many failed reads, quitting\n", stderr);
                return 1;

            default:
                run(ch);
        }//switch
    }//loop
}//main

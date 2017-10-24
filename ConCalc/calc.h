#ifndef CALC_H
#define CALC_H

#include <stdio.h>

inline
bool eol(int c)
{
    return c==EOF || c=='\n' || c==26;
}

inline
void ignore_stdin()
{
    while (!eol(getchar()));
}

void run(int ch);

#endif // CALC_H

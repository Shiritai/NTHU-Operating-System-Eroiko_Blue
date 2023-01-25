#include "syscall.h"

int
main()
{
        int n;
        for (n = 1; n < 50; ++n) {}
        Exit(3);
}

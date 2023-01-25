#include "syscall.h"

int
main()
{
	int n, i;
	for (n = 1; n < 10; ++n) {
		PrintInt(2);
		for (i=0; i<100; ++i);
	}
	Exit(2);
}

/* Does nothing but outputs to the printer and terminates */

#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

void main() {
	print(WRITETERMINAL, "printTest is ok\n");
	
	print(WRITETERMINAL, "Test number 4 is ok\n");
	
	SYSCALL(TERMINATE, 0, 0, 0);
}

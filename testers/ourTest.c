/*****************OURTEST.C***************** 
*		
*	Simple test file that counts the number of A's in a given string.
*	This file tests:
*		-WRITETERMINAL
*		-READTERMINAL
*		-TERMINATE
*
*
*
*		Written by: Tyler Charuhas and Alex Delaet
*/

#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"


void main () {

	int status;
	int i;
	int listSize = 15;
	int numOfAs = 0;
	char arr[listSize];

	print(WRITETERMINAL, "Number of As\n");
	print(WRITETERMINAL, "Enter a string: ");
		
	status = SYSCALL(READTERMINAL, (int)&arr[0], 0, 0);
	arr[status] = EOS;

	for(i = 0; i <= listSize; i++) {
		if(arr[i] == 'a' || arr[i] == 'A') {
			numOfAs++;
		}
	}	
	char c = numOfAs + '0';
	print(WRITETERMINAL, "There are ");
	if(numOfAs > 10){
		print(WRITETERMINAL, "10+");
	}
	else if(numOfAs == 10){
		print(WRITETERMINAL, "10");
	}
	else{
		print(WRITETERMINAL, &c);
	}
	print(WRITETERMINAL, " As \n");

	SYSCALL(TERMINATE, 0, 0, 0);

}


#include "syscall.h"

int main(void)
{
	// you should run fileIO_test1 first before running this one
	char test[26];
	char check[] = "abcdefghijklmnopqrstuvwxyz";
	OpenFileId fid;
	int count, success, i;
	fid = Open("file7.test");
	//MSG("fid = ");
	if (fid < 0) MSG("Failed on opening file");

	count = Read(test, 26, fid);
	if (count != 14) MSG("Failed on reading file");
	success = Close(fid);
	if (success != 1) MSG("Failed on closing file");
	for (i = 0; i < 14; ++i) {
		if (test[i] != check[12+i]) MSG("Failed: reading wrong result");
	}
	MSG("Passed! ^_^");
	Halt();
}


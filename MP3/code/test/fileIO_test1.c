#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";
	//int success= Create("file1.test");
   int success;
	OpenFileId fid;
	int i;
	//if (success != 1) MSG("Failed on creating file");
 Create("file0.test");
 Create("file1.test");
 Create("file2.test");
 Create("file3.test");
 Create("file4.test");
 Create("file5.test");
 Create("file6.test");
 Create("file7.test");
 Create("file8.test");
 Create("file9.test");
 Create("file10.test");
 
	fid = Open("file0.test");
	fid = Open("file1.test");
 fid = Open("file2.test");
 fid = Open("file3.test");
 fid = Open("file4.test");
 fid = Open("file5.test");
 fid = Open("file6.test");
 fid = Open("file7.test");
 fid = Open("file8.test");
 fid = Open("file9.test");
 fid = Open("file10.test");
 Close(6);
 for (i = 12; i < 26; ++i) {
		int count = Write(test + i, 1, 7);
		if (count != 1) MSG("Failed on writing file");
	}
  Close(15);
	if (fid < 0) MSG("Failed on opening file");
	// else MSG("Succeeded on opening file");
	
	for (i = 0; i < 26; ++i) {
		int count = Write(test + i, 1, fid);
		if (count != 1) MSG("Failed on writing file");
	}
       
	 success = Close(fid);
	 if (success != 1) MSG("Failed on closing file");
	 MSG("Success on creating file1.test");
	Halt();
}


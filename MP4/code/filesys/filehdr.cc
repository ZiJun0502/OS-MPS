// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader()
{
	numBytes = -1;
	numSectors = -1;
	memset(dataSectors, -1, sizeof(dataSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	// nothing to do now
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
	numBytes = fileSize;
	numSectors = divRoundUp(fileSize, SectorSize);

	if (freeMap->NumClear() < numSectors)
		return FALSE;
	//base case
	if(fileSize <= L0){
		for (int i = 0; i < numSectors; i++)
		{
			dataSectors[i] = freeMap->FindAndSet();
			ASSERT(dataSectors[i] >= 0);
		}
		
	}// recursive step
	else{
		int levelSize;
		if(fileSize <= L1){
			levelSize = L0;
		}else if(fileSize <= L2){
			levelSize = L1;
		}else if(fileSize <= L3){
			levelSize = L2;
		}else{
			levelSize = L3;
		}
		for(int i = 0 ; fileSize > 0 ; i++, fileSize -= levelSize){
			dataSectors[i] = freeMap->FindAndSet();
			FileHeader* child = new FileHeader();
			child->Allocate(freeMap, min(levelSize, fileSize));
			child->WriteBack(dataSectors[i]);
		}
	}
	return TRUE;
}


// bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
// {
// 	numBytes = fileSize;
// 	numSectors = divRoundUp(fileSize, SectorSize);  // need numSectors to support total 'fileSize' bytes
// 	if (freeMap->NumClear() < numSectors)
// 		return FALSE; // not enough space
// 	for (int i = 0; i < numSectors; i++)
// 	{
// 		// Allocate datablock here! One sector at a time
// 		dataSectors[i] = freeMap->FindAndSet();
// 		// since we checked that there was enough free space,
// 		// we expect this to succeed
// 		ASSERT(dataSectors[i] >= 0); // sector number should be >=0
// 	}
// 	return TRUE;
// }
//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------
// void FileHeader::Deallocate(PersistentBitmap *freeMap)
// {
// 	for (int i = 0; i < numSectors; i++)
// 	{
// 		ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
// 		freeMap->Clear((int)dataSectors[i]);
// 	}
// }
void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
	if(numBytes <= L0){
		for (int i = 0; i < numSectors; i++)
		{	
			ASSERT(freeMap->Test((int)dataSectors[i]));
			freeMap->Clear((int)dataSectors[i]);
		}
	}else{
		for (int i = 0; i < numSectors ; i++) // 要 deallocate 幾次 (幾個level)
		{
			FileHeader* child = new FileHeader();
			child->FetchFrom(dataSectors[i]);	
			child->Deallocate(freeMap); // recursive.
		}
	}
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
	kernel->synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
	kernel->synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------
// int FileHeader::ByteToSector(int offset)
// {
// 	return (dataSectors[offset / SectorSize]);
// }
int FileHeader::ByteToSector(int offset)
{
	// base case
	if(numBytes <= L0)	
		return (dataSectors[offset / SectorSize]);
	int levelSize;
	FileHeader* child = new FileHeader();
	if(numBytes <= L1){
		levelSize = L0;
	}else if(numBytes <= L2){
		levelSize = L1;
	}else if(numBytes <= L3){
		levelSize = L2;
	}else{
		levelSize = L3;
	}
	int ind = offset / levelSize;
	int off_offset = offset - ind * levelSize;
	child->FetchFrom(dataSectors[ind]);
	child->ByteToSector(off_offset);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
	int i, j, k;
	char *data = new char[SectorSize];

	printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
	for (i = 0; i < numSectors; i++)
		printf("%d ", dataSectors[i]);
	printf("\nFile contents:\n");
	
	if(numBytes <= L0){
		for (i = k = 0; i < numSectors; i++)
		{
			kernel->synchDisk->ReadSector(dataSectors[i], data);
			for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
			{
				if ('\040' <= data[j] && data[j] <= '\176') 
					printf("%c", data[j]);
				else
					printf("\\%x", (unsigned char)data[j]);
			}
			printf("\n");
		}
	}else{
		for(int i=0; i< numSectors/NumDirect; i++){
			FileHeader* child;
			child->FetchFrom(dataSectors[i]);
			child->Print();
		}
	}
	delete[] data;
	
}

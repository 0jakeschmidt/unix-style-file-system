#include "disk.h"
#include "diskmanager.h"
#include "bitvector.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "util.h"
#include <time.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;



FileSystem::FileSystem(DiskManager *dm, char fileSystemName)
{
	// this should set up the root directory.. not sure what to do if the directory already exsist, maybe use bit vector.
	myfileSystemName = fileSystemName;
	myDM = dm;
	myfileSystemSize = myDM->getPartitionSize(myfileSystemName);
	// create and set the partition manager for file system.
	myPM = new PartitionManager(myDM,myfileSystemName, myfileSystemSize);
	// check and set root directory.
	if (myPM->getFreeDiskBlock() == 1){
		//build root directory 
		char buff[1];
		buff[0] = '/';
		createDirectory(buff,1);
	}else{}

}
int FileSystem::createFile(char *filename, int fnameLen)
{	
	char name = filename[fnameLen-1];
	
	//invalid file names here
	if(filename[0] != '/' ){
		return -3;
	}
	if((name < 'a' || name > 'z') && (name < 'A' || name > 'Z')){
		return -3;
	}
	//return -1 if the file exsists
	if(searchForFile(1,filename, fnameLen) > 0){
		return -1;
	}
	// if not enough diskSpace
	int blkNum = myPM->getFreeDiskBlock();
	if(blkNum <0){
		return -2;
	}
	// all tests passed, create the file
	else
	{		
			char buff[64];
			// write the file inode to a buffer
			createBlankfile(buff,name);

			// get directory buffer to search for
			char directBuff[64];
			
			//2 cases
			if (fnameLen == 2)
			{	// it belongs in root 
				char fname = filename[fnameLen-1];
			
				// set it to be written to the directory
				int position = getFreePointer(1);
			
				// get the data in root into a buffer
				myPM->readDiskBlock(1,directBuff);
	

				// write the position of the file-I-node to the buffer
				directBuff[position] = fname; 
			
				position++;
				writeIntToBuffer(position, blkNum, directBuff);
				directBuff[position+4] = 'f';
				myPM->writeDiskBlock(1,directBuff);

		
				// actually writes the buffer to a block
				int status = myPM->writeDiskBlock(blkNum, buff);
			  	return status;
			}
			else if (fnameLen >=4)
			{
				// it belongs in different directory only need second to last pair ie. 
				// file name '/a/b/c/d' -> the directory it needs placed in is /c
				char path[fnameLen-2];

				for(int j=0; j<fnameLen-2;j++)
				{
					path[j] = filename[j];
				}
				int location = searchForFile(1,path,fnameLen-2);

				char fname = filename[fnameLen-1];
			
				
				// set it to be written to the directory
				int position = getFreePointer(location);
				// get the data in root into a buffer
				myPM->readDiskBlock(location,directBuff);
				// write the position of the file-I-node to the buffer
				directBuff[position-1] = fname; 
				writeIntToBuffer(position, blkNum, directBuff);
				directBuff[position+4] = 'f';

				myPM->writeDiskBlock(location,directBuff);
				int status = myPM->writeDiskBlock(blkNum, buff);
				return status;
			}

			
	}

}
int FileSystem::createDirectory(char *dirname, int dnameLen)
{
	char buff[64];
	int writePoint=0;
	char name = dirname[dnameLen-1];

	if(dirname[0] != '/' ){
		return -3;
	}
	//if((name < 'a' || name > 'z') && (name < 'A' || name > 'Z')){
	//	return -3;
	//}
	//return -1 if the file exsists
	if(searchForFile(1,dirname, dnameLen) > 0){
		return -1;
	}
	
	// write name to buffer
	buff[writePoint] = name;
	//incrementWritePointer
	writePoint++;
	writeIntToBuffer(writePoint,1234,buff);
	writePoint+=4;
	buff[writePoint] ='d';
	writePoint++;
	
	for(int i= 0; i<9;i++){
	//write the pointers blank
		buff[writePoint] = '#';
		writePoint++;
		writeIntToBuffer(writePoint,0,buff);
		
		// update write point for 4 bytes
		writePoint+=4;
		//write file type as z so its blank
		buff[writePoint] = 'z';
		
		writePoint++;
	}

	// write for the inode link 
	
	writeIntToBuffer(writePoint,0,buff);
	
	int blkNum = myPM->getFreeDiskBlock();
	myPM->writeDiskBlock(1, buff);


	//this handles the writing of the block to the actual disk
	/*
	if(name != '/'){
		//get block number 
		
		char directBuff[64];
		if (dnameLen == 2)
			{	// it belongs in root 
				char dname [1];
				dname[0] = '/';
				// set it to be written to the directory
				int position = getFreePointer(1);
				// get the data in root into a buffer
				myPM->readDiskBlock(1,directBuff);
				// write the position of the file-I-node to the buffer
				writeIntToBuffer1(position, blkNum, directBuff);
				directBuff[position+4] = 'd';
				myPM->writeDiskBlock(1,directBuff);

				int status = myPM->writeDiskBlock(blkNum, buff);
			  	return status;
			}
			else if (dnameLen >=4)
			{
				// it belongs in different directory only need second to last pair ie. 
				// file name '/a/b/c/d' -> the directory it needs placed in is /c
				char dname [1];
				dname[0] =  dirname[dnameLen-4];
				int dirBlock = searchForFile(1,dname,1);
				// set it to be written to the directory
				int position = getFreePointer(dirBlock);
				// get the data in root into a buffer
				myPM->readDiskBlock(dirBlock,directBuff);
				// write the position of the file-I-node to the buffer
				writeIntToBuffer1(position, blkNum, directBuff);
				directBuff[position+4] = 'd';

				myPM->writeDiskBlock(dirBlock,directBuff);
				int status = myPM->writeDiskBlock(blkNum, buff);
				return status;
			}



	}
	*/


}
int FileSystem::lockFile(char *filename, int fnameLen)
{

}
int FileSystem::unlockFile(char *filename, int fnameLen, int lockId)
{

}
int FileSystem::deleteFile(char *filename, int fnameLen)
{

}
int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{

}
int FileSystem::openFile(char *filename, int fnameLen, char mode, int lockId)
{

}
int FileSystem::closeFile(int fileDesc)
{

}
int FileSystem::readFile(int fileDesc, char *data, int len)
{
  return 0;
}
int FileSystem::writeFile(int fileDesc, char *data, int len)
{

  return 0;
}
int FileSystem::appendFile(int fileDesc, char *data, int len)
{

}
int FileSystem::seekFile(int fileDesc, int offset, int flag)
{

}
int FileSystem::renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2)
{

}
int FileSystem::getAttribute(char *filename, int fnameLen /* ... and other parameters as needed */)
{

}
int FileSystem::setAttribute(char *filename, int fnameLen /* ... and other parameters as needed */)
{

}

int FileSystem::searchForFile(int start,char *fileName, int len){
	/*
	this function will find files recursively in the directories
	for generic use call it like this 
	searchForFile(1,filename,filenameLength);
	the starting point is the directory it starts searching from, 1 == root
	returns -1 if file not found 
	else returns the block number for the file
	*/
	char buff[64];
	// everything starts at root.
	//grab root and look for directory if name is longer, else if last char search for file name.
	int nextBlock=1;

		// if name is /a/b/c/d
		//           
		char name= fileName[1];
		
		// grabbed the root;
		myPM->readDiskBlock(start,buff); 

		//look through the root for file name 
			for(int i=0; i<64; i++)
			{
				if (buff[i] == name)
				{	

					nextBlock = readIntFromBuffer(i+1, buff);
				
					if(len == 2 )
					{
						return nextBlock;
					}
					
					//shrink file name ie. /a/b/c/d -> /b/c/d
					char newName[len-2];
					for(int j=0; j<len-2; j++)
					{
						newName[j]=fileName[j+2];

					}
				
					return searchForFile(nextBlock,newName,len-2);

				}
			}
	
	return -1;
}

int FileSystem::getFreePointer(int blockNum)
{
	// takes a blk number and returns the pos of free pointers 
	char buff[64];
	myPM->readDiskBlock(blockNum, buff);
	// two cases, either a file-inode or directory i-node
	char type = buff[1];

	if(type == 'f')
	{
		for (int i = 6; i < 64; i+=4)
		{
			if (readIntFromBuffer(i,buff) == 0)
			return i;
		}
	}
		//directory
		for(int i=6; i<64; i++)
		{	
			char sentianl = buff[i];
			if(sentianl == '#')
			{
				return i;
			}
		}
}
void FileSystem::createBlankfile(char* buff, char name)
{
	int writePoint=0;
			buff[writePoint] =name; 
			writePoint++;
			// write f for file type 
			buff[writePoint] = 'f';
			writePoint++;

			// write 0 for file size 
			for (int i=0; i<4; i++){
			writeIntToBuffer(writePoint+i,0,buff);
			writePoint++;
			}

			// write 0 for direct blocks 3 times
			for (int j=0; j<3; j++)
			{
				for (int i=0; i<4; i++)
				{
				writeIntToBuffer(writePoint+i,0,buff);
				writePoint++;
				}
			}
			// write the indirect block as 0
			for (int i=0; i<4; i++)
			{
			writeIntToBuffer(writePoint+i,0,buff);
			writePoint++;
			}
}


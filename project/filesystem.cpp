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
#include <utility>
#include <assert.h>

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
  }else{
    // file has already been created
  }
}

FileSystem::~FileSystem()
{
  map<int, FileInfo*>::iterator it;
  for(it = fileInfo.begin(); it != fileInfo.end(); it++)
  {
    delete it->second->rwPointers;
    delete it->second->modes;
    delete it->second;
  }
}

FileInfo* FileSystem::_initFileInfo(int block)
{
  assert(fileInfo.find(block) == fileInfo.end());

  FileInfo* info = new FileInfo();

  info->lockId = -1;
  info->opens = 0;
  info->rwPointers = new map<int, int>();
  info->modes = new map<int, char>();

  fileInfo.insert(make_pair(block, info));

  return info;
}

int FileSystem::_getBlockFromDescriptor(int fileDesc)
{
  map<int, FileInfo*>::iterator it;
  for(it = fileInfo.begin(); it != fileInfo.end(); it++)
  {
    FileInfo* info = it->second;

    if(info->rwPointers->find(fileDesc) != info->rwPointers->end())
    {
      return it->first;
    }
  }

  return -1;
}

int FileSystem::_makeFileDescriptor(char* filename, int fnameLen, int offset)
{
  long result = time(NULL) + offset;

  int hash  = 0x811c9dc5;
  int prime = 0x1000193;

  for(int i = 0; i < fnameLen; i++)
  {
    if(filename[i] != '/')
    {
      unsigned short value = filename[i];
      hash = hash ^ value;
      hash *= prime;
    }
  }

  return static_cast<int>(result - hash);
}

FileInfo* FileSystem::_getInfoFromDescriptor(int fileDesc)
{
  map<int, FileInfo*>::iterator it;
  for(it = fileInfo.begin(); it != fileInfo.end(); it++)
  {
    FileInfo* info = it->second;

    if(info->rwPointers->find(fileDesc) != info->rwPointers->end())
    {
      return info;
    }
  }

  return NULL;
}

char FileSystem::_getModeFromDescriptor(int fileDesc)
{
  FileInfo* info = _getInfoFromDescriptor(fileDesc);

  if(info == NULL)
  {
    return '\0';
  }

  return info->modes->find(fileDesc)->second;
}

int FileSystem::_getRWFromDescriptor(int fileDesc)
{
  FileInfo* info = _getInfoFromDescriptor(fileDesc);

  if(info == NULL)
  {
    return -1;
  }

  return info->rwPointers->find(fileDesc)->second;
}

void FileSystem::_setRWFromDescriptor(int fileDesc, int rw)
{
  FileInfo* info = _getInfoFromDescriptor(fileDesc);

  // NOTE: Fails silently
  if(info == NULL)
  {
    return;
  }

  map<int, int>::iterator iter = info->rwPointers->find(fileDesc);

  iter->second = rw;
}

int FileSystem::createFile(char *filename, int fnameLen)
{  
  char name = filename[fnameLen-1];
  //invalid file names here
  int flag = validateInput(filename, fnameLen);
  if (flag != 0){
    return flag;
  }
  // if not enough diskSpace
  int blkNum = myPM->getFreeDiskBlock();
  if(blkNum <0){
    return -2;
  }
  // all tests passed, create the file
  else{    
    char buff[64];
    createBlankfile(buff,name);
    //2 cases
    if (fnameLen == 2){ 
     // it belongs in root 
      char root[1];
      root[0] = '/';
      placeInDirectory(name, blkNum,'f',root , 2);
      // actually writes the buffer to a block
      int status = myPM->writeDiskBlock(blkNum, buff);
      return status;
    }
    else if (fnameLen >=4){
      placeInDirectory(name, blkNum,'f', filename, fnameLen);
      int status = myPM->writeDiskBlock(blkNum, buff);
      return status;
    }
  }
}

int FileSystem::createDirectory(char *dirname, int dnameLen)
{
  char buff[64];
  int blknum =0;
  char name = dirname[dnameLen-1];
  int flag = validateInput(dirname,dnameLen);

  if(flag != 0)
  {
    return flag;
  }
 
  if(name == '/')
  { 
    createBlankDirectory(buff,name);
    myPM->writeDiskBlock(1, buff);
    return 0;
  }
    blknum = myPM->getFreeDiskBlock();
    if (blknum<0)
    {
      //not enough disk space
      return -2;
    }
    createBlankDirectory(buff,name);
    myPM->writeDiskBlock(blknum, buff);
    placeInDirectory(name,blknum, 'd', dirname, dnameLen);
    return 0;
}

// Errors: 
// -1 -> The file is already locked
// -2 -> The file does not exist
// -3 -> The file is open
// -4 -> Others
int FileSystem::lockFile(char *filename, int fnameLen)
{
  int block = searchForFile(1, filename, fnameLen);

  // The requested file does not exist
  if(block == -1)
  {
    return -2;
  }

  FileInfo* info;

  // If the file info table does not have info on this block
  if(fileInfo.find(block) == fileInfo.end())
  {
    info = _initFileInfo(block);
  }
  else
  {
    info = fileInfo[block];

    // The file is currently open somewhere
    if(info->opens != 0)
    {
      return -3;
    }

    // The file is already locked
    if(info->lockId != -1)
    {
      return -1;
    }
  }

  // Not sure if this should always be 1
  info->lockId = 1;

  return info->lockId;
}

// 0  -> Success
// -1 -> Lock ID is invalid
// -2 -> Anything else
int FileSystem::unlockFile(char *filename, int fnameLen, int lockId)
{
  int block = searchForFile(1, filename, fnameLen);

  if(block != -1)
  {
    if(fileInfo.find(block) != fileInfo.end())
    {
      FileInfo* info = fileInfo[block];

      if(lockId != -1 && lockId == info->lockId)
      {
        info->lockId = -1;
        return 0;
      }

      return -1;
    }
  }

  return -2;
}
/*
-1 if the file does not exist, 
-2 if the file is in use or locked,
-3if the file cannot be deleted for any other reason, and 
0 if the file is deleted successfully.
*/
int FileSystem::deleteFile(char *filename, int fnameLen)
{

  int block = validateInput(filename,fnameLen);
  if(block ==-3)
  {
    // filename wasnt vaild
    return block;
  }
  block = searchForFile(1,filename,fnameLen);
  if(block == -1)
  {
    return -1;
  }
  FileInfo* info;
 
  // opened or locked 
  if(fileInfo.find(block) == fileInfo.end())
  {
    info = _initFileInfo(block);
  }
  info = fileInfo[block];

  if(info->lockId != -1)
  {
      return -2; // locked 
  }

  if(info->opens > 0)
  {
    return -2;// file is opened
  }
  
  //rest bitvector for files 
  resetFilePointers(block);
  myPM->returnDiskBlock(block);

  fileInfo.erase(fileInfo.find(block));
  
  char buff[64];

  if(fnameLen==2)
  {
    // if the file is in root 
    myPM->readDiskBlock(1,buff);
    //printBuffer(buff,64);
    int namePosition = searchDirectory(1,filename[fnameLen-1]);
    buff[namePosition] = '#';
    namePosition++;
    writeIntToBuffer(namePosition,0,buff);
    
    // update write point for 4 bytes
    namePosition+=4;
    //write file type as z so its blank
    buff[namePosition] = 'z';
   // printBuffer(buff,64);
    myPM->writeDiskBlock(1,buff);

    return 0;
  }
  else
  {
    // if the file is not in root
    char subdirc[fnameLen-2];
    for(int i = 0; i < fnameLen-2; ++i)
    {
      subdirc[i] = filename[i];
    }
    int blk =searchForFile(1,subdirc,fnameLen-2);
    myPM->readDiskBlock(blk,buff);
    int namePosition = searchDirectory(blk,filename[fnameLen-1]);
    buff[namePosition] = '#';
    namePosition++;
    writeIntToBuffer(namePosition,0,buff);
    
    // update write point for 4 bytes
    namePosition+=4;
    //write file type as z so its blank
    buff[namePosition] = 'z';
   // printBuffer(buff,64);
    myPM->writeDiskBlock(blk,buff);
    return 0;
  }
  
    


}

int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{
  //1. find the blk number 
  //2. reset the bit vector 
  //3. find the previous directory, which we have the blk num for, its the buff[1]
  //4. delete entry from previous directory
  //5. go to all of directories memory and replace with #s
}

// >0 -> Success
// -1 -> File doesn't exist
// -2 -> Invalid mode
// -3 -> Locking restrictions
// -4 -> Any other error
int FileSystem::openFile(char *filename, int fnameLen, char mode, int lockId)
{
  // Invalid mode
  if(mode != 'r' && mode != 'w' && mode != 'm')
  {
    return -2;
  }

  int block = searchForFile(1, filename, fnameLen);

  // File doesn't exist
  if(block == -1)
  {
    return -1;
  }

  FileInfo* info;

  if(fileInfo.find(block) == fileInfo.end())
  {
    info = _initFileInfo(block);
  }
  else
  {
    info = fileInfo[block];

    if(info->lockId != lockId)
    {
      return -3;
    }
  }

  info->opens++;

  int fd = _makeFileDescriptor(filename, fnameLen, info->opens);

  info->modes->insert(make_pair(fd, mode));
  info->rwPointers->insert(make_pair(fd, 0));

  return fd;
}

// 0  -> Success
// -1 -> Invalid desc
// -2 -> Other errors
int FileSystem::closeFile(int fileDesc)
{
  FileInfo* info = _getInfoFromDescriptor(fileDesc);

  if(info != NULL)
  {
    if(info->opens > 0)
    {
      // These three things are the actual action of "closing" a file
      info->opens--;
      info->rwPointers->erase(fileDesc);
      info->modes->erase(fileDesc);

      return 0;
    }
  }
  else
  {
    return -1;
  }

  return -2;
}

int FileSystem::readFile(int fileDesc, char *data, int len)
{ 
  int block = _getBlockFromDescriptor(fileDesc);

  if(len < 0) return -2;
  int rwPointer = _getRWFromDescriptor(fileDesc);
  FileInfo* info = _getInfoFromDescriptor(fileDesc);
  if(info != NULL){
    char buff[64];
    myPM->readDiskBlock(block, buff);
    for(int i = 0; i <= len; i++){
      data[i] = buff[rwPointer + i];
    }
  }else{
    return -1;
  }
  return -3;
}
int FileSystem::writeFile(int fileDesc, char *data, int len)
{

  return 0;
}
int FileSystem::appendFile(int fileDesc, char *data, int len)
{

}

/*
* -2 if offset will take you out of bounds of file 
* -1 if fileDesc, offset, or flag is invalid
*  0 if successful
*/
int FileSystem::seekFile(int fileDesc, int offset, int flag)
{
  FileInfo* info = _getInfoFromDescriptor(fileDesc);
  if(info == NULL || (offset < 0 && !(flag != 0)) || flag < 0){
    return -1;
  }
  if(flag == 0){
    int currentRW = _getRWFromDescriptor(fileDesc);
    //are all files size 64?
    if(currentRW + offset > 64 || currentRW + offset < 0){
      return -2;
    }else{
      _setRWFromDescriptor(fileDesc, currentRW + offset);
    }
  }else{
    //set to byte number offest in the file
    _setRWFromDescriptor(fileDesc, offset);
  }
  return 0;
}
int FileSystem::renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2)
{
    /*
 -1 invalid filename,  !
 -2 if the file does not exist, ! 
 -3 if there already exists a file whose name is the same as the name pointed to by fname2, !
 -4 if file is opened or locked, 
 -5 for any other reason, 
  0 if successful. 
   */


  int testValue = validateInput(filename2,fnameLen2);
  
  if (testValue == -3 || testValue == -1)
  {
    if(testValue ==-3)
    {
      return -1;
    }
    else{
      return -3;
    }
  }
  //-3 bad name
  // -1 then already pointed to by another file



  testValue = searchForFile(1, filename1, fnameLen1);
  if (testValue < 0)
  {
    return -2;
  }

  // if >0 then file1 doesnt exist

  FileInfo* info;
 

   if(fileInfo.find(testValue) == fileInfo.end())
  {
    info = _initFileInfo(testValue);
  }
  info = fileInfo[testValue];

  if(info->lockId != -1)
  {
    
      return -4; // locked 
  }

  if(info->opens > 0)
  {
    return -4;// file is opened
  }

  char buff[64];

  myPM->readDiskBlock(testValue,buff);
  //change the name in the file
  buff[0] =filename2[fnameLen2-1];
  myPM->writeDiskBlock(testValue,buff);

  // rewrite the name in the files directory 

  if(fnameLen1==2)
  {
    // if the file is in root 
    myPM->readDiskBlock(1,buff);
    //printBuffer(buff,64);
    int namePosition = searchDirectory(1,filename1[fnameLen1-1]);
    buff[namePosition] =filename2[fnameLen2-1];
    //printBuffer(buff,64);
    myPM->writeDiskBlock(1,buff);
    return 0;
  }
  else
  {
    // if the file is not in root
    char subdirc[fnameLen1-2];
    for(int i = 0; i < fnameLen1-2; ++i)
    {
      subdirc[i] = filename1[i];
    }
    int blk =searchForFile(1,subdirc,fnameLen1-2);
    myPM->readDiskBlock(blk,buff);
    int namePosition = searchDirectory(blk,filename1[fnameLen1-1]);
    buff[namePosition] =filename2[fnameLen2-1];
   // printBuffer(buff,64);
    myPM->writeDiskBlock(blk,buff);
    return 0;
  }

  return -5;

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
  int nextBlock=1;

  // Ensure that the first character is always a / for the root directory
  if(fileName[0] != '/')
  {
    return -1;
  }          
    char name = fileName[1];
    
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
      writeIntToBuffer(writePoint,0,buff);
      writePoint+=4;

      // write 0 for direct blocks 3 times
    for (int j=0; j<3; j++)
    {
      
        writeIntToBuffer(writePoint,0,buff);
        writePoint+=4; 
    }
    // write the indirect block as 0
      writeIntToBuffer(writePoint,0,buff);
      writePoint+=4;
    
}
void FileSystem::createBlankDirectory(char* buff,char name)
{
  int writePoint=0;
  buff[writePoint] = name;
  
  //incrementWritePointer
  writePoint++;
  writeIntToBuffer(writePoint,1,buff);
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
}

void FileSystem::placeInDirectory(char name, int blkNum, char type, char* subDirectoryName, int subdirecNameLen)
{
  // pass this the name of the file
  // the blknum for the i-node, 
  //type, so 'd' or 'f'
  // and name of sub directory, so /a/b/c/d -> pass it c and this places d into c
  char directBuff[64];

  if(subdirecNameLen == 2)
  {
    // set it to be written to the directory
    int position = getFreePointer(1);
    
    // get the data in root into a buffer
    myPM->readDiskBlock(1,directBuff);
    // write the position of the file-I-node to the buffer
    directBuff[position] = name; 
    position++;
    writeIntToBuffer(position, blkNum, directBuff);
    directBuff[position+4] = type;
    
    myPM->writeDiskBlock(1,directBuff);
  }
  else
  {
    // it belongs in different directory 
    // file name '/a/b/c/d' -> the directory it needs placed in is /c
    char sub[subdirecNameLen-2];
    for (int i = 0; i < subdirecNameLen-2; ++i)
    {
      sub[i] = subDirectoryName[i];
    }
    int direcNum = searchForFile(1,sub,subdirecNameLen-2);
    
    int position = getFreePointer(direcNum);
    // get the data in root into a buffer
    myPM->readDiskBlock(direcNum,directBuff);
    // write the position of the file-I-node to the buffer
    directBuff[position] = name; 
    position++;
    writeIntToBuffer(position, blkNum, directBuff);
    directBuff[position+4] = type;
  
    myPM->writeDiskBlock(direcNum,directBuff);    
  }
}

/*
return -3 == bad name
return -1 == file already exsists
return 0 == good name 
*/
int FileSystem::validateInput(char* name, int nameLen)
{
  // this function validates input for file names
  int valid=0;
  if(name[0] != '/' ){
    return -3;
  }
  for (int i = 0; i < nameLen; ++i)
  {
    if(i%2!=0)
    {
      if((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'z'))
      {
        valid++;
      }
    }else if(name[i] == '/')
    {
      valid++;
    }

  }

  if (valid != nameLen)
  {
    return -3;
  }

  if(searchForFile(1,name, nameLen) > 0){
    //if file exists return -1
    return -1;
  }
  return 0;
}

int FileSystem::searchDirectory(int blknum,char name)
{
  char buff[64];

  myPM->readDiskBlock(blknum,buff);

  for (int i = 0; i < 64; ++i)
  {
    if(buff[i] == name)
    {
      
      return i;
    }
  }
  return -1;
}

void FileSystem::resetFilePointers(int block)
{
  // takes a blockNumber and resets all of its pointer blocks in the bitvector
  char buff[64];
  myPM->readDiskBlock(block,buff);

  for (int i = 6; i < 18; i+=4)
  {
      int pointer = readIntFromBuffer(i,buff);
      if(pointer>0){
        myPM->returnDiskBlock(pointer);
      }
      
  }
    //TODO check for idisk block to more pointers
}
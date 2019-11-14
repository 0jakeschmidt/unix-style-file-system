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
    {  // it belongs in root 

      char root[1];
      root[0] = '/';
      placeInDirectory(name, blkNum,'f',root , 2);
      // actually writes the buffer to a block
      int status = myPM->writeDiskBlock(blkNum, buff);
        return status;
    }
    else if (fnameLen >=4)
    {
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

  if(dirname[0] != '/' )
  {
    return -3;
  }
  //if((name < 'a' || name > 'z') && (name < 'A' || name > 'Z') && (name != '/'))
  //{
//    return -3;
//  }
  //return -1 if the file exsists
  if(searchForFile(1,dirname, dnameLen) > 0)
  {
    return -1;
  }
  // write name to buffer  
    
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
  //this handles the writing of the block to the actual disk
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

int FileSystem::deleteFile(char *filename, int fnameLen)
{
  // TODO: Make sure that file deletion removes the entry from the `fileInfo`
  //       map because the data will stay there and then could end up associated
  //       with a block that has something completely different in it
}

int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{

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

  // Ensure that the first character is always a / for the root directory
  if(fileName[0] != '/')
  {
    return -1;
  }

    // if name is /a/b/c/d
    //           
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

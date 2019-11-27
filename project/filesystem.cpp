#include "disk.h"
#include "diskmanager.h"
#include "bitvector.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "util.h"
#include <vector>
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
  if(searchForFile(1,filename,fnameLen) >0){
    return -1;
  }
  // if not enough diskSpace
  int blkNum = myPM->getFreeDiskBlock();
  if(blkNum <0){
    return -2;
  }
  // all tests passed, create the file
  else{    
    char buff[64];
    //createBlankfile(buff,name);
    createBlankfile(blkNum,name);
    //2 cases
    if (fnameLen == 2){ 
     // it belongs in root 
      char root[1];
      root[0] = '/';
      placeInDirectory(name, blkNum,'f',root , 2);
      // actually writes the buffer to a block
      int status = 0;
      return status;
    }
    else if (fnameLen >=4){
      placeInDirectory(name, blkNum,'f', filename, fnameLen);
      int status = 0;
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
  //TODO this might have an error, due to search only looking at the last part of the name as a file not a directory
  if(searchForFile(1,dirname,dnameLen) > 0){
    return -1;
  }
 
  if(name == '/')
  { 
    createBlankDirectory(1);
    return 0;
  }
    blknum = myPM->getFreeDiskBlock();
    if (blknum<0)
    {
      //not enough disk space
      return -2;
    }
    createBlankDirectory(blknum);
    
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
    
    int directNum =1;
    int namePosition = getFilePosInDirectory(directNum,filename[fnameLen-1]);
    myPM->readDiskBlock(directNum,buff);
    buff[namePosition] = '#';
    namePosition++;
    writeIntToBuffer(namePosition,0,buff);
    
    // update write point for 4 bytes
    namePosition+=4;
    //write file type as z so its blank
    buff[namePosition] = '#';
   
    myPM->writeDiskBlock(directNum,buff);

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
    
    int namePosition = getFilePosInDirectory(blk,filename[fnameLen-1]);
    myPM->readDiskBlock(blk,buff);
    buff[namePosition] = '#';
    namePosition++;
    writeIntToBuffer(namePosition,0,buff);
    
    // update write point for 4 bytes
    namePosition+=4;
    //write file type as z so its blank
    buff[namePosition] = '#';
   
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
  char buffer[64];
  myPM->readDiskBlock(1,buffer);

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

/*
* -1 if file is null
* -2 if len is < 0
* -3 if action is not permitted
*/
int FileSystem::readFile(int fileDesc, char *data, int len){ 
  FileInfo* info = _getInfoFromDescriptor(fileDesc);
  char mode = _getModeFromDescriptor(fileDesc);
  int rwPointer = _getRWFromDescriptor(fileDesc);
  int blockNum = _getBlockFromDescriptor(fileDesc);
  int fileSize = getFileSize(blockNum);
  if(len < 0) return -2;
  if(info == NULL) return -1;
  if(info->lockId != -1 || mode == 'w') return -3;
  if(rwPointer >= fileSize) return 0;

  vector<int> blocks;
  getFileDataPointers(blockNum, blocks);
  if(blocks.size() <= 0) return 0;
  int startBlock = rwPointer / 64;
  int endBlock = (len + rwPointer) / 64;
  int currentBlock = startBlock;
  int dataLen = 0;
  int returnVal = len;
  bool hasOverflow = false;
  endBlock = blocks.size() -1 < endBlock ? blocks.size() -1 : endBlock;
  for(;currentBlock<=endBlock; currentBlock++){
    if(hasOverflow) break;
    int readLocation = 0;
    char buff[64];
    if(currentBlock == startBlock){
      readLocation = rwPointer % 64; 
    }
    myPM->readDiskBlock(blocks.at(currentBlock), buff);
    for(; readLocation<64;readLocation++){
      if(dataLen + rwPointer >= fileSize || dataLen == len){
        returnVal =  dataLen;
        hasOverflow = true;
        break;
      };
      data[dataLen++] = buff[readLocation];
    }
  }

  _setRWFromDescriptor(fileDesc, rwPointer + dataLen);
  return returnVal;
}

int FileSystem::writeFile(int fileDesc, char *data, int len)
{
  if(len < 0) return -2;
  FileInfo* info = _getInfoFromDescriptor(fileDesc);
  int block = _getBlockFromDescriptor(fileDesc);
  int rwPointer = _getRWFromDescriptor(fileDesc);
  vector<int> blocks;
  getFileDataPointers(block, blocks);
  int startBlock = rwPointer / 64;
  int returnVal = -1;
  char mode = _getModeFromDescriptor(fileDesc);
  int fileSize = getFileSize(block);
  if(info != NULL ) {
    if(info->lockId != -1 || mode == 'r') return -3;
    int currentBlock = startBlock;
    bool isOutOfBounds = false;
    for(int i = 0; i < len; i+=64){
      if(isOutOfBounds){
        break;
      }
      char buff[64];
      if(currentBlock >= blocks.size()){
        int diskBlock = myPM->getFreeDiskBlock();
        if(diskBlock != -1){
          blocks.push_back(diskBlock);
        }else{
          return -3;
        }
      }
      int dataBlock = blocks.at(currentBlock);
      //possible error below
      int startNode = currentBlock == startBlock ? currentBlock % 64 : 0; 
      // possible error above

      for(int k = startNode; k < 64; k++){
        if(k+i >= len) {
          isOutOfBounds = true;
          returnVal = k+i;
        }else{
          buff[k] = data[k+i];
        }
      }
      myPM->writeDiskBlock(dataBlock, buff);
      currentBlock++;
    }
    setFileDataPointers(block, blocks);
    if(rwPointer + len >= fileSize){
      setFileSize(block, len + rwPointer);
    }
    _setRWFromDescriptor(fileDesc, rwPointer + len);
    returnVal = len;
  }

  return returnVal;
}
int FileSystem::appendFile(int fileDesc, char *data, int len)
{
  int blockNum = _getBlockFromDescriptor(fileDesc);
  int fileSize = getFileSize(blockNum);
  int oldRwPointer = _getRWFromDescriptor(fileDesc);
  _setRWFromDescriptor(fileDesc, fileSize);
  int wFile = writeFile(fileDesc, data, len);
  if(wFile < 0){
    _setRWFromDescriptor(fileDesc, oldRwPointer);
  }
  return wFile;
}

/*
* -2 if offset will take you out of bounds of file 
* -1 if fileDesc, offset, or flag is invalid
*  0 if successful
*/
int FileSystem::seekFile(int fileDesc, int offset, int flag)
{
  FileInfo* info = _getInfoFromDescriptor(fileDesc);
  if(info == NULL || (offset < 0 && flag != 0)){
    return -1;
  }
  int currentRW = _getRWFromDescriptor(fileDesc);
  int blockNum = _getBlockFromDescriptor(fileDesc);
  int fileSize = getFileSize(blockNum);
  if(flag != 0){
    if(offset >= fileSize){
      return -2;
    }
    _setRWFromDescriptor(fileDesc, offset);
  }else{
    int newRw = currentRW + offset;
    if( newRw >= fileSize || newRw < 0){
      return -2;
    }
    _setRWFromDescriptor(fileDesc, newRw);
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
  
  if (testValue == -3 )
  {
    return -1;
  }
    testValue = searchForFile(1, filename2, fnameLen2);
  if (testValue > 0)
  {
    return -3;
  }

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

  if(info->opens >0)
  {
    return -4;// file is opened
  }

  char buff[64];

  setFileName(testValue,filename2[fnameLen2-1]);
  

  if(fnameLen1==2)
  {
    // if the file is in root 
    
    int directNum =1;
    int namePosition = getFilePosInDirectory(directNum, filename1[fnameLen1-1]);
    myPM->readDiskBlock(directNum,buff);
    buff[namePosition] =filename2[fnameLen2-1];
   
    myPM->writeDiskBlock(directNum,buff);
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
    int namePosition = getFilePosInDirectory(blk, filename1[fnameLen1-1]);
    myPM->readDiskBlock(blk,buff);
    buff[namePosition] =filename2[fnameLen2-1];
  
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

  if(validateInput(fileName,len) ==-3){

    return -1;
  }
  char name = fileName[len-1];
  if(len == 2){

    return getFilefromDirec(start,name);
  }
  else if(len>2){
    name = fileName[1];
    int point =getDirecfromDirec(start,name);
    if(point == -1){
      return -1;
    }
    char newName[len-2];
    for(int j=0; j<len-2; j++)
    {
      newName[j]=fileName[j+2];
    }
      return  searchForFile(point,newName,len-2);
  }
  return-1;

}

int FileSystem::getFreePointerDirectory(int &blockNum)
{
  // takes a blk number and returns the pos of free pointers 
  char buff[64];
  myPM->readDiskBlock(blockNum, buff);
  
    //directory
    for(int i=0; i<60; i++)
    {  
      char sentianl = buff[i];
      if(sentianl == '#')
      {
        return i;
      }
    }
    int indirect = getDirecIndirect(blockNum);
    if(getDirecIndirect(blockNum) >0)
    {
     blockNum = indirect; 
     return getFreePointerDirectory(blockNum);
    }
    else{
      setDirecIndirect(blockNum);
      indirect = getDirecIndirect(blockNum);
      blockNum = indirect;

      return getFreePointerDirectory(blockNum);
    }
}
void FileSystem::createBlankfile(int block, char name)
{
    setFileName(block,name);

    setFileType(block,'f');

    setFileSize(block,0);
  
    vector<int> pointers;
    for (int i = 0; i < 3; ++i)
    {
      pointers.push_back(0);
    }
   
    setFileDataPointers(block, pointers);
    setFileIndirect(block,0);
    
}
void FileSystem::createBlankDirectory(int block)
{
  char buff[64];
  myPM-> readDiskBlock(block, buff);
  for (int i = 0; i < 64; ++i)
  {
    buff[i] ='#';
  }
  myPM->writeDiskBlock(block,buff);

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
    int one=1;
  
    int position = getFreePointerDirectory(one);
  
    // get the data in root into a buffer
    myPM->readDiskBlock(one,directBuff);
    // write the position of the file-I-node to the buffer
    directBuff[position] = name; 
    position++;
    writeIntToBuffer(position, blkNum, directBuff);
    directBuff[position+4] = type;
    
    myPM->writeDiskBlock(one,directBuff);
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
    
    int position = getFreePointerDirectory(direcNum);
   
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

  return 0;
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

//takes a disk blknumber, returns that files name 
char FileSystem::getFileName(int block){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    return buff[0];
}
// takes block number and a char for a name, writes it to that block
void FileSystem::setFileName(int block, char name){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    buff[0] = name;
    myPM->writeDiskBlock(block,buff);
}
//takes a disk blknumber, returns that files type
char FileSystem::getFileType(int block){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    return buff[1];
}
// takes block number and a char for a name, writes it to that block
void FileSystem::setFileType(int block, char type){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    buff[1] = type;
    myPM->writeDiskBlock(block,buff);
}
//gets and the file size takes a block number
int FileSystem::getFileSize(int block){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    int size = readIntFromBuffer(2,buff);
    return size;
}
// takes block number and a char for a name, writes it to that block
void FileSystem::setFileSize(int block, int size){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    writeIntToBuffer(2,size, buff);
    myPM->writeDiskBlock(block,buff);
}
//gets the block number for indirect inode. takes the file i-node block number for a parameter 
int FileSystem::getFileIndirect(int block){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    int inode = readIntFromBuffer(18,buff);
    return inode;
}
// takes block number and a char for a name, writes it to that block
void FileSystem::setFileIndirect(int block, int blknumIndirect){
    char buff[64];
    myPM->readDiskBlock(block,buff);
    writeIntToBuffer(18,blknumIndirect, buff);
    myPM->writeDiskBlock(block,buff);
}

// needs a blank int vector passed by reference, will fill the vector with block numbers 
// and a block number
void FileSystem::getFileDataPointers(int block, vector<int> &pointers){
    pointers.clear();
    char buff[64];
    myPM->readDiskBlock(block,buff);
    int readPoint= 6;
    //get first three pointers 
    for (int i = 0; i < 3; ++i)
    { 
      int dataBlock = readIntFromBuffer(readPoint,buff);
      readPoint+=4;
      if(dataBlock != 0)
      {
        pointers.push_back(dataBlock);
      }
    }
    //get file i-node if needed 
    int inode = getFileIndirect(block);
    if(inode!= 0)
    {
      myPM->readDiskBlock(inode,buff);
      readPoint =0;
      for (int i = 0; i < 16; ++i)
      { 
      int dataBlock = readIntFromBuffer(readPoint,buff);
      readPoint+=4;
      if(dataBlock != 0)
      {
        pointers.push_back(dataBlock);
      }
    }

    }
}

// pass in a vector filled with blk numbers for the data blocks, and the file inode block.
// this function handles inode blocks for you.
void FileSystem::setFileDataPointers(int block, vector<int> &pointers){
    char buff[64];
    myPM->readDiskBlock(block,buff);

    int readPoint= 6;
    //set first three pointers 
    for (int i = 0; i < pointers.size() && i < 3; ++i)
    { 
      
      int dataBlock = pointers.at(i);
      writeIntToBuffer(readPoint,dataBlock,buff);
      readPoint+=4;
      
    }
    myPM->writeDiskBlock(block,buff);

    if(pointers.size() >=4)
    {
      int indirect = getFileIndirect(block);
        if(indirect ==0)
        {
         indirect = myPM->getFreeDiskBlock();
         setFileIndirect(block,indirect);
       }

      myPM->readDiskBlock(indirect,buff);
      readPoint =0;
      for (int i = 3; i < pointers.size(); ++i)
      {
        int dataBlock = pointers.at(i);
        writeIntToBuffer(readPoint,dataBlock,buff);
        readPoint+=4;
      }
      myPM->writeDiskBlock(indirect,buff);
    }

}

void FileSystem::setDirecIndirect(int block)
{
  char buff[64];
  int pointer = myPM->getFreeDiskBlock();
  myPM->readDiskBlock(block,buff);

  writeIntToBuffer(60,pointer,buff);
  myPM->writeDiskBlock(block,buff);

  
}

int FileSystem::getDirecIndirect(int block)
{
  char buff[64];
  myPM->readDiskBlock(block,buff);
  if(buff[60] =='#'){
    return -1;
  }
  int point = readIntFromBuffer(60,buff);
  return point;
}

int FileSystem::getDirecfromDirec(int block,char name)
{
  char buff[64];
  myPM->readDiskBlock(block,buff);
  int point =-1;
  for (int i = 5; i < 64; i+=6)
  {
    if(buff[i]== 'd' && buff[i-5] == name)
      {
        point = readIntFromBuffer((i-4),buff);
        return point;
      }
  }
  int indirect = getDirecIndirect(block);
  if(indirect> 0)
  {
    myPM->readDiskBlock(indirect,buff);
     for (int i = 5; i < 64; i+=6)
    {
      if(buff[i]== 'd' && buff[i-5] == name)
      {
        point = readIntFromBuffer((i-4),buff);
        return point;
      }
    }
  }
  return -1;
}
//returns pointer to file 
int FileSystem::getFilefromDirec(int block, char name)
{
  
  char buff[64];
  myPM->readDiskBlock(block,buff);
  int point =-1;
  for (int i = 5; i < 64; i+=6)
  {
    if(buff[i]== 'f' && buff[i-5] == name)
      {
        point = readIntFromBuffer((i-4),buff);
        return point;
      }
  }
  int indirect = getDirecIndirect(block);
  if(indirect> 0)
  {
    myPM->readDiskBlock(indirect,buff);
     for (int i = 5; i < 64; i+=6)
    {
      if(buff[i]== 'f' && buff[i-5] == name)
      {
        point = readIntFromBuffer((i-4),buff);
        return point;
      }
    }
  }
  return -1;
}
// returns position in the file a directory 
int FileSystem::getFilePosInDirectory(int &block, char name)
{
  
  char buff[64];
  myPM->readDiskBlock(block,buff);

  int point =-1;
  for (int i = 5; i < 64; i+=6)
  {
    if(buff[i]== 'f' && buff[i-5] == name)
      {
        
        return i-5;
      }
  }
  int indirect = getDirecIndirect(block);
  if(indirect> 0)
  {
    myPM->readDiskBlock(indirect,buff);
   
  
     for (int i = 5; i < 64; i+=6)
    {
      if(buff[i]== 'f' && buff[i-5] == name)
      {
        block = indirect;
        
        return i-5;
      }
    }
  }
  return -1;
}


#include "disk.h"
#include "diskmanager.h"
#include "bitvector.h"
#include "partitionmanager.h"
#include <iostream>

using namespace std;


PartitionManager::PartitionManager(DiskManager *dm, char partitionname, int partitionsize)
{
  myDM = dm;
  myPartitionName = partitionname;
  char buff[64];
  myPartitionSize = myDM->getPartitionSize(myPartitionName);


  /* If needed, initialize bit vector to keep track of free and allocted
     blocks in this partition */
  myDM->readDiskBlock(partitionname, 0, buff);
  myBV = new BitVector(partitionsize);
  if(buff[0] == '#'){
    myBV->setBit(0);
    myBV->setBit(1);
    _writeBitVector();
  }else {
    myBV->setBitVector((unsigned int*) buff);
  }
  
}

PartitionManager::~PartitionManager()
{
}

int PartitionManager::_writeBitVector()
{
  char buffer[64];
  myBV->getBitVector((unsigned int*) buffer);
  return myDM->writeDiskBlock(myPartitionName, 0, buffer);
}

/*
 * return blocknum, -1 otherwise
 */
int PartitionManager::getFreeDiskBlock()
{
  int blocknum = -1;
  /* write the code for allocating a partition block */
  for(int i = 0; i < myPartitionSize; i++){
    if(myBV->testBit(i) == 0){
      blocknum = i;
      break;
    };
  }
  return blocknum;
}

/*
 * return 0 for sucess, -1 otherwise
 */
int PartitionManager::returnDiskBlock(int blknum)
{
  /* write the code for deallocating a partition block */
  myBV->resetBit(blknum);
  return _writeBitVector() < 0 ? -1 : 0;
 }


int PartitionManager::readDiskBlock(int blknum, char *blkdata)
{
  return myDM->readDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::writeDiskBlock(int blknum, char *blkdata)
{
  return myDM->writeDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::getBlockSize() 
{
  return myDM->getBlockSize();
}

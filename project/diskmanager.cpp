#include "disk.h"
#include "diskmanager.h"
#include "util.h"

#include <iostream>

using namespace std;

DiskManager::DiskManager(Disk *d, int partcount, DiskPartition *dp)
{
  myDisk = d;
  partCount = partcount;
  int r = myDisk->initDisk();
  char buffer[64]; 

  // Initialize the buffer to 64 '#'s
  for(int i = 0; i < 64; i++)
  {
    buffer[i] = '#';
  }

  diskP = new DiskPartition[partCount];

  // r == 1 indicates that a new disk file was created
  if(r == 1)
  {
    // Write the number of partitions to the disk's superblock buffer
    writeIntToBuffer(0, partCount, buffer);

    for(int i = 0; i < partCount; i++)
    {
      diskP[i].partitionName = dp[i].partitionName;
      diskP[i].partitionSize = dp[i].partitionSize;

      // The 4 is to account for the partCount already in the buffer
      // Each partition name is 1 character and each partition size
      // is 4 characters so we offset by (i*5)

      // Write the single character partition name to the buffer
      buffer[4 + (i * 5)] = diskP[i].partitionName;

      // Write the partition size to the buffer
      writeIntToBuffer(4 + ((i * 5) + 1), diskP[i].partitionSize, buffer);
    }

    myDisk->writeDiskBlock(0, buffer);
  }
  else if(r == 0)
  {
    // Populate buffer from disk file
    myDisk->readDiskBlock(0, buffer);

    // Read the partition count from the first 4 charactets of the buffer
    partCount = readIntFromBuffer(0, buffer);

    for(int i = 0; i < partCount; i++)
    {
      // Using the same offsets as when writing the superblock
      diskP[i].partitionName = buffer[4 + (i * 5)];
      diskP[i].partitionSize = readIntFromBuffer(4 + ((i * 5) + 1), buffer);
    }
  }
}

/*
 *   returns: 
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char *blkdata)
{
  int absoluteBlock = getAbsoluteBlock(diskP, partCount, partitionname, blknum);

  // Indicates that the partition doesn't exist
  if(absoluteBlock == -1)
  {
    return -3;
  }

  int diskRead = myDisk->readDiskBlock(absoluteBlock, blkdata);

  if(diskRead != 0)
  {
    return diskRead;
  }

  return 0;
}


/*
 *   returns: 
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::writeDiskBlock(char partitionname, int blknum, char *blkdata)
{
  int absoluteBlock = getAbsoluteBlock(diskP, partCount, partitionname, blknum);

  // Indicates that the partition doesn't exist
  if(absoluteBlock == -1)
  {
    return -3;
  }

  int diskRead = myDisk->writeDiskBlock(absoluteBlock, blkdata);

  if(diskRead != 0)
  {
    return diskRead;
  }

  return 0;
}

/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{
  for(int i = 0; i < partCount; i++)
  {
    if(diskP[i].partitionName == partitionname)
    {
      return diskP[i].partitionSize;
    }
  }

  return -1;
}

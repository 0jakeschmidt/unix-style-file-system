using namespace std;

class DiskPartition {
  public:
    char partitionName;
    int partitionSize;
};

class DiskManager {
  Disk *myDisk;
  int partCount;
  DiskPartition *diskP;

  /* declare other private members here */

  public:
    DiskManager(Disk *d, int partCount, DiskPartition *dp);
    ~DiskManager();
    int readDiskBlock(char partitionname, int blknum, char *blkdata);
    int writeDiskBlock(char partitionname, int blknum, char *blkdata);
    int getBlockSize() {return myDisk->getBlockSize();};
    int getPartitionSize(char partitionname);
};


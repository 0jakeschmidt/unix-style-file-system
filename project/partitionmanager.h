class PartitionManager {
  DiskManager *myDM;
  BitVector *myBV;

  public:
    char myPartitionName;
    int myPartitionSize;
    PartitionManager(DiskManager *dm, char partitionname, int partitionsize);
    ~PartitionManager();
    int readDiskBlock(int blknum, char *blkdata);
    int writeDiskBlock(int blknum, char *blkdata);
    int getBlockSize();
    int getFreeDiskBlock();
    int returnDiskBlock(int blknum);
};


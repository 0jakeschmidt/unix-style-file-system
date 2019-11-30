#include <map>
#include <vector>
struct FileInfo
{
  int opens, lockId;

  // Associations from file descriptors to rw pointers, modes
  std::map<int, int>*  rwPointers;
  std::map<int, char>* modes;
};

class FileSystem {
  DiskManager *myDM;
  PartitionManager *myPM;

  char myfileSystemName;
  int myfileSystemSize;
  
  // An association from block numbers to FileInfo instances
  std::map<int, FileInfo*> fileInfo;

  FileInfo* _initFileInfo(int block);
  int       _makeFileDescriptor(char* filename, int fnameLen, int offset);

  FileInfo* _getInfoFromDescriptor(int fileDesc);
  char      _getModeFromDescriptor(int fileDesc);
  int       _getRWFromDescriptor(int fileDesc);
  void      _setRWFromDescriptor(int fileDesc, int rw);

  // The returns the block number for the file i-node
  int       _getBlockFromDescriptor(int fileDesc);

  public:
    FileSystem(DiskManager *dm, char fileSystemName);
   ~FileSystem();

    int createFile(char *filename, int fnameLen);
    int createDirectory(char *dirname, int dnameLen);
    int lockFile(char *filename, int fnameLen);
    int unlockFile(char *filename, int fnameLen, int lockId);
    int deleteFile(char *filename, int fnameLen);
    int deleteDirectory(char *dirname, int dnameLen);
    int openFile(char *filename, int fnameLen, char mode, int lockId);
    int closeFile(int fileDesc);
    int readFile(int fileDesc, char *data, int len);
    int writeFile(int fileDesc, char *data, int len);
    int appendFile(int fileDesc, char *data, int len);
    int seekFile(int fileDesc, int offset, int flag);
    int renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2);
    int getAttribute(char *filename, int fnameLen /* ... and other parameters as needed */);
    int setAttribute(char *filename, int fnameLen /* ... and other parameters as needed */);
    int searchForFile(int start,char *fileName, int); // returns -1 if the file doesnt exsist, otherwise it returns blk number for file i-node
    int searchForDirec(int start,char *fileName, int);
    int getFreePointerDirectory(int &blockNum);// returns a free pointer when given blocknumber 
    void createBlankfile(int blknum, char name); // files a buffer with name in correct spot and file type, all pointers blank
    void createBlankDirectory(int blocknumber);// creates a blank directory with something in each spot for file names and types 
    void placeInDirectory(char name, int blkNum, char type, char* subDirectoryName, int subdirecNameLen);
    int validateInput(char* name, int nameLen); // checks naming conventions for files
    void resetFilePointers(int block);// frees the pointers to file block
    char getFileName(int block);
    void setFileName(int block, char name);// takes block number and a char for a name, writes it to that block
    char getFileType(int block); //takes a disk blknumber, returns that files type
    void setFileType(int block, char type);// takes block number and a char for a name, writes it to that block
    int getFileSize(int block);// takes a block number and gets the file size in that block
    void setFileSize(int block, int size);// takes a block number and sets that file i-node size 
    int getFileIndirect(int block); // takes a block number and returns the file indirect blk number, if ==0 then no indirect blk number
    void setFileIndirect(int block, int blknumIndirect);// takes a block number for file i node and blk number for indirect inode and sets it
    void getFileDataPointers(int block, vector<int> &pointers);// takes a block number and vector by reference, returns the vector with all of the pointers to data blocks, handles indirect blocks too
    void setFileDataPointers(int block, vector<int> &pointers);// takes a block number and vector, sets the pointers in the file i-node for data, handles indirect blocks
    void setDirecIndirect(int block);
    int getDirecIndirect(int block);
    int getFilefromDirec(int block, char name);
    int getDirecfromDirec(int block, char name);
    int getFilePosInDirectory(int &block, char name);


};
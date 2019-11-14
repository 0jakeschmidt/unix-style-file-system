#include <map>

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
    int getFreePointer(int blockNum);// returns a free pointer when given blocknumber 
    void createBlankfile(char* buff, char name); // files a buffer with name in correct spot and file type, all pointers blank
    void createBlankDirectory(char* buuf,char name);// creates a blank directory with something in each spot for file names and types 
    void placeInDirectory(char name, int blkNum, char type, char* subDirectoryName, int subdirecNameLen);
};


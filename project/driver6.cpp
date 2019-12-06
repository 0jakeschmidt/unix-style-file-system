
/* Driver 6*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "client.h"
#include "util.h"

using namespace std;

/*
  This driver will test the getAttributes() and setAttributes()
  functions. You need to complete this driver according to the
  attributes you have implemented in your file system, before
  testing your program.
  
  
  Required tests:
  get and set on the fs1 on a file
    and on a file that doesn't exist
    and on a file in a directory in fs1
    and on a file that doesn't exist in a directory in fs1

 fs2, fs3
  on a file both get and set on both fs2 and fs3

  samples are provided below.  Use them and/or make up your own.


*/

int main()
{

  Disk *d = new Disk(300, 64, const_cast<char *>("DISK1"));
  DiskPartition *dp = new DiskPartition[3];

  dp[0].partitionName = 'A';
  dp[0].partitionSize = 100;
  dp[1].partitionName = 'B';
  dp[1].partitionSize = 75;
  dp[2].partitionName = 'C';
  dp[2].partitionSize = 105;

  DiskManager *dm = new DiskManager(d, 3, dp);
  int f1;
  FileSystem *fs1 = new FileSystem(dm, 'A');
  FileSystem *fs2 = new FileSystem(dm, 'B');
  FileSystem *fs3 = new FileSystem(dm, 'C');
  Client *c1 = new Client(fs1);
  Client *c2 = new Client(fs2);
  Client *c3 = new Client(fs3);
  Client *c4 = new Client(fs1);
  Client *c5 = new Client(fs2);



  int r;
  char tBuff[3];
  char oBuff[2];
  r = c1->myFS->setAttribute(const_cast<char *>("/e/b"), 4, const_cast<char*>("png"), 't');
  cout << "rv from setAttribute for /e/b is "<<r<<(r==0 ? " correct wrote png" : " failed")<<endl;
  r = c1->myFS->setAttribute(const_cast<char *>("/e/f"), 4, const_cast<char*>("jpg"), 't');
  cout << "rv from setAttribute for /e/f is "<<r<<(r==0 ? " correct wrote jpg" : " failed")<<endl;
  r = c1->myFS->getAttribute(const_cast<char *>("/e/b"),  4, tBuff, 't');
  cout<< "rv from getAttribute for /e/b is " << r << (r==0 ? " correct" : " failed")<<endl;
  printBuffer(tBuff, 3);
  r = c1->myFS->getAttribute(const_cast<char *>("/e/f"),  4, tBuff, 't');
  cout<< "rv from getAttribute for /e/f is " << r << (r==0 ? " correct" : " failed")<<endl;
  printBuffer(tBuff, 3);

  r = c1->myFS->getAttribute(const_cast<char *>("/p"), 2, tBuff, 't');  //should failed!
  cout<< "rv from getAttribute for /p is " << r << (r==-1 ? " correct" : " failed")<<endl;
  r = c4->myFS->setAttribute(const_cast<char *>("/p"), 2, const_cast<char*>("gif"), 't');  //should failed!
  cout<< "rv from getAttribute for /p is " << r << (r==-1 ? " correct" : " failed")<<endl;

  
  r = c2->myFS->setAttribute(const_cast<char *>("/f"), 2, const_cast<char*>("c2"), 'o');
  cout << "rv from setAttribute for /f "<< r << (r == -3 ? " correct wrote c2" : " failed")<<endl;
  r = c5->myFS->setAttribute(const_cast<char *>("/z"), 2, const_cast<char*>("c5"), 'o');
  cout << "rv from setAttribute for /z "<< r << (r == 0 ?" correct wrote c5" : " failed")<<endl;

  r = c2->myFS->getAttribute(const_cast<char *>("/f"), 2, oBuff, 'o');
  cout << "rv from getAttribute for /f "<<r <<(r == -3 ? " correct" : " failed")<<endl;
  r = c5->myFS->getAttribute(const_cast<char *>("/z"), 2, oBuff, 'o');
  cout << "rv from getAttribute for /z " << r << (r == 0 ? " correct" : " failed")<<endl;
  printBuffer(oBuff, 2);


  // verify these are correct
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/l"), 10, const_cast<char*>("tar"), 't');
  cout << "rv from setAttribute for /o/o/o/a/l "<<r<< (r== -1 ? " correct wrote tar" : " failed")<<endl;
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/d"), 10, const_cast<char*>("txt"), 't');
  cout << "rv from setAttribute for /o/o/o/a/d "<<r<<( r == -1 ? " correct wrote tar" : " failed")<<endl;

  r = c3->myFS->getAttribute(const_cast<char *>("/o/o/o/a/l"), 10, tBuff, 't');
  cout << "rv from getAttribute for /o/o/o/a/l "<<r<<(r == -1 ? " correct" : " failed")<<endl;
  r = c3->myFS->getAttribute(const_cast<char *>("o/o/o/a/d"), 9, tBuff, 't');
  cout << "rv from getAttribute for o/o/o/a/d "<<r<<(r == -3 ? " correct" : " failed")<<endl;
  
  r = c2->myFS->setAttribute(const_cast<char *>("/f"),2, const_cast<char*>("doc"), 't');
  cout << "rv from setAttribute for /f "<<r<<(r == -3 ? " correct wrote doc" : " failed")<<endl;
  r = c5->myFS->setAttribute(const_cast<char *>("/z"), 2, const_cast<char*>("png"), 't');
  cout << "rv from setAttribute for /z "<<r<<(r == 0 ? " correct wrote png" : " failed")<<endl;
  r = c2->myFS->getAttribute(const_cast<char *>("/f"), 2, tBuff, 't');
  cout << "rv from getAttribute for /f "<<r<<(r == -3 ? " correct" : " failed")<<endl;
  r = c5->myFS->getAttribute(const_cast<char *>("/z"), 2, tBuff, 't');
  cout << "rv from getAttribute for /z "<<r<<(r == 0 ? " correct" : " failed")<<endl;
  printBuffer(tBuff, 3);


  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/l"), 10, const_cast<char*>("c3"), 'o');
  cout << "rv from setAttribute for /o/o/o/a/l "<<r<<(r == -1 ? " correct" : " failed")<<endl;
  r = c3->myFS->setAttribute(const_cast<char *>("/o/o/o/a/d"), 10, const_cast<char*>("c3"), 'o');
  cout << "rv from setAttribute for /o/o/o/a/d "<<r<<(r == -1 ? " correct" : " failed")<<endl;
  r = c3->myFS->getAttribute(const_cast<char *>("/o/o/o/a/l"), 10, oBuff, 'o');
  cout << "rv from setAttribute for /o/o/o/a/l"<<r<<(r == -1 ? " correct" : " failed")<<endl;
  r = c3->myFS->getAttribute(const_cast<char *>("o/o/o/a/d"), 9, oBuff, 'o');
  cout << "rv from setAttribute for o/o/o/a/d"<<r<<(r == -3 ? " correct" : " failed")<<endl;
  cout<<"end driver 6"<<endl;
  return 0;
}

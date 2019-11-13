
/* Driver 6*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "client.h"

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
  FileSystem *fs1 = new FileSystem(dm, 'A');
  FileSystem *fs2 = new FileSystem(dm, 'B');
  FileSystem *fs3 = new FileSystem(dm, 'C');
  Client *c1 = new Client(fs1);
  Client *c2 = new Client(fs2);
  Client *c3 = new Client(fs3);
  Client *c4 = new Client(fs1);
  Client *c5 = new Client(fs2);



  int r;

/*
What every need to show your set and get Attributes functions work

  r = c1->myFS->setAttributes(const_cast<char *>("/e/f"), ...);
  cout << ...
  r = c4->myFS->setAttributes(const_cast<char *>("/e/b"), ...);
  cout << ...
  r = c1->myFS->getAttributes(const_cast<char *>("/e/f"), ...);
  cout << ...
  r = c4->myFS->getAttributes(const_cast<char *>("/e/b"), ...);
  cout << ...
  r = c1->myFS->getAttributes(const_cast<char *>("/p"), ...);  //should failed!
  cout << ...
  r = c4->myFS->setAttributes(const_cast<char *>("/p"), ...);  //should failed!
  cout << ...
  
  r = c2->myFS->setAttributes(const_cast<char *>("/f"), ...);
  cout << ...
  r = c5->myFS->setAttributes(const_cast<char *>("/z"), ...);
  cout << ...
  r = c2->myFS->getAttributes(const_cast<char *>("/f"), ...);
  cout << ...
  r = c5->myFS->getAttributes(const_cast<char *>("/z"), ...);
  cout << ...

  r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
  cout << ...
  r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/d"), ...);
  cout << ...
  r = c3->myFS->getAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
  cout << ...
  r = c3->myFS->getAttributes(const_cast<char *>("o/o/o/a/d"), ...);
  cout << ...
  
  r = c2->myFS->setAttributes(const_cast<char *>("/f"), ...);
  cout << ...
  r = c5->myFS->setAttributes(const_cast<char *>("/z"), ...);
  cout << ...
  r = c2->myFS->getAttributes(const_cast<char *>("/f"), ...);
  cout << ...
  r = c5->myFS->getAttributes(const_cast<char *>("/z"), ...);
  cout << ...

  r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
  cout << ...
  r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/d"), ...);
  cout << ...
  r = c3->myFS->getAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
  cout << ...
  r = c3->myFS->getAttributes(const_cast<char *>("o/o/o/a/d"), ...);
  cout << ...
*/
  return 0;
}

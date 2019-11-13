#include "util.h"

#include <cstdlib>
#include <iostream>

#include "diskmanager.h"
#include "disk.h"

using namespace std;

/*
 * getIntFromBuffer will take c string and position.  Will convert the 
 * 4 position to a number.
 *
 * example: buff "123456789"  pos =3  will return the integer 4567
 *       pos=0 returns 1234
 *      
 *      Note last note, atoi requires an end of string marker '\0' to work
 *      correctly, and that is an EXTRA character (hint)
*/
int readIntFromBuffer(int pos, char* buff)
{
  char resultString[5];

  for(int i = 0; i < 4; i++)
  {
    resultString[i] = buff[pos+i];
  }

  resultString[4] = '\0';

  return atoi(resultString);
}

/*
 * takes a position and buffer and a number.  ;
 * and will convert the number to a 4 characters and starting at the position.
 *   if the number is shorter then 4 digits, use zeros to fill left positions.
 *
 *  example: buff= "1234567890"
 *  pos=3, num=0  so buff is changed to "1230000890"
 *    0 is converted to 0000
 *  pos=1, num=102 so buff is  "1010200890"
 *    Note, 102 was converted to 0102  Don't forget the 0
*/ 
void writeIntToBuffer(int pos, int num, char* buff)
{
  for(int i = pos + 3; i >= pos; i--)
  {
    if(num == 0)
    {
      buff[i] = '0';
    }
    else
    {
      buff[i] = static_cast<char>(num % 10 + 48);
      num /= 10;
    }
  }
}

void printBuffer(char* buff, int size)
{
  for (int i = 0; i < size; i++)
  {
    cout << buff[i];
  }

  cout << "\n";
}

int getAbsoluteBlock(DiskPartition* partitions, int partitionCount, int partitionName, int relativeBlock)
{
  // Starts at 1 because block 0 is the superblock
  int  result = 1;
  bool partitionExists = false;

  for(int i = 0; i < partitionCount; i++)
  {
    if(partitions[i].partitionName == partitionName)
    {
      partitionExists = true;
      break;
    }

    result += partitions[i].partitionSize;
  }

  if(!partitionExists)
  {
    return -1;
  }

  return result + relativeBlock;
}


#pragma once

class DiskPartition;
class Disk;

int readIntFromBuffer(int position, char* buffer);
void writeIntToBuffer(int position, int value, char* buffer);
void printBuffer(char * buffer, int size);

int getAbsoluteBlock(DiskPartition* partitions, int partitionCount, int partitionName, int relativeBlock);
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>//for memcpy
#include <iostream>//for cout

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  // StaticBuffer buffer;
  // OpenRelTable cache;
  // Below is stage-1
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer, 7000);
  char message[] = "hello";
  memcpy(buffer + 20, message, 6);
  Disk::writeBlock(buffer, 7000);

  unsigned char buffer2[BLOCK_SIZE];
  char message2[6];
  Disk::readBlock(buffer2, 7000);
  memcpy(message2, buffer2 + 20, 6);
  std::cout << message2;
// // Sage-1 exercise
//   unsigned char buffer[BLOCK_SIZE];
//   Disk::readBlock(buffer,0);

//   for (int i = 0; i < 4; i++) {
//     std::cout<<"Entry "<<i<<": "<<(int)buffer[i]<<std::endl;
//   }

 

  return FrontendInterface::handleFrontend(argc, argv);
}

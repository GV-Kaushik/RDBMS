#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <cstring>//for memcpy
#include <iostream>//for cout
int main(int argc, char *argv[]) {
  Disk disk_run;

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  //RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  //HeadInfo attrCatHeader;
  int attrblock;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);
  //attrCatBuffer.getHeader(&attrCatHeader);
  //printf("num:%d\n",attrCatHeader.rblock);

  std::cout << "BEFORE CHANGING STUDENTS ATTRIBUTE CLASS:\n";

  /* i = 0 to total relation count */
  for (int i=0;i<relCatHeader.numEntries;i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS];// // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
    /* j = 0 to number of entries in the attribute catalog */
    attrblock=ATTRCAT_BLOCK;
     while(attrblock!=-1){
  RecBuffer attrCatBuffer(attrblock);
    HeadInfo attrCatHeader;
    attrCatBuffer.getHeader(&attrCatHeader);
    for (int j=0;j<attrCatHeader.numEntries;j++) {

      // declare attrCatRecord and load the attribute catalog entry into it
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord,j);
      if (strcmp(attrCatRecord[0].sVal, relCatRecord[0].sVal) == 0) {
        const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        printf("  %s: %s\n", attrCatRecord[1].sVal/* get the attribute name */, attrType);
      }
 
/* attribute catalog entry corresponds to the current relation */
      if (strcmp(attrCatRecord[0].sVal, "Students") == 0 && strcmp(attrCatRecord[0].sVal,relCatRecord[0].sVal)==0) {
        if(strcmp(attrCatRecord[1].sVal,"Class")==0){
            strcpy(attrCatRecord[1].sVal,"Batch");
             unsigned char buffer[BLOCK_SIZE];
             Disk::readBlock(buffer, attrblock);
             char msg[]="Batch";
             int start=32+attrCatHeader.numSlots+(16*attrCatHeader.numAttrs*j);
             memcpy(buffer+start,attrCatRecord,16*attrCatHeader.numAttrs);
             Disk::writeBlock(buffer,attrblock);


        }
      
        // const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        // printf("  %s: %s\n", attrCatRecord[1].sVal/* get the attribute name */, attrType);
      }
    }
    
    attrblock=attrCatHeader.rblock;
  }
  
  printf("\n");
}
std::cout << "After CHANGING STUDENTS ATTRIBUTE CLASS to Batch:\n";
 for (int i=0;i<relCatHeader.numEntries;i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS];// // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
    /* j = 0 to number of entries in the attribute catalog */
    attrblock=ATTRCAT_BLOCK;
     while(attrblock!=-1){
  RecBuffer attrCatBuffer(attrblock);
    HeadInfo attrCatHeader;
    attrCatBuffer.getHeader(&attrCatHeader);
    for (int j=0;j<attrCatHeader.numEntries;j++) {

      // declare attrCatRecord and load the attribute catalog entry into it
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord,j);
 
/* attribute catalog entry corresponds to the current relation */
      if (strcmp(attrCatRecord[0].sVal, relCatRecord[0].sVal) == 0) {
        const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        printf("  %s: %s\n", attrCatRecord[1].sVal/* get the attribute name */, attrType);
      }
    }
    
    attrblock=attrCatHeader.rblock;
  }
  
  printf("\n");
}

  return 0;
}

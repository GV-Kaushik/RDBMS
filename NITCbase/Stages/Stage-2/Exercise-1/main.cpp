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

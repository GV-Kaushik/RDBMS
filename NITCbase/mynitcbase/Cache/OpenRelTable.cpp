#include "OpenRelTable.h"
#include <cstring>
#include<stdlib.h>//for malloc

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT

  /* No need like below 
//    RecBuffer attrCatBlock(RELCAT_BLOCK);// DOUBT  

//   Attribute attrCatRecord[RELCAT_NO_ATTRS];
//   attrCatBlock.getRecord(attrCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
*/


  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
  struct RelCacheEntry attrCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &attrCacheEntry.relCatEntry);
  attrCacheEntry.recId.block = RELCAT_BLOCK;
  attrCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
   RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCacheEntry;






  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  

  // iterate through all the attributes of the relation catalog and create a linked
  // list of AttrCacheEntry (slots 0 to 5)
  // for each of the entries, set
  //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
  //    attrCacheEntry.recId.slot = i   (0 to 5)
  //    and attrCacheEntry.next appropriately
  // NOTE: allocate each entry dynamically using malloc

  // set the next field in the last entry to nullptr
/**** setting up Relation Catalog relation in the Attribute Cache Table ****/
RecBuffer attrCatBlock(ATTRCAT_BLOCK);

Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

AttrCacheEntry *head = nullptr;
AttrCacheEntry *tail = nullptr;

for (int i = 0; i < RELCAT_NO_ATTRS; i++){
    attrCatBlock.getRecord(attrCatRecord,i);

    AttrCacheEntry *attrCacheEntry=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block=ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot=i;
    attrCacheEntry->next=nullptr;

    if(head==nullptr){
        head=attrCacheEntry;
        tail=attrCacheEntry;
    }else {
        tail->next=attrCacheEntry;
        tail=attrCacheEntry;
    }
}

  AttrCacheTable::attrCache[RELCAT_RELID]=head/* head of the linked list */;

  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  // set up the attributes of the attribute cache similarly.
  // read slots 6-11 from attrCatBlock and initialise recId appropriately

  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

AttrCacheEntry *head2=nullptr;
AttrCacheEntry *tail2=nullptr;

for (int i=RELCAT_NO_ATTRS;i<RELCAT_NO_ATTRS+ATTRCAT_NO_ATTRS;i++){
    attrCatBlock.getRecord(attrCatRecord,i);

    AttrCacheEntry *attrCacheEntry=(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block=ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot=i;
    attrCacheEntry->next=nullptr;

    if(head2==nullptr){
        head2=attrCacheEntry;
        tail2=attrCacheEntry;
    }else{
        tail2->next=attrCacheEntry;
        tail2=attrCacheEntry;
    }
}

AttrCacheTable::attrCache[ATTRCAT_RELID]=head2;

// find Students' record in the relation catalog
int studentsSlot=-1;
Attribute studentsRelRecord[RELCAT_NO_ATTRS];

for(int slot=0;slot<20;slot++){
    if(relCatBlock.getRecord(studentsRelRecord,slot)!=SUCCESS){
        break;
    }
    if(strcmp(studentsRelRecord[0].sVal,"Students")==0){
        studentsSlot=slot;
        break;
    }
}

// cache the relation catalog entry for Students
struct RelCacheEntry studentsRelCacheEntry;
RelCacheTable::recordToRelCatEntry(studentsRelRecord,&studentsRelCacheEntry.relCatEntry);
studentsRelCacheEntry.recId.block=RELCAT_BLOCK;
studentsRelCacheEntry.recId.slot=studentsSlot;

RelCacheTable::relCache[studentsSlot]=(struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
*(RelCacheTable::relCache[studentsSlot])=studentsRelCacheEntry;

// cache the attribute catalog entries for Students
int numAttrs=studentsRelCacheEntry.relCatEntry.numAttrs;
int found=0;
int slot=0;

struct AttrCacheEntry *head3=nullptr;
struct AttrCacheEntry *tail3=nullptr;

while(found<numAttrs){
    Attribute studentsAttrRecord[ATTRCAT_NO_ATTRS];
    if(attrCatBlock.getRecord(studentsAttrRecord,slot)!=SUCCESS){
        break;
    }
    if(strcmp(studentsAttrRecord[0].sVal,"Students")==0){
        struct AttrCacheEntry *attr=(struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        attr->next=nullptr;
        attr->dirty=false;
        attr->searchIndex={-1,-1};
        attr->recId.block=ATTRCAT_BLOCK;
        attr->recId.slot=slot;
        AttrCacheTable::recordToAttrCatEntry(studentsAttrRecord,&attr->attrCatEntry);

        if(head3==nullptr){
            head3=attr;
            tail3=attr;
        }
        else{
            tail3->next=attr;
            tail3=attr;
        }
        found++;
    }
    slot++;
}

AttrCacheTable::attrCache[studentsSlot]=head3;
}

OpenRelTable::~OpenRelTable() {
  // free all the memory that you allocated in the constructor
 
    for(int i=0;i<2;i++){
        free(RelCacheTable::relCache[i]);
        RelCacheTable::relCache[i]=nullptr;
    }

    for(int i=0;i<2;i++){
        AttrCacheEntry *curr=AttrCacheTable::attrCache[i];
        while(curr!=nullptr){
            AttrCacheEntry *next=curr->next;
            free(curr);
            curr=next;
        }
        AttrCacheTable::attrCache[i]=nullptr;
    }

}
#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h> //for malloc

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable()
{

  // initialise all values in relCache and attrCache to be nullptr and all entries in tableMetaInfo to be free
  for (int i = 0; i < MAX_OPEN; ++i)
  {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free = true;
  }

  // load the relation and attribute catalog into the relation cache (we did this already)
  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
  relCacheEntry.dirty=false;
  relCacheEntry.searchIndex={-1,-1};

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
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
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCacheEntry;

  // load the relation and attribute catalog into the attribute cache (we did this already)

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

  for (int i = 0; i < RELCAT_NO_ATTRS; i++)
  {
    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheEntry *attrCacheEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    attrCacheEntry->dirty=false;
    attrCacheEntry->searchIndex={-1,-1};
    attrCacheEntry->next = nullptr;

    if (head == nullptr)
    {
      head = attrCacheEntry;
      tail = attrCacheEntry;
    }
    else
    {
      tail->next = attrCacheEntry;
      tail = attrCacheEntry;
    }
  }

  AttrCacheTable::attrCache[RELCAT_RELID] = head /* head of the linked list */;

  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  // set up the attributes of the attribute cache similarly.
  // read slots 6-11 from attrCatBlock and initialise recId appropriately

  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  AttrCacheEntry *head2 = nullptr;
  AttrCacheEntry *tail2 = nullptr;

  for (int i = RELCAT_NO_ATTRS; i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; i++)
  {
    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheEntry *attrCacheEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    attrCacheEntry->next = nullptr;

    if (head2 == nullptr)
    {
      head2 = attrCacheEntry;
      tail2 = attrCacheEntry;
    }
    else
    {
      tail2->next = attrCacheEntry;
      tail2 = attrCacheEntry;
    }
  }

  AttrCacheTable::attrCache[ATTRCAT_RELID] = head2;

  /************ Setting up tableMetaInfo entries ************/

  // in the tableMetaInfo array
  //   set free = false for RELCAT_RELID and ATTRCAT_RELID
  //   set relname for RELCAT_RELID and ATTRCAT_RELID

  tableMetaInfo[RELCAT_RELID].free = false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
  tableMetaInfo[ATTRCAT_RELID].free = false;
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable(){

  // close all open relations (from rel-id = 2 onwards. Why?)
  for(int i=2;i<MAX_OPEN;++i){
    if(!tableMetaInfo[i].free){
      OpenRelTable::closeRel(i);
    }
  }

  /**** Closing the catalog relations in the relation cache ****/

  // releasing the relation cache entry of the attribute catalog
  if(RelCacheTable::relCache[ATTRCAT_RELID]->dirty){
    RelCatEntry relCatEntry=RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry;
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCatEntry,relCatRecord);

    RecId recId=RelCacheTable::relCache[ATTRCAT_RELID]->recId;
    RecBuffer relCatBlock(recId.block);
    relCatBlock.setRecord(relCatRecord,recId.slot);
  }
  free(RelCacheTable::relCache[ATTRCAT_RELID]);
  RelCacheTable::relCache[ATTRCAT_RELID]=nullptr;

  // releasing the relation cache entry of the relation catalog
  if(RelCacheTable::relCache[RELCAT_RELID]->dirty){
    RelCatEntry relCatEntry=RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCatEntry,relCatRecord);

    RecId recId=RelCacheTable::relCache[RELCAT_RELID]->recId;
    RecBuffer relCatBlock(recId.block);
    relCatBlock.setRecord(relCatRecord,recId.slot);
  }
  free(RelCacheTable::relCache[RELCAT_RELID]);
  RelCacheTable::relCache[RELCAT_RELID]=nullptr;

  // free the memory allocated for the attribute cache entries of the
  // relation catalog and the attribute catalog
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

int OpenRelTable::getFreeOpenRelTableEntry()
{
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (tableMetaInfo[i].free)
    {
      return i;
    }
  }
  return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (strcmp(tableMetaInfo[i].relName, relName) == 0)
    {
      return i;
    }
  }
  return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{ // here it was unsignedcheck this?

  int ret = OpenRelTable::getRelId(relName);

  if (ret != E_RELNOTOPEN) /* the relation `relName` already has an entry in the Open Relation Table */
  {
    // (checked using OpenRelTable::getRelId())
    return ret;

    // return that relation id;
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
  int slot = OpenRelTable::getFreeOpenRelTableEntry();

  if (slot == E_CACHEFULL /* free slot not available */)
  {
    return E_CACHEFULL;
  }

  // let relId be used to store the free slot.
  int relId = slot;

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  char relNameAttr[ATTR_SIZE] = "RelName";
  Attribute attrVal;
  strcpy(attrVal.sVal, relName);

  RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, relNameAttr, attrVal, EQ);

  if (relcatRecId.block == -1 && relcatRecId.slot == -1 /* relcatRecId == {-1, -1} */)
  {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
  RecBuffer relcatBlock(relcatRecId.block);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relcatBlock.getRecord(relCatRecord, relcatRecId.slot);

  RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);

  relCacheEntry.recId.block = relcatRecId.block;
  relCacheEntry.recId.slot = relcatRecId.slot;
  relCacheEntry.dirty = false;          // doubt
  relCacheEntry.searchIndex = {-1, -1}; // doubt

  RelCacheTable::relCache[relId] = (struct RelCacheEntry *)malloc(sizeof(struct RelCacheEntry));
  *(RelCacheTable::relCache[relId]) = relCacheEntry;

 
  

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry *listHead=nullptr;
  
  AttrCacheEntry *tail = nullptr;

  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  char attrcatRelNameAttr[ATTR_SIZE] = "RelName";

  while (true)
  {
    /*iterate over all the entries in the Attribute Catalog corresponding to each
    attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
    care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
    corresponding to Attribute Catalog before the first call to linearSearch().*/

    /* let attrcatRecId store a valid record id an entry of the relation, relName,
    in the Attribute Catalog.*/
    RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatRelNameAttr, attrVal, EQ);

    if (attrcatRecId.block == -1 && attrcatRecId.slot == -1)
    {
      // no more attribute records for this relation
      break;
    }

     /* read the record entry corresponding to attrcatRecId and create an
    Attribute Cache entry on it using RecBuffer::getRecord() and
    AttrCacheTable::recordToAttrCatEntry().
    update the recId field of this Attribute Cache entry to attrcatRecId.
    add the Attribute Cache entry to the linked list of listHead .*/
    // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()

    RecBuffer attrcatBlock(attrcatRecId.block);

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    attrcatBlock.getRecord(attrCatRecord, attrcatRecId.slot);

    AttrCacheEntry *attrCacheEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = attrcatRecId.block;
    attrCacheEntry->recId.slot = attrcatRecId.slot;
    attrCacheEntry->dirty=false;
    attrCacheEntry->searchIndex={-1,-1};
    attrCacheEntry->next = nullptr;

    if (listHead == nullptr)
    {
      listHead = attrCacheEntry;
      tail = attrCacheEntry;
    }
    else
    {
      tail->next = attrCacheEntry;
      tail = attrCacheEntry;
    }
  }

  // set the relIdth entry of the AttrCacheTable to listHead.
  AttrCacheTable::attrCache[relId] = listHead;

  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  tableMetaInfo[relId].free=false;
  strcpy(tableMetaInfo[relId].relName,relName);

  return relId;
}


int OpenRelTable::closeRel(int relId) {
  if (relId==0 || relId==1/* rel-id corresponds to relation catalog or attribute catalog*/) {
    return E_NOTPERMITTED;
  }

  if (relId<0 || relId>=MAX_OPEN/* 0 <= relId < MAX_OPEN */) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free==true/* rel-id corresponds to a free slot*/) {
    return E_RELNOTOPEN;
  }

  /****** Releasing the Relation Cache entry of the relation ******/

  if (RelCacheTable::relCache[relId]->dirty==true) {

    // Get the Relation Catalog entry from RelCacheTable::relCache
    // Then convert it to a record using RelCacheTable::relCatEntryToRecord().
    union Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry), record);

    // declaring an object of RecBuffer class to write back to the buffer
    RecId recId = RelCacheTable::relCache[relId]->recId;
    RecBuffer relCatBlock(recId.block);

    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    relCatBlock.setRecord(record, recId.slot);
  }

  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function

  free(RelCacheTable::relCache[relId]);

  AttrCacheEntry *curr = AttrCacheTable::attrCache[relId];
  while (curr != nullptr) {
    AttrCacheEntry *next = curr->next;
    free(curr);
    curr = next;
  }
  RelCacheTable::relCache[relId] = nullptr;
  AttrCacheTable::attrCache[relId] = nullptr;
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr

  // update `tableMetaInfo` to set `relId` as a free slot
  tableMetaInfo[relId].free = true;
  strcpy(tableMetaInfo[relId].relName,"");

  return SUCCESS;
}


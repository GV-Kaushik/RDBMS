#include "BPlusTree.h"

#include <cstring>

int BPlusTree::bPlusCreate(int relId,char attrName[ATTR_SIZE]){

  //if relId is either RELCAT_RELID or ATTRCAT_RELID:
  //return E_NOTPERMITTED
  if(relId==RELCAT_RELID||relId==ATTRCAT_RELID){
    return E_NOTPERMITTED;
  }

  //get the attribute catalog entry of attribute `attrName`
  //using AttrCacheTable::getAttrCatEntry()
  AttrCatEntry attrCatEntry;
  int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

  //if getAttrCatEntry fails
  //return the error code from getAttrCatEntry
  if(ret!=SUCCESS){
    return ret;
  }

  //an index already exists for the attribute (check rootBlock field)
  if(attrCatEntry.rootBlock!=-1){
    return SUCCESS;
  }

  /******Creating a new B+ Tree ******/

  //get a free leaf block using constructor 1 to allocate a new block
  IndLeaf rootBlockBuf;

  //(if the block could not be allocated, the appropriate error code
  //will be stored in the blockNum member field of the object)

  //declare rootBlock to store the blockNumber of the new leaf block
  int rootBlock=rootBlockBuf.getBlockNum();

  //if there is no more disk space for creating an index
  if(rootBlock==E_DISKFULL){
    return E_DISKFULL;
  }

  //initialising the header of the new root leaf block
  HeadInfo leafHead;
  rootBlockBuf.getHeader(&leafHead);
  leafHead.blockType=IND_LEAF;
  leafHead.pblock=-1;
  leafHead.lblock=-1;
  leafHead.rblock=-1;
  leafHead.numEntries=0;
  rootBlockBuf.setHeader(&leafHead);

  //set the root block in the attribute cache entry, bPlusInsert reads it from there
  attrCatEntry.rootBlock=rootBlock;
  AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatEntry);

  RelCatEntry relCatEntry;

  //load the relation catalog entry into relCatEntry
  //using RelCacheTable::getRelCatEntry()
  RelCacheTable::getRelCatEntry(relId,&relCatEntry);

  //first record block of the relation
  int block=relCatEntry.firstBlk;

  /***** Traverse all the blocks in the relation and insert them one
         by one into the B+ Tree *****/
  while(block!=-1){

    //declare a RecBuffer object for `block`
    RecBuffer blockBuffer(block);

    unsigned char slotMap[relCatEntry.numSlotsPerBlk];

    //load the slot map into slotMap using RecBuffer::getSlotMap()
    blockBuffer.getSlotMap(slotMap);

    //for every occupied slot of the block
    for(int slot=0;slot<relCatEntry.numSlotsPerBlk;slot++){
      if(slotMap[slot]==SLOT_OCCUPIED){

        Attribute record[relCatEntry.numAttrs];
        //load the record corresponding to the slot into `record`
        //using RecBuffer::getRecord()
        blockBuffer.getRecord(record,slot);

        //declare recId and store the rec-id of this record in it
        RecId recId={block,slot};

        //insert the attribute value corresponding to attrName from the record
        //into the B+ tree using bPlusInsert
        //(bPlusInsert will destroy any existing bplus tree if
        //insert fails i.e when disk is full)
        int retVal=bPlusInsert(relId,attrName,record[attrCatEntry.offset],recId);

        if(retVal==E_DISKFULL){
          //unable to get enough blocks to build the B+ Tree
          return E_DISKFULL;
        }
      }
    }

    //get the header of the block using BlockBuffer::getHeader()
    HeadInfo head;
    blockBuffer.getHeader(&head);

    //set block=rblock of current block (from the header)
    block=head.rblock;
  }

  return SUCCESS;
}

RecId BPlusTree::bPlusSearch(int relId,char attrName[ATTR_SIZE],Attribute attrVal,int op){
    // declare searchIndex which will be used to store search index for attrName.
    IndexId searchIndex;
    /* get the search index corresponding to attribute with name attrName
       using AttrCacheTable::getSearchIndex(). */
    AttrCacheTable::getSearchIndex(relId,attrName,&searchIndex);
    // TO CHECK LATER: step 2 of the algorithm in the doc says to reset searchIndex to {-1,-1}
    // right here (after reading it), and it is set again only when a hit is found.
    // i did not add that reset, because the note in the doc says that after the last hit
    // every call returns {-1,-1} until the caller resets the search index.
    // the two descriptions disagree, so if a repeated search does not start from
    // the beginning (or keeps returning old results) in testing, add this line here:
    // AttrCacheTable::resetSearchIndex(relId,attrName);
    AttrCatEntry attrCatEntry;
    /* load the attribute cache entry into attrCatEntry using
     AttrCacheTable::getAttrCatEntry(). */
    AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
    // declare variables block and index which will be used during search
    int block,index;
    if(searchIndex.block==-1&&searchIndex.index==-1){
        // (search is done for the first time)
        // start the search from the first entry of root.
        block=attrCatEntry.rootBlock;
        index=0;
        if(block==-1){
            // attrName doesn't have a B+ tree
            return RecId{-1,-1};
        }
    }else{
        /*a valid searchIndex points to an entry in the leaf index of the attribute's
        B+ Tree which had previously satisfied the op for the given attrVal.*/
        block=searchIndex.block;
        index=searchIndex.index+1; // search is resumed from the next index.
        // load block into leaf using IndLeaf::IndLeaf().
        IndLeaf leaf(block);
        // declare leafHead which will be used to hold the header of leaf.
        HeadInfo leafHead;
        // load header into leafHead using BlockBuffer::getHeader().
        leaf.getHeader(&leafHead);
        if(index>=leafHead.numEntries){
            /* (all the entries in the block has been searched; search from the
            beginning of the next leaf index block. */
            // update block to rblock of current block and index to 0.
            block=leafHead.rblock;
            index=0;
            if(block==-1){
                // (end of linked list reached - the search is done.)
                return RecId{-1,-1};
            }
        }
    }
    /******  Traverse through all the internal nodes according to value
            of attrVal and the operator op                             ******/
    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */
    while(StaticBuffer::getStaticBlockType(block)==IND_INTERNAL){ //use StaticBuffer::getStaticBlockType()
        // load the block into internalBlk using IndInternal::IndInternal().
        IndInternal internalBlk(block);
        HeadInfo intHead;
        // load the header of internalBlk into intHead using BlockBuffer::getHeader()
        internalBlk.getHeader(&intHead);
        // declare intEntry which will be used to store an entry of internalBlk.
        InternalEntry intEntry;
        if(op==NE||op==LT||op==LE){
            /*
            - NE: need to search the entire linked list of leaf indices of the B+ Tree,
            starting from the leftmost leaf index. Thus, always move to the left.
            - LT and LE: the attribute values are arranged in ascending order in the
            leaf indices of the B+ Tree. Values that satisfy these conditions, if
            any exist, will always be found in the left-most leaf index. Thus,
            always move to the left.
            */
            // load entry in the first slot of the block into intEntry
            // using IndInternal::getEntry().
            internalBlk.getEntry(&intEntry,0);
            block=intEntry.lChild;
        }else{
            /*
            - EQ, GT and GE: move to the left child of the first entry that is
            greater than (or equal to) attrVal
            (we are trying to find the first entry that satisfies the condition.
            since the values are in ascending order we move to the left child which
            might contain more entries that satisfy the condition)
            */
            /*
             traverse through all entries of internalBlk and find an entry that
             satisfies the condition.
             if op == EQ or GE, then intEntry.attrVal >= attrVal
             if op == GT, then intEntry.attrVal > attrVal
             Hint: the helper function compareAttrs() can be used for comparing
            */
            // TO CHECK LATER: compareAttrs(Attribute,Attribute,int attrType) is the signature
            // i assumed (returns <0, 0 or >0), check it against your header
            int i=0;
            bool found=false;
            while(i<intHead.numEntries){
                internalBlk.getEntry(&intEntry,i);
                int cmpVal=compareAttrs(intEntry.attrVal,attrVal,attrCatEntry.attrType);
                if((op==GT&&cmpVal>0)||((op==EQ||op==GE)&&cmpVal>=0)){
                    found=true;
                    break;
                }
                i++;
            }
            if(found){
                // move to the left child of that entry
                block=intEntry.lChild; // left child of the entry
            }else{
                // move to the right child of the last entry of the block
                // i.e numEntries - 1 th entry of the block
                // (intEntry still holds the last entry after the loop)
                block=intEntry.rChild; // right child of last entry
            }
        }
    }
    // NOTE: `block` now has the block number of a leaf index block.
    /******  Identify the first leaf index entry from the current position
            that satisfies our condition (moving right)             ******/
    while(block!=-1){
        // load the block into leafBlk using IndLeaf::IndLeaf().
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        // load the header to leafHead using BlockBuffer::getHeader().
        leafBlk.getHeader(&leafHead);
        // declare leafEntry which will be used to store an entry from leafBlk
        Index leafEntry;
        while(index<leafHead.numEntries){
            // load entry corresponding to block and index into leafEntry
            // using IndLeaf::getEntry().
            leafBlk.getEntry(&leafEntry,index);
            int cmpVal=compareAttrs(leafEntry.attrVal,attrVal,attrCatEntry.attrType); /* comparison between leafEntry's attribute value
                        and input attrVal using compareAttrs()*/
            if(
                (op==EQ&&cmpVal==0)||
                (op==LE&&cmpVal<=0)||
                (op==LT&&cmpVal<0)||
                (op==GT&&cmpVal>0)||
                (op==GE&&cmpVal>=0)||
                (op==NE&&cmpVal!=0)
            ){
                // (entry satisfying the condition found)
                // set search index to {block, index}
                IndexId newSearchIndex={block,index};
                AttrCacheTable::setSearchIndex(relId,attrName,&newSearchIndex);
                // return the recId {leafEntry.block, leafEntry.slot}.
                return RecId{leafEntry.block,leafEntry.slot};
            }else if((op==EQ||op==LE||op==LT)&&cmpVal>0){
                /*future entries will not satisfy EQ, LE, LT since the values
                    are arranged in ascending order in the leaves */
                // return RecId {-1, -1};
                return RecId{-1,-1};
            }
            // search next index.
            ++index;
        }
        /*only for NE operation do we have to check the entire linked list;
        for all the other op it is guaranteed that the block being searched
        will have an entry, if it exists, satisfying that op. */
        if(op!=NE){
            break;
        }
        // block = next block in the linked list, i.e., the rblock in leafHead.
        // update index to 0.
        block=leafHead.rblock;
        index=0;
    }
    // no entry satisfying the op was found; return the recId {-1,-1}
    return RecId{-1,-1};
}

int BPlusTree::bPlusDestroy(int rootBlockNum){

  //rootBlockNum lies outside the valid range [0,DISK_BLOCKS-1]
  if(rootBlockNum<0||rootBlockNum>=DISK_BLOCKS){
    return E_OUTOFBOUND;
  }

  //type of block (using StaticBuffer::getStaticBlockType())
  int type=StaticBuffer::getStaticBlockType(rootBlockNum);

  if(type==IND_LEAF){
    //declare an instance of IndLeaf for rootBlockNum using appropriate
    //constructor
    IndLeaf leaf(rootBlockNum);

    //release the block using BlockBuffer::releaseBlock()
    leaf.releaseBlock();

    return SUCCESS;

  }else if(type==IND_INTERNAL){
    //declare an instance of IndInternal for rootBlockNum using appropriate
    //constructor
    IndInternal internalBlk(rootBlockNum);

    //load the header of the block using BlockBuffer::getHeader()
    HeadInfo head;
    internalBlk.getHeader(&head);

    //iterate through all the entries of the internalBlk and destroy the lChild
    //of the first entry and rChild of all entries using BPlusTree::bPlusDestroy()
    //(the rchild of an entry is the same as the lchild of the next entry,
    //so the overlapping children are not deleted more than once)
    InternalEntry internalEntry;
    internalBlk.getEntry(&internalEntry,0);
    bPlusDestroy(internalEntry.lChild);

    for(int i=0;i<head.numEntries;i++){
      internalBlk.getEntry(&internalEntry,i);
      bPlusDestroy(internalEntry.rChild);
    }

    //release the block using BlockBuffer::releaseBlock()
    internalBlk.releaseBlock();

    return SUCCESS;

  }else{
    //block is not an index block
    return E_INVALIDBLOCK;
  }
}

int BPlusTree::bPlusInsert(int relId,char attrName[ATTR_SIZE],Attribute attrVal,RecId recId){

  //get the attribute cache entry corresponding to attrName
  //using AttrCacheTable::getAttrCatEntry()
  AttrCatEntry attrCatEntry;
  int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

  //if getAttrCatEntry() failed
  //return the error code
  if(ret!=SUCCESS){
    return ret;
  }

  //rootBlock of B+ Tree (from attrCatEntry)
  int blockNum=attrCatEntry.rootBlock;

  if(blockNum==-1){
    //there is no index on attribute (rootBlock is -1)
    return E_NOINDEX;
  }

  //find the leaf block to which insertion is to be done using the
  //findLeafToInsert() function
  int leafBlkNum=findLeafToInsert(blockNum,attrVal,attrCatEntry.attrType);

  //insert the attrVal and recId to the leaf block at blockNum using the
  //insertIntoLeaf() function
  //declare a struct Index with attrVal=attrVal, block=recId.block and
  //slot=recId.slot to pass as argument to the function
  Index indexEntry;
  indexEntry.attrVal=attrVal;
  indexEntry.block=recId.block;
  indexEntry.slot=recId.slot;

  //the insertIntoLeaf() function will propagate the insertion to the
  //required internal nodes by calling the required helper functions
  //like insertIntoInternal() or createNewRoot()
  ret=insertIntoLeaf(relId,attrName,leafBlkNum,indexEntry);

  if(ret==E_DISKFULL){
    //destroy the existing B+ tree by passing the rootBlock to bPlusDestroy()
    bPlusDestroy(blockNum);

    //update the rootBlock of attribute catalog cache entry to -1 using
    //AttrCacheTable::setAttrCatEntry()
    attrCatEntry.rootBlock=-1;
    AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatEntry);

    return E_DISKFULL;
  }

  return SUCCESS;
}



int BPlusTree::findLeafToInsert(int rootBlock,Attribute attrVal,int attrType){
  int blockNum=rootBlock;

  while(StaticBuffer::getStaticBlockType(blockNum)!=IND_LEAF){
    //block is not of type IND_LEAF

    //declare an IndInternal object for block using appropriate constructor
    IndInternal internalBlk(blockNum);

    //get header of the block using BlockBuffer::getHeader()
    HeadInfo blockHeader;
    internalBlk.getHeader(&blockHeader);

    //iterate through all the entries, to find the first entry whose
    //attribute value>=value to be inserted
    InternalEntry entry;
    int index;
    for(index=0;index<blockHeader.numEntries;index++){
      internalBlk.getEntry(&entry,index);
      if(compareAttrs(entry.attrVal,attrVal,attrType)>=0){
        break;
      }
    }

    if(index==blockHeader.numEntries){
      //no such entry is found
      //set blockNum=rChild of (nEntries-1)'th entry of the block
      //(i.e. rightmost child of the block)
      internalBlk.getEntry(&entry,blockHeader.numEntries-1);
      blockNum=entry.rChild;
    }else{
      //set blockNum=lChild of the entry that was found
      blockNum=entry.lChild;
    }
  }

  return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId,char attrName[ATTR_SIZE],int blockNum,Index indexEntry){

  //get the attribute cache entry corresponding to attrName
  //using AttrCacheTable::getAttrCatEntry()
  AttrCatEntry attrCatEntry;
  AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

  //declare an IndLeaf instance for the block using appropriate constructor
  IndLeaf leaf(blockNum);

  HeadInfo blockHeader;
  //store the header of the leaf index block into blockHeader
  //using BlockBuffer::getHeader()
  leaf.getHeader(&blockHeader);

  //the following variable will be used to store a list of index entries with
  //existing indices+the new index to insert
  Index indices[blockHeader.numEntries+1];

  //iterate through all the entries in the block and copy them to the array indices
  //also insert `indexEntry` at appropriate position in the indices array
  //maintaining the ascending order
  int k=0;
  bool inserted=false;
  for(int i=0;i<blockHeader.numEntries;i++){
    Index currEntry;
    leaf.getEntry(&currEntry,i);

    if(!inserted&&compareAttrs(currEntry.attrVal,indexEntry.attrVal,attrCatEntry.attrType)>0){
      indices[k++]=indexEntry;
      inserted=true;
    }
    indices[k++]=currEntry;
  }
  if(!inserted){
    indices[k++]=indexEntry;
  }

  if(blockHeader.numEntries!=MAX_KEYS_LEAF){
    //leaf block has not reached max limit

    //increment blockHeader.numEntries and update the header of block
    //using BlockBuffer::setHeader()
    blockHeader.numEntries++;
    leaf.setHeader(&blockHeader);

    //iterate through all the entries of the array `indices` and populate the
    //entries of block with them using IndLeaf::setEntry()
    for(int i=0;i<blockHeader.numEntries;i++){
      leaf.setEntry(&indices[i],i);
    }

    return SUCCESS;
  }

  //if we reached here, the `indices` array has more entries than can fit
  //in a single leaf index block, so the entries are split between two leaf
  //blocks using splitLeaf(), which returns the blockNum of the new block
  //or E_DISKFULL if there are no more blocks to be allocated
  int newRightBlk=splitLeaf(blockNum,indices);

  //if splitLeaf() returned E_DISKFULL
  //return E_DISKFULL
  if(newRightBlk==E_DISKFULL){
    return E_DISKFULL;
  }

  int ret;
  if(blockHeader.pblock!=-1){
    //the current leaf block was not the root

    //insert the middle value from `indices` into the parent block using the
    //insertIntoInternal() function (i.e the last value of the left block)
    //the middle value will be at index 31 (MIDDLE_INDEX_LEAF)
    InternalEntry internalEntry;
    internalEntry.attrVal=indices[MIDDLE_INDEX_LEAF].attrVal;
    internalEntry.lChild=blockNum;
    internalEntry.rChild=newRightBlk;

    //insertIntoInternal(relId,attrName,parent of current block,new internal entry)
    ret=insertIntoInternal(relId,attrName,blockHeader.pblock,internalEntry);
  }else{
    //the current block was the root block and is now split, a new internal index
    //block needs to be allocated and made the root of the tree
    ret=createNewRoot(relId,attrName,indices[MIDDLE_INDEX_LEAF].attrVal,blockNum,newRightBlk);
  }

  //if either of the above calls returned an error (E_DISKFULL), then return that
  //else return SUCCESS
  return ret;
}

int BPlusTree::splitLeaf(int leafBlockNum,Index indices[]){

  //declare rightBlk, an instance of IndLeaf using constructor 1 to obtain new
  //leaf index block that will be used as the right block in the splitting
  IndLeaf rightBlk;

  //declare leftBlk, an instance of IndLeaf using constructor 2 to read from
  //the existing leaf block
  IndLeaf leftBlk(leafBlockNum);

  int rightBlkNum=rightBlk.getBlockNum();
  int leftBlkNum=leftBlk.getBlockNum();

  if(rightBlkNum==E_DISKFULL){
    //failed to obtain a new leaf index block because the disk is full
    return E_DISKFULL;
  }

  HeadInfo leftBlkHeader,rightBlkHeader;
  //get the headers of left block and right block using BlockBuffer::getHeader()
  leftBlk.getHeader(&leftBlkHeader);
  rightBlk.getHeader(&rightBlkHeader);

  //set rightBlkHeader with the following values
  //number of entries=(MAX_KEYS_LEAF+1)/2=32, pblock=pblock of leftBlk,
  //lblock=leftBlkNum, rblock=rblock of leftBlk
  //and update the header of rightBlk using BlockBuffer::setHeader()
  rightBlkHeader.blockType=IND_LEAF;
  rightBlkHeader.numEntries=(MAX_KEYS_LEAF+1)/2;
  rightBlkHeader.pblock=leftBlkHeader.pblock;
  rightBlkHeader.lblock=leftBlkNum;
  rightBlkHeader.rblock=leftBlkHeader.rblock;
  rightBlk.setHeader(&rightBlkHeader);

  //set leftBlkHeader with the following values
  //number of entries=(MAX_KEYS_LEAF+1)/2=32, rblock=rightBlkNum
  //and update the header of leftBlk using BlockBuffer::setHeader()
  leftBlkHeader.numEntries=(MAX_KEYS_LEAF+1)/2;
  leftBlkHeader.rblock=rightBlkNum;
  leftBlk.setHeader(&leftBlkHeader);

  //set the first 32 entries of leftBlk=the first 32 entries of indices array
  //and set the first 32 entries of newRightBlk=the next 32 entries of
  //indices array using IndLeaf::setEntry()
  for(int i=0;i<(MAX_KEYS_LEAF+1)/2;i++){
    leftBlk.setEntry(&indices[i],i);
    rightBlk.setEntry(&indices[i+(MAX_KEYS_LEAF+1)/2],i);
  }

  return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId,char attrName[ATTR_SIZE],int intBlockNum,InternalEntry intEntry){

  //get the attribute cache entry corresponding to attrName
  //using AttrCacheTable::getAttrCatEntry()
  AttrCatEntry attrCatEntry;
  AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

  //declare intBlk, an instance of IndInternal using constructor 2 for the block
  //corresponding to intBlockNum
  IndInternal intBlk(intBlockNum);

  HeadInfo blockHeader;
  //load blockHeader with header of intBlk using BlockBuffer::getHeader()
  intBlk.getHeader(&blockHeader);

  //declare internalEntries to store all existing entries+the new entry
  InternalEntry internalEntries[blockHeader.numEntries+1];

  //iterate through all the entries in the block and copy them to the array
  //`internalEntries`, insert `intEntry` at appropriate position in the
  //array maintaining the ascending order
  int k=0;
  bool inserted=false;
  for(int i=0;i<blockHeader.numEntries;i++){
    InternalEntry currEntry;
    intBlk.getEntry(&currEntry,i);

    if(!inserted&&compareAttrs(currEntry.attrVal,intEntry.attrVal,attrCatEntry.attrType)>0){
      internalEntries[k++]=intEntry;
      inserted=true;

      //update the lChild of the entry immediately following the newly added
      //entry to the rChild of the newly added entry
      currEntry.lChild=intEntry.rChild;
    }
    internalEntries[k++]=currEntry;
  }
  if(!inserted){
    internalEntries[k++]=intEntry;
  }

  if(blockHeader.numEntries!=MAX_KEYS_INTERNAL){
    //internal index block has not reached max limit

    //increment blockheader.numEntries and update the header of intBlk
    //using BlockBuffer::setHeader()
    blockHeader.numEntries++;
    intBlk.setHeader(&blockHeader);

    //iterate through all entries in internalEntries array and populate the
    //entries of intBlk with them using IndInternal::setEntry()
    for(int i=0;i<blockHeader.numEntries;i++){
      intBlk.setEntry(&internalEntries[i],i);
    }

    return SUCCESS;
  }

  //if we reached here, the `internalEntries` array has more entries than
  //can fit in a single internal index block, so the entries are split between
  //two internal index blocks using splitInternal(), which returns the blockNum
  //of the new block or E_DISKFULL if there are no more blocks to be allocated
  int newRightBlk=splitInternal(intBlockNum,internalEntries);

  if(newRightBlk==E_DISKFULL){
    //splitInternal() returned E_DISKFULL

    //using bPlusDestroy(), destroy the right subtree, rooted at intEntry.rChild
    //this corresponds to the tree built up till now that has not yet been
    //connected to the existing B+ Tree
    bPlusDestroy(intEntry.rChild);

    return E_DISKFULL;
  }

  int ret;
  if(blockHeader.pblock!=-1){
    //the current block was not the root

    //insert the middle value from `internalEntries` into the parent block
    //using the insertIntoInternal() function (recursively)
    //the middle value will be at index 50 (MIDDLE_INDEX_INTERNAL)
    InternalEntry middleEntry;
    middleEntry.lChild=intBlockNum;
    middleEntry.rChild=newRightBlk;
    middleEntry.attrVal=internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;

    //insertIntoInternal(relId,attrName,parent of current block,new internal entry)
    ret=insertIntoInternal(relId,attrName,blockHeader.pblock,middleEntry);
  }else{
    //the current block was the root block and is now split, a new internal index
    //block needs to be allocated and made the root of the tree
    ret=createNewRoot(relId,attrName,internalEntries[MIDDLE_INDEX_INTERNAL].attrVal,intBlockNum,newRightBlk);
  }

  //if either of the above calls returned an error (E_DISKFULL), then return that
  //else return SUCCESS
  return ret;
}

int BPlusTree::splitInternal(int intBlockNum,InternalEntry internalEntries[]){

  //declare rightBlk, an instance of IndInternal using constructor 1 to obtain new
  //internal index block that will be used as the right block in the splitting
  IndInternal rightBlk;

  //declare leftBlk, an instance of IndInternal using constructor 2 to read from
  //the existing internal index block
  IndInternal leftBlk(intBlockNum);

  int rightBlkNum=rightBlk.getBlockNum();
  int leftBlkNum=leftBlk.getBlockNum();

  if(rightBlkNum==E_DISKFULL){
    //failed to obtain a new internal index block because the disk is full
    return E_DISKFULL;
  }

  HeadInfo leftBlkHeader,rightBlkHeader;
  //get the headers of left block and right block using BlockBuffer::getHeader()
  leftBlk.getHeader(&leftBlkHeader);
  rightBlk.getHeader(&rightBlkHeader);

  //set rightBlkHeader with the following values
  //number of entries=(MAX_KEYS_INTERNAL)/2=50, pblock=pblock of leftBlk
  //and update the header of rightBlk using BlockBuffer::setHeader()
  rightBlkHeader.blockType=IND_INTERNAL;
  rightBlkHeader.numEntries=MAX_KEYS_INTERNAL/2;
  rightBlkHeader.pblock=leftBlkHeader.pblock;
  rightBlk.setHeader(&rightBlkHeader);

  //set leftBlkHeader with the following values
  //number of entries=(MAX_KEYS_INTERNAL)/2=50
  //and update the header using BlockBuffer::setHeader()
  leftBlkHeader.numEntries=MAX_KEYS_INTERNAL/2;
  leftBlk.setHeader(&leftBlkHeader);

  //set the first 50 entries of leftBlk=index 0 to 49 of internalEntries array
  //set the first 50 entries of newRightBlk=entries from index 51 to 100
  //of internalEntries array using IndInternal::setEntry()
  //(index 50 will be moving to the parent internal index block)
  for(int i=0;i<MAX_KEYS_INTERNAL/2;i++){
    leftBlk.setEntry(&internalEntries[i],i);
    rightBlk.setEntry(&internalEntries[i+MAX_KEYS_INTERNAL/2+1],i);
  }

  //block type of a child of any entry of the internalEntries array
  int type=StaticBuffer::getStaticBlockType(internalEntries[0].lChild);

  //the children of the new right block are the lChild of entry 51
  //and the rChild of entries 51 to 100
  //first the lChild of the first entry of the right block
  BlockBuffer firstChild(internalEntries[MAX_KEYS_INTERNAL/2+1].lChild);
  HeadInfo childHeader;
  firstChild.getHeader(&childHeader);
  childHeader.pblock=rightBlkNum;
  firstChild.setHeader(&childHeader);

  //then the rChild of every entry in the right block
  for(int i=MAX_KEYS_INTERNAL/2+1;i<=MAX_KEYS_INTERNAL;i++){
    //declare an instance of BlockBuffer to access the child block using
    //constructor 2
    BlockBuffer childBlk(internalEntries[i].rChild);

    //update pblock of the block to rightBlkNum using BlockBuffer::getHeader()
    //and BlockBuffer::setHeader()
    childBlk.getHeader(&childHeader);
    childHeader.pblock=rightBlkNum;
    childBlk.setHeader(&childHeader);
  }

  return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId,char attrName[ATTR_SIZE],Attribute attrVal,int lChild,int rChild){

  //get the attribute cache entry corresponding to attrName
  //using AttrCacheTable::getAttrCatEntry()
  AttrCatEntry attrCatEntry;
  AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

  //declare newRootBlk, an instance of IndInternal using appropriate constructor
  //to allocate a new internal index block on the disk
  IndInternal newRootBlk;

  int newRootBlkNum=newRootBlk.getBlockNum();

  if(newRootBlkNum==E_DISKFULL){
    //failed to obtain an empty internal index block because the disk is full

    //using bPlusDestroy(), destroy the right subtree, rooted at rChild
    //this corresponds to the tree built up till now that has not yet been
    //connected to the existing B+ Tree
    bPlusDestroy(rChild);

    return E_DISKFULL;
  }

  //update the header of the new block with numEntries=1 using
  //BlockBuffer::getHeader() and BlockBuffer::setHeader()
  HeadInfo newRootHeader;
  newRootBlk.getHeader(&newRootHeader);
  newRootHeader.blockType=IND_INTERNAL;
  newRootHeader.numEntries=1;
  newRootHeader.pblock=-1;
  newRootBlk.setHeader(&newRootHeader);

  //create a struct InternalEntry with lChild, attrVal and rChild from the
  //arguments and set it as the first entry in newRootBlk using IndInternal::setEntry()
  InternalEntry rootEntry;
  rootEntry.lChild=lChild;
  rootEntry.attrVal=attrVal;
  rootEntry.rChild=rChild;
  newRootBlk.setEntry(&rootEntry,0);

  //declare BlockBuffer instances for the `lChild` and `rChild` blocks using
  //appropriate constructor and update the pblock of those blocks to `newRootBlkNum`
  //using BlockBuffer::getHeader() and BlockBuffer::setHeader()
  BlockBuffer leftChild(lChild);
  BlockBuffer rightChild(rChild);

  HeadInfo childHeader;

  leftChild.getHeader(&childHeader);
  childHeader.pblock=newRootBlkNum;
  leftChild.setHeader(&childHeader);

  rightChild.getHeader(&childHeader);
  childHeader.pblock=newRootBlkNum;
  rightChild.setHeader(&childHeader);

  //update rootBlock=newRootBlkNum for the entry corresponding to `attrName`
  //in the attribute cache using AttrCacheTable::setAttrCatEntry()
  attrCatEntry.rootBlock=newRootBlkNum;
  AttrCacheTable::setAttrCatEntry(relId,attrName,&attrCatEntry);

  return SUCCESS;
}

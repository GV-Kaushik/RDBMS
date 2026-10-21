#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(char blockType)
{
    // allocate a block on the disk and a buffer in memory to hold the new block of
    // given type using getFreeBlock function and get the return error codes if any.
    // blockType is 'R', 'I' or 'L' - translate to the int constants
    // getFreeBlock/setBlockType expect.
    int type;
    if (blockType == 'R')
    {
        type = REC;
    }
    else if (blockType == 'I')
    {
        type = IND_INTERNAL;
    }
    else
    {
        type = IND_LEAF;
    }

    int ret = this->getFreeBlock(type);

    // set the blockNum field of the object to that of the allocated block
    // number if the method returned a valid block number,
    // otherwise set the error code returned as the block number.
    this->blockNum = ret;

    // (The caller must check if the constructor allocatted block successfully
    // by checking the value of block number field.)
}

BlockBuffer::BlockBuffer(int blockNum)
{
    // initialise this.blockNum with the argument
    this->blockNum = blockNum;
}

RecBuffer::RecBuffer() : BlockBuffer('R') {}
// call parent non-default constructor with 'R' denoting record block.

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType) {}
// call the corresponding parent constructor
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum) {}
// call the corresponding parent constructor
IndInternal::IndInternal() : IndBuffer('I') {}
// call the corresponding parent constructor
// 'I' used to denote IndInternal.
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum) {}
// call the corresponding parent constructor
IndLeaf::IndLeaf() : IndBuffer('L') {} // this is the way to call parent non-default constructor.
// 'L' used to denote IndLeaf.
IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum) {}
// this is the way to call parent non-default constructor.

int BlockBuffer::getBlockNum()
{

    // return corresponding block number.
    return this->blockNum;
}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head)
{
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret; // return any errors that might have occured in the process
    }

    // populate the numEntries, numAttrs and numSlots fields in *head
    memcpy(&head->blockType, bufferPtr, 4);
     memcpy(&head->pblock, bufferPtr + 4, 4);
    memcpy(&head->numSlots, bufferPtr + 24, 4);
    memcpy(&head->numEntries, bufferPtr + 16 /* fill this */, 4);
    memcpy(&head->numAttrs, bufferPtr + 20 /* fill this */, 4);
    memcpy(&head->rblock, bufferPtr + 12 /* fill this */, 4);
    memcpy(&head->lblock, bufferPtr + 8 /* fill this */, 4);

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head)
{

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    // loadBlockAndGetBufferPtr(&bufferPtr).
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    // return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }

    // cast bufferPtr to type HeadInfo*
    // bufferPtr is just raw bytes (unsigned char*) with no structure info.
    // this cast doesn't move/copy anything - it just relabels the same
    // memory as a HeadInfo struct, so we can access fields like
    // bufferHeader->numSlots instead of manually computing byte offsets.
    // works because the block on disk was written in this exact layout.
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    bufferHeader->blockType = head->blockType;
    bufferHeader->pblock = head->pblock;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numSlots = head->numSlots;

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }

    // return SUCCESS;
    return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{
    struct HeadInfo head;

    // get the header using this.getHeader() function
    int result = this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
       - each record will have size attrCount * ATTR_SIZE
       - slotMap will be of size slotCount
    */
    int recordSize = attrCount * ATTR_SIZE;
    int offset = HEADER_SIZE + slotCount + (recordSize * slotNum);
    unsigned char *slotPointer = bufferPtr + offset /* calculate buffer + offset */;

    // load the record into the rec data structure
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    // return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }

    /* get the header of the block using the getHeader() function */
    struct HeadInfo head;
    this->getHeader(&head);

    // get number of attributes in the block.
    int attrCount = head.numAttrs;

    // get the number of slots in the block.
    int slotCount = head.numSlots;

    // if input slotNum is not in the permitted range return E_OUTOFBOUND.
    if (slotNum < 0 || slotNum >= slotCount)
    {
        return E_OUTOFBOUND;
    }

    /* offset bufferPtr to point to the beginning of the record at required
       slot. the block contains the header, the slotmap, followed by all
       the records. so, for example,
       record at slot x will be at bufferPtr + HEADER_SIZE + (x*recordSize)
       copy the record from `rec` to buffer using memcpy
       (hint: a record will be of size ATTR_SIZE * numAttrs)
    */
    int recordSize = attrCount * ATTR_SIZE;
    unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + (recordSize * slotNum);
    memcpy(slotPointer, rec, recordSize);

    // update dirty bit using setDirtyBit()
    /* (the above function call should not fail since the block is already
       in buffer and the blockNum is valid. If the call does fail, there
       exists some other issue in the code) */
    StaticBuffer::setDirtyBit(this->blockNum);

    // return SUCCESS
    return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
    /* check whether the block is already present in the buffer
       using StaticBuffer.getBufferNum() */
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    // if present (!=E_BLOCKNOTINBUFFER),
    if (bufferNum != E_BLOCKNOTINBUFFER)
    {
        // set the timestamp of the corresponding buffer to 0 and increment the
        // timestamps of all other occupied buffers in BufferMetaInfo.
        for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
        {
            if (StaticBuffer::metainfo[bufferIndex].free == false)
            {
                StaticBuffer::metainfo[bufferIndex].timeStamp++;
            }
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }
    // else
    else
    {
        // get a free buffer using StaticBuffer.getFreeBuffer()
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        // if the call returns E_OUTOFBOUND, return E_OUTOFBOUND here as
        // the blockNum is invalid
        if (bufferNum == E_OUTOFBOUND)
        {
            return E_OUTOFBOUND;
        }

        // Read the block into the free buffer using readBlock()
        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }

    // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
    *buffPtr = StaticBuffer::blocks[bufferNum];

    // return SUCCESS;
    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType)
{

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    // return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }

    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    // *((int32_t *)bufferPtr) = blockType;
    //
    // bufferPtr is unsigned char* -> writes 1 byte at a time.
    // casting to int32_t* relabels the SAME address as "read/write
    // 4 bytes at once, as one integer" - no data moved, just a
    // different sized "pen" on the same starting spot.
    // the * then dereferences it: "go to that address and write here".
    // so this writes blockType (4 bytes) into the first 4 bytes of
    // the block in one shot, instead of writing only 1 byte (which
    // is what *bufferPtr = blockType would do, truncating the int).
    // blockType is literally the first field of HeadInfo, so this is
    // equivalent to bufferHeader->blockType = blockType in setHeader().
    *((int32_t *)bufferPtr) = blockType;

    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed
    // return the returned value from the call
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }

    // return SUCCESS
    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType)
{

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    // a free block is marked UNUSED_BLK in blockAllocMap.
    int freeBlockNum = -1;
    for (int i = 0; i < DISK_BLOCKS; i++)
    {
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK)
        { // doubt
            freeBlockNum = i;
            break;
        }
    }

    // if no block is free, return E_DISKFULL.
    if (freeBlockNum == -1)
    {
        return E_DISKFULL;
    }

    // set the object's blockNum to the block number of the free block.
    this->blockNum = freeBlockNum;

    // find a free buffer using StaticBuffer::getFreeBuffer() .
    // this loads a buffer slot for this new block (buffer index itself
    // isn't needed here - getFreeBuffer() updates StaticBuffer's metainfo
    // internally; later calls that load this blockNum will find it there).
    int bufferNum = StaticBuffer::getFreeBuffer(freeBlockNum);

    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHeader() function.
    struct HeadInfo head;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;
    this->setHeader(&head);

    // update the block type of the block to the input block type using setBlockType().
    this->setBlockType(blockType);

    // return block number of the free block.
    return freeBlockNum;
}

void BlockBuffer::releaseBlock()
{

    // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
    if (this->blockNum == INVALID_BLOCKNUM)
    {
        return;
    }

    // else
    /* get the buffer number of the buffer assigned to the block
       using StaticBuffer::getBufferNum().
       (this function return E_BLOCKNOTINBUFFER if the block is not
       currently loaded in the buffer)
        */
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    // if the block is present in the buffer, free the buffer
    // by setting the free flag of its StaticBuffer::tableMetaInfo entry
    // to true.
    if (bufferNum != E_BLOCKNOTINBUFFER)
    {
        StaticBuffer::metainfo[bufferNum].free = true;
    }

    // free the block in disk by setting the data type of the entry
    // corresponding to the block number in StaticBuffer::blockAllocMap
    // to UNUSED_BLK.
    StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;

    // set the object's blockNum to INVALID_BLOCK (-1)
    this->blockNum = INVALID_BLOCKNUM;
}

// IMP POINT
// BlockBuffer::releaseBlock() takes a block number and frees that block
// in the buffer and in the block allocation map, so the block can be reused

// higher layers must not touch the buffer directly
// to work on a block, first create a BlockBuffer/RecBuffer object for it
// and do everything through that object's methods
// (only exception: StaticBuffer::getStaticBlockType())

// releaseBlock() marks the block free on disk and sets the object's
// block number to INVALID_BLK, so it no longer points to any block

// after release the object is unusable, any method called on it returns an error
// so never use a BlockBuffer/RecBuffer object after releasing its block
// read whatever is needed (like rblock) from the header BEFORE releasing

/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap)
{
    unsigned char *bufferPtr;

    // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    struct HeadInfo head;
    // get the header of the block using getHeader() function
    int result = this->getHeader(&head);
    if (result != SUCCESS)
    {
        return result;
    }

    int slotCount = head.numSlots /* number of slots in block from header */;

    // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
    memcpy(slotMap, slotMapInBuffer, slotCount);

    return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap)
{
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using
       loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    // return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }

    // get the header of the block using the getHeader() function
    struct HeadInfo head;
    this->getHeader(&head);

    int numSlots = head.numSlots;

    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
    memcpy(slotMapInBuffer, slotMap, numSlots);

    // update dirty bit using StaticBuffer::setDirtyBit
    // if setDirtyBit failed, return the value returned by the call
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }

    // return SUCCESS
    return SUCCESS;
}

int IndInternal::getEntry(void *ptr, int indexNum)
{
    // if the indexNum is not in the valid range of [0, MAX_KEYS_INTERNAL-1]
    //     return E_OUTOFBOUND.
    if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
    {
        return E_OUTOFBOUND;
    }
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }
    /*
    - copy the entries from the indexNum`th entry to *internalEntry
    - make sure that each field is copied individually as in the following code
    - the lChild and rChild fields of InternalEntry are of type int32_t
    - int32_t is a type of int that is guaranteed to be 4 bytes across every
      C++ implementation. sizeof(int32_t) = 4
    */
    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * (sizeof(int) + ATTR_SIZE) )         [why?]
       from bufferPtr */
    // why: the block is stored as  header | ptr0 key0 ptr1 key1 ptr2 key2 ... key99 ptr100
    // the pointer between two keys is shared, it is rChild of entry i and lChild of entry i+1
    // and it is stored only once. so going from one entry to the next skips just
    // one pointer + one key = 4 + 16 = 20 bytes
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);
    // ptr is the address of the start of the struct InternalEntry given by the caller
    // we cant copy the whole struct in one go, because on disk the entry is packed
    // (lChild at 0, attrVal at 4, rChild at 20) but in memory the compiler adds padding
    // so attrVal and rChild are at different offsets inside the struct
    // so each field is copied separately to its own address
    // (struct InternalEntry *)ptr -> tells the compiler the layout of the memory at ptr
    // ->field                     -> goes to that field (gives the value stored there)
    // &                           -> address of that field, which is where memcpy has to write
    // entryPtr has no & because it is already an address (the source in the buffer)
    memcpy(&(((struct InternalEntry *)ptr)->lChild), entryPtr, sizeof(int32_t));
    memcpy(&(((struct InternalEntry *)ptr)->attrVal), entryPtr + 4, sizeof(Attribute));
    memcpy(&(((struct InternalEntry *)ptr)->rChild), entryPtr + 20, sizeof(int32_t));
    // return SUCCESS.
    return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum)
{

    // if the indexNum is not in the valid range of [0,MAX_KEYS_INTERNAL-1]
    if (indexNum < 0 || indexNum >= MAX_KEYS_INTERNAL)
    {
        return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block
    // using loadBlockAndGetBufferPtr(&bufferPtr)
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr)!=SUCCESS
    // return the value returned by the call
    if (ret != SUCCESS)
    {
        return ret;
    }

    // typecast the void pointer to an internal entry pointer
    struct InternalEntry *internalEntry = (struct InternalEntry *)ptr;

    // the indexNum'th entry will begin at an offset of
    // HEADER_SIZE+(indexNum*(sizeof(int)+ATTR_SIZE)) from bufferPtr
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * 20);

    // copying each field separately, lChild and rChild are int32_t (4 bytes)
    memcpy(entryPtr, &(internalEntry->lChild), 4);
    memcpy(entryPtr + 4, &(internalEntry->attrVal), ATTR_SIZE);
    memcpy(entryPtr + 20, &(internalEntry->rChild), 4);

    // update dirty bit using setDirtyBit()
    // if setDirtyBit failed, return the value returned by the call

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }

    // return SUCCESS
    return SUCCESS;
}

int IndLeaf::getEntry(void *ptr, int indexNum)
{
    // if the indexNum is not in the valid range of [0, MAX_KEYS_LEAF-1]
    //     return E_OUTOFBOUND.
    if (indexNum < 0 || indexNum >= MAX_KEYS_LEAF)
    {
        return E_OUTOFBOUND;
    }
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
    //     return the value returned by the call.
    if (ret != SUCCESS)
    {
        return ret;
    }
    // copy the indexNum'th Index entry in buffer to memory ptr using memcpy
    /* the indexNum'th entry will begin at an offset of
       HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE)  from bufferPtr */
    // why: every leaf entry is a full struct Index = attrVal(16B) + block(4B) + slot(4B) + unused(8B) = 32B
    // nothing is shared between leaf entries, so they are just placed one after another,
    // each LEAF_ENTRY_SIZE bytes. HEADER_SIZE skips the header and
    // indexNum*LEAF_ENTRY_SIZE skips the entries before ours
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);
    // ptr is the address of the start of the struct Index given by the caller
    // here we want to write the whole struct, and the start of the struct is exactly ptr
    // so ptr is used as it is (no & needed). the disk layout is same as the struct
    // in memory (no padding), so one memcpy of the whole entry is enough
    memcpy((struct Index *)ptr, entryPtr, LEAF_ENTRY_SIZE);
    // return SUCCESS
    return SUCCESS;
}

int IndLeaf::setEntry(void *ptr, int indexNum)
{

    // if the indexNum is not in the valid range of [0,MAX_KEYS_LEAF-1]
    if (indexNum < 0 || indexNum >= MAX_KEYS_LEAF)
    {
        return E_OUTOFBOUND;
    }

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block
    // using loadBlockAndGetBufferPtr(&bufferPtr)
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr)!=SUCCESS
    // return the value returned by the call
    if (ret != SUCCESS)
    {
        return ret;
    }

    // typecast the void pointer to an index pointer
    struct Index *leafEntry = (struct Index *)ptr;

    // the indexNum'th entry will begin at an offset of
    // HEADER_SIZE+(indexNum*LEAF_ENTRY_SIZE) from bufferPtr
    unsigned char *entryPtr = bufferPtr + HEADER_SIZE + (indexNum * LEAF_ENTRY_SIZE);

    // copying each field separately since the struct alignment in memory
    // can be different from the layout on disk
    memcpy(entryPtr, &(leafEntry->attrVal), ATTR_SIZE);
    memcpy(entryPtr + ATTR_SIZE, &(leafEntry->block), sizeof(int32_t));
    memcpy(entryPtr + ATTR_SIZE + 4, &(leafEntry->slot), sizeof(int32_t));
    memcpy(entryPtr + ATTR_SIZE + 8, leafEntry->unused, 8);

    // update dirty bit using setDirtyBit()
    // if setDirtyBit failed, return the value returned by the call
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }

    // return SUCCESS
    return SUCCESS;
}
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{
    double diff;
    if (attrType == STRING)
        diff = strcmp(attr1.sVal, attr2.sVal);
    else
        diff = attr1.nVal - attr2.nVal;

    if (diff > 0)
        return 1;
    if (diff < 0)
        return -1;
    return 0;
}

#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
     RecId prevRecId;
     int result=RelCacheTable::getSearchIndex(relId,&prevRecId);
     if(result!=SUCCESS)return {-1,-1};//is thsi crct?

    // let block and slot denote the record id of the record being currently checked
    int block,slot;

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relCatBuf;
        int result2=RelCacheTable::getRelCatEntry(relId,&relCatBuf);
        if(result2!=SUCCESS)return {-1,-1};
        
        // block = first record block of the relation
        // slot = 0
        block=relCatBuf.firstBlk;
        slot=0;

    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        // block = search index's block
        // slot = search index's slot + 1
        block=prevRecId.block;
        slot=prevRecId.slot+1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
           RecBuffer recBlock(block);
           HeadInfo recBlockHeader;

        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        int result3=recBlock.getHeader(&recBlockHeader);
        if(result3!= SUCCESS)return {-1,-1};
        int attrs=recBlockHeader.numAttrs;
        Attribute recBlockrecord[attrs];
        int result4=recBlock.getRecord(recBlockrecord,slot); // this shld be done actual checking slots bound?
        ///if(result4!=SUCCESS){ slot++; continue; } // claude gave


        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char slotMap[recBlockHeader.numSlots];
         int result5=recBlock.getSlotMap(slotMap);
         if(result5!=SUCCESS)return {-1,-1};

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot>=recBlockHeader.numSlots){
            // update block = right block of block
            // update slot = 0
            block=recBlockHeader.rblock;
            slot=0;
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotMap[slot]==SLOT_UNOCCUPIED){
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }
        

        // compare record's attribute value to the the given attrVal as below:

        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        AttrCatEntry attrCatBuf;
        int result6 = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
        if(result6!= SUCCESS) {
         return {-1, -1};
        }
        int offset=attrCatBuf.offset;

        /* use the attribute offset to get the value of the attribute from
           current record */
       // wt do from here 
       Attribute recAttrVal=recBlockrecord[offset];

        int cmpVal;  // will store the difference between the attributes
        // set cmpVal using compareAttrs()
        cmpVal=compareAttrs(recAttrVal,attrVal,attrCatBuf.attrType);

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
           RecId searchIndex={block,slot};
            RelCacheTable::setSearchIndex(relId,&searchIndex);

            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
       RelCacheTable::resetSearchIndex(RELCAT_RELID);
    


    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal,newName);

    // search the relation catalog for an entry with "RelName" = newRelationName
    char relNameAttr[ATTR_SIZE] = "RelName";
    RecId recId=BlockAccess::linearSearch(RELCAT_RELID,relNameAttr,newRelationName,EQ);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if(recId.block!=-1 ||recId.slot!=-1){
        return E_RELEXIST;
    }



    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal,oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
     recId=BlockAccess::linearSearch(RELCAT_RELID,relNameAttr,oldRelationName,EQ);


    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
     if(recId.block==-1 && recId.slot==-1){
        return E_RELNOTEXIST;
    }

   
    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    RecBuffer relCatBlock(recId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord,recId.slot);

    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,newName);

    // set back the record value using RecBuffer.setRecord
    relCatBlock.setRecord(relCatRecord, recId.slot);

    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord
    int numberOfAttributes=relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    for (int i=0;i<numberOfAttributes;i++) {
        RecId attrCatRecId=BlockAccess::linearSearch(ATTRCAT_RELID,relNameAttr,oldRelationName,EQ);

        RecBuffer attrCatBlock(attrCatRecId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);

        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,newName);

        attrCatBlock.setRecord(attrCatRecord,attrCatRecId.slot);
    }

    return SUCCESS;

}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal,relName);

    // Search for the relation with name relName in relation catalog using linearSearch()
    char relNameCol[ATTR_SIZE]="RelName";
    RecId relRecId=BlockAccess::linearSearch(RELCAT_RELID,relNameCol,relNameAttr,EQ);

    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    if(relRecId.block==-1 && relRecId.slot==-1){
        return E_RELNOTEXIST;
    }

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1,-1};
    Attribute attrRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while(true){
        // linear search on the attribute catalog for RelName = relNameAttr
        RecId currRecId=BlockAccess::linearSearch(ATTRCAT_RELID,relNameCol,relNameAttr,EQ);

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        if(currRecId.block==-1 && currRecId.slot==-1){
            break;
        }

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        RecBuffer currBlock(currRecId.block);
        currBlock.getRecord(attrRecord,currRecId.slot);

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        // NOTE: no break here - we must keep scanning the rest of this
        // relation's attributes even after finding oldName, otherwise we
        // could miss a later attribute that already has newName and end
        // up creating a duplicate attribute name
        if(strcmp(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)==0){
            attrToRenameRecId.block=currRecId.block;
            attrToRenameRecId.slot=currRecId.slot;
        }

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
        if(strcmp(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0){
            return E_ATTREXIST;
        }
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    if(attrToRenameRecId.block==-1 && attrToRenameRecId.slot==-1){
        return E_ATTRNOTEXIST;
    }

    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    RecBuffer targetBlock(attrToRenameRecId.block);
    Attribute targetRecord[ATTRCAT_NO_ATTRS];
    targetBlock.getRecord(targetRecord,attrToRenameRecId.slot);

    //   update the AttrName of the record with newName
    strcpy(targetRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);

    //   set back the record with RecBuffer.setRecord
    targetBlock.setRecord(targetRecord,attrToRenameRecId.slot);

    return SUCCESS;
}
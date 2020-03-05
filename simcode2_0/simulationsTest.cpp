#include<bits/stdc++.h>
#include "blockBuffer.h"
#include "../define/constants.h"
#define K 5
#define L 24



using namespace std;
int getRecordSim(char * relName);
int getFirstBlockOfRec(char * relName);


/****************************************Static Buffer**************************************************/


/****************************************BLock Buffer**************************************************/

BlockBuffer::BlockBuffer(int blockNum){

//cout <<"Inside BlockBuffer(int blockNum) constructor\n";

// set the blockNum field of the object to input argument.
this->blockNum = blockNum;

// copy the block into buffer using getBlock function.
/***Note: disregard the return type***/
/***To be updated: make it camel case***/
getBlock();

//cout <<"Exiting BlockBuffer(int blockNum) constructor\n";
}

BlockBuffer::BlockBuffer(char blockType){
/***Note: disregard the return type***/

// allocate a block on the disc and a buffer in memory to hold the new block of given type using getFreeBlock function.

// set the blockNum field of the object to that of the allocated block number.
int type;
if(blockType == 'R')
	type = REC;
else if(blockType == 'I')
	type = IND_INTERNAL;
else
	type = IND_LEAF;
	
this->blockNum = getFreeBlock(type);
}

unsigned char * BlockBuffer::getBufferPtr(){
	//find the buffer index of the block using getBlock()
	
	//cout <<"Entering getBufferPtr\n"; 
	int bufferIndex = getBlock();
	
	//cout <<"Exiting getBufferPtr "<<bufferIndex<<"\n"; 
	// return the pointer to this buffer (blocks[bufferIndex]).
	return StaticBuffer::blocks[bufferIndex];
}

void BlockBuffer::getHeader(struct HeadInfo *head){	
// get the starting address of the buffer containing the block using getBufferPtr. 
	 unsigned char * startOfBuffer = getBufferPtr();
	        
//copy the header of block to the memory location pointed to by the argument head using appropriate type casting
	*head = *((struct HeadInfo*) (startOfBuffer));

}

void BlockBuffer::setHeader(struct HeadInfo *head){
// get the starting address of the buffer containing the block using getBufferPtr.
	unsigned char * startOfBuffer = getBufferPtr();
	
//copy the contents of the memory location pointed to by the argument head to the header of block using appropriate type casting
	*((struct HeadInfo*) (startOfBuffer)) = *head;
	StaticBuffer::setDirtyBit(this->blockNum);

}

int BlockBuffer::getBlock(){
     //check whether the block is already present in the buffer using StaticBuffer.getBufferNum() .
		int bufferIndex = StaticBuffer::getBufferNum(this->blockNum);
		
		////cout <<"Inside getBlock() bufferIndex "<< bufferIndex<<endl;
		
     //if present, set the timestamp of the corresponding buffer to 0 and increment the timpestamps of all other occupied buffers in the BufferMetaInfo.
     if(bufferIndex != FAILURE){

     	for (int i = 0;i<32;++i) {
     		if(StaticBuffer::metainfo[i].timeStamp != -1)
     			StaticBuffer::metainfo[i].timeStamp++;
     	}
     	StaticBuffer::metainfo[bufferIndex].timeStamp=0;
     }
     
     //if not present, get a free buffer using StaticBuffer.getFreeBuffer() and read the block into the free buffer using readBlock().
     else {

     	bufferIndex = StaticBuffer::getFreeBuffer(this->blockNum);
     	//cout <<"Loaded to buffer " <<bufferIndex<<endl;
     	Disk::readBlock((void *)StaticBuffer::blocks[bufferIndex],this->blockNum);
     }
   ////cout <<"Exiting getBlock()\n";
   
   return bufferIndex;
}

int BlockBuffer::getFreeBlock(int blockType){
	//cout <<"Entering getFreeBlock\n";
//iterate through the StaticBuffer.blockAllocMap and find the index of a free block in the Disk::
	this->blockNum=-1;
	for(int i=0;i<8192;++i)
		if(((int32_t)(StaticBuffer::blockAllocMap[i]))==myUNUSED){
		    //cout<<(int32_t)(StaticBuffer::blockAllocMap[i])<<endl;
			this->blockNum = i;
			break;
		}
		//cout<<this->blockNum<<endl;
	
   
//if no block is free, return FAILURE.
	if(this->blockNum == -1)
		return FAILURE;
   
 //  cout<<"Current blockNum "<<this->blockNum<<endl;
//find a free buffer using StaticBuffer.getFreeBuffer() 
	int bufferIndex = StaticBuffer::getFreeBuffer(this->blockNum);

/*//update StaticBuffer.blockAllocMap.
	StaticBuffer::blockAllocMap[blockNum] = blockType;*/
   
//update the block type of the block to the input block type using Blockbuffer.setBlockType().
	setBlockType(blockType);
	
   ////cout <<"Exiting getFreeBlock "<<this->blockNum<<" "<<blockNum<<" "<<bufferIndex<<"\n";
//return block number of the free block.
	return this->blockNum;

}

int BlockBuffer::getBlockNum(){

//return corresponding block number
	return this->blockNum;
}

int BlockBuffer::getBlockType(int bufferIndex){

//blocks[bufferIndex][0] gives the staring address of the buffer
unsigned char * startOfBuffer = StaticBuffer::blocks[bufferIndex];

//retrieve the first 4 bytes of the buffer that stores the block type.
	int blockType = *((int32_t*)(startOfBuffer));
	return blockType;
	
}

void BlockBuffer::setBlockType(int blockType){
//cout <<"Entering setBlockType\n";		
//find the starting address of the buffer using BlockBuffer.getBufferPtr()
unsigned char * startOfBuffer = getBufferPtr();

//store the given block type in the first 4 bytes of the buffer
*((int32_t*)(startOfBuffer)) = blockType;

//update the BlockAllocMap 
StaticBuffer::blockAllocMap[this->blockNum] = blockType;
StaticBuffer::setDirtyBit(this->blockNum);
	
//cout <<"Exiting setBlockType\n";
}


/****************************************Record Buffer**************************************************/
RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum){}
RecBuffer::RecBuffer() : BlockBuffer('R'){}

void RecBuffer::getSlotMap(unsigned char *slotMap){
	//cout <<"Entering getSlotMap\n";
	unsigned char *bufferPtr = getBufferPtr();
	int numOfSlots=*((int32_t*) (bufferPtr + 6*4));
	memcpy(slotMap, bufferPtr + 32, numOfSlots);
	
	//cout <<"Exiting getSlotMap\n";
} 
void RecBuffer::setSlotMap(unsigned char *slotMap){
	unsigned char *bufferPtr = getBufferPtr();
	int numOfSlots=*((int32_t*) (bufferPtr + 6*4));
	StaticBuffer::setDirtyBit(this->blockNum);
/*	for(int i=0; i<32; i++)
	    if(this->blockNum == staticBuffer.metainfo[i].blockNum)
	        staticBuffer.metainfo[i].dirty = true;*/ 
	memcpy(bufferPtr + 32, slotMap, numOfSlots);
	return;
}

int RecBuffer::getRecord(union Attribute *rec,int slotNum){ 

	//cout <<"Entering getRecord\n";
	unsigned char* bufferPtr = getBufferPtr();
	int numOfAttrib=*((int32_t*) (bufferPtr + 5*4));
	int numOfSlots=*((int32_t*) (bufferPtr + 6*4));
	if(slotNum < 0 || slotNum > numOfSlots - 1)
		return E_OUTOFBOUND;  
	unsigned char *slotMap = new unsigned char;
	getSlotMap(slotMap);
	if((int32_t*)(slotMap + slotNum) == 0)
	    return E_FREESLOT;
	//memcpy((void*)(bufferPtr + 32 + numOfSlots + (slotNum*numOfAttrib)*ATTR_SIZE), (void*)rec, numOfAttrib*ATTR_SIZE);
	
	memcpy((void*)rec, (void*)(bufferPtr + 32 + numOfSlots + (slotNum*numOfAttrib)*ATTR_SIZE), numOfAttrib*ATTR_SIZE);
	
	//cout <<"Exiting getRecord with blockNum "<<this->blockNum<<endl;
	return SUCCESS;
	
}

int RecBuffer::setRecord(union Attribute *rec,int slotNum){  //return type to be updated in the code 

	//cout <<"Entering setRecord\n";
	unsigned char* bufferPtr = getBufferPtr();
	int numOfAttrib=*((int32_t*) (bufferPtr + 5*4));
	int numOfSlots=*((int32_t*) (bufferPtr + 6*4));
	
	//cout <<"Attr && Slots "<<numOfAttrib<<" "<<numOfSlots<<"\n";
	if(slotNum < 0 || slotNum > numOfSlots - 1)
		return E_OUTOFBOUND;
	
	//memcpy((void*)rec, (void*)(bufferPtr + 32 + numOfSlots +(slotNum*numOfAttrib)*ATTR_SIZE), numOfAttrib*ATTR_SIZE);
	
	memcpy((void*)(bufferPtr + 32 + numOfSlots +(slotNum*numOfAttrib)*ATTR_SIZE), (void*)rec, numOfAttrib*ATTR_SIZE);
	
	StaticBuffer::setDirtyBit(this->blockNum);//update dirty bit
	//cout <<"Exiting setRecord "<<numOfAttrib<<" "<<numOfSlots<<"\n";
	return SUCCESS;
}


/**********************************************IND BUFFER***********************************************************************/
IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum){}
IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType){} //to update camelCase in "char blocktype" 

/**********************************************Class IndInternal***************************************************************/
IndInternal::IndInternal() : IndBuffer('I'){}
IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum){}

int IndInternal::getEntry(void *ptr, int indexNum){
    unsigned char* bufferPtr = getBufferPtr();
    int numOfEntries = *((int32_t*) (bufferPtr + 4*4));
    if(indexNum<0 || indexNum > numOfEntries - 1) //to be updated 
        return E_OUTOFBOUND;
    struct InternalEntry entry;
	entry=*((struct InternalEntry*) (bufferPtr + 32 + indexNum*20)); //need to double check as snrs code was different
	(*(struct InternalEntry*)ptr) = entry;
    return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum){
    unsigned char* bufferPtr = getBufferPtr();
    int numOfEntries = *((int32_t*) (bufferPtr + 4*4));
    if(indexNum<0 || indexNum > numOfEntries - 1) //to be updated 
        return E_OUTOFBOUND;
    struct InternalEntry Entry;
    Entry = (*(struct InternalEntry*)ptr);
	*((struct InternalEntry*) (bufferPtr + 32 + indexNum*20)) = Entry;
	StaticBuffer::setDirtyBit(this->blockNum);//update dirty bit
    return SUCCESS;

}
/******************************************************IND_LEAF**********************************************************************/
IndLeaf::IndLeaf() : IndBuffer('L'){}

IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum){}

int IndLeaf::getEntry(void *ptr, int indexNum){

// get the starting address of the buffer containing the block using BlockBuffer.getBufferPtr().
/***To be updated: no need for BlockBuffer.***/
unsigned char * startOfBuffer = getBufferPtr();

// if the indexNum is not in range of 0-(#Entries(in block)+1), return E_OUTOFBOUND

int numEntries = *((int32_t *)(startOfBuffer + 4 * 4));

if((indexNum<0) || indexNum > numEntries)
	return E_OUTOFBOUND;

// copy the indexNum'th Index entry in block to memory ptr(ptr can be type casted appropriately if needed). 
	struct Index Entry;
	Entry=*((struct Index*) (startOfBuffer + 32 + indexNum*32));
	*((struct Index*)ptr) = Entry;
	
    
// return SUCCESS.
	return SUCCESS;

}

int IndLeaf::setEntry(void *ptr, int indexNum){						
    unsigned char* bufferPtr = getBufferPtr();
    int numOfEntries = *((int32_t*) (bufferPtr + 4*4));
    if(indexNum<0 || indexNum > numOfEntries - 1) //to be updated 
        return E_OUTOFBOUND;
    struct Index Entry;
    Entry = (*(struct Index*)ptr);
	*((struct Index*) (bufferPtr + 32 + indexNum*32)) = Entry;
	StaticBuffer::setDirtyBit(this->blockNum);//update dirty bit
    return SUCCESS;
}



///////////////////////////////////////SIMULATIONS////////////////////////////////////////////////////////////////////////////////

struct recBlock {
	int32_t blockType;
	int32_t pblock;
	int32_t lblock;
	int32_t rblock;
	int32_t numEntries;
	int32_t numAttrs; 
	int32_t numSlots;
	unsigned char reserved[4];
	unsigned char slotMap[L];
	struct record {
	Attribute recordElem[K][16];	
    }recordList[L];
    unsigned char unused[72];	
};

struct internalIndexBlock {
	int32_t blockType;
	int32_t pblock;
	int32_t lblock;
	int32_t rblock;
	int32_t numEntries;
	int32_t numAttrs; 
	int32_t numSlots;
	unsigned char reserved[4];
	struct internalIndex{
    unsigned char childPtr[4];
    unsigned char attrVal[16];
    }intIndex[100];
	unsigned char childPtr100[4];
	unsigned char unused[11];
};

struct leafIndexBlock {
	int32_t blockType;
	int32_t pblock;
	int32_t lblock;
	int32_t rblock;
	int32_t numEntries;
	int32_t numAttrs; 
	int32_t numSlots;
	unsigned char reserved[4];
	struct internalLeaf{
    unsigned char attrVal[16];
    unsigned char blockNum[4];
    unsigned char slotNum[4];
    unsigned char unused[8];
    }intIndex[63];	
};

int getFormatString(char* relationName, char *format)
{
    if(strcmp(relationName,"relationCat")==0)
        strcpy(format,"siiiii");
    else if(strcmp(relationName,"attributeCat")==0)
        strcpy(format,"sssiii");
    else{
        int blockNum=5,len =0;
        do{
	    class RecBuffer *attrCat= new RecBuffer(blockNum);
	    union Attribute *attrCatAttrs = new union Attribute[6];
	    struct HeadInfo * attrCatHead = new struct HeadInfo ();
	    unsigned char slotMap[20];
	    attrCat->getSlotMap(slotMap);
	    int i;
	    //cout <<"loop start\n";
	    for(i=0;i<20;i++)
	        if(slotMap[i]=='1')
	        {
	            attrCat->getRecord(attrCatAttrs,i);
	            if(strcmp(attrCatAttrs[0].strval,relationName )==0)
	            {
	                if(strcmp(attrCatAttrs[2].strval,"String")==0)
	                    format[len++]='s';
	                else if(strcmp(attrCatAttrs[2].strval,"Integer")==0)
	                    format[len++]='i';
	                else
	                    format[len++]='f';
	            }
	        }
	    //cout <<"loop end\n";
	    attrCat->getHeader(attrCatHead);
	    blockNum = attrCatHead->rblock;
	
	    //getRecordSim((char*)"attributeCat");
	    //cout << "Format blockNum "<< blockNum<<endl;
	    }while(blockNum!=-1);
	    format[len]='\0';
	    }
	return 1;
}

int setRecordSim(int numAttr, char * format, int read, ...) { 
//returns the blockNum in which the record is stored
    
    //testing RecBuffer 
    class RecBuffer *recRec= new RecBuffer();
    
    //setting up header
    struct HeadInfo * recHead = new struct HeadInfo ();
    recHead-> blockType = REC;
	recHead-> pblock=-1;
	recHead-> lblock=-1;//need to be changed
	recHead-> rblock=-1;//need to be changed
	recHead-> numEntries=1;//entry added below
	recHead-> numAttrs=numAttr; 
	recHead-> numSlots=floor(2016/(16* numAttr + 1));
	strcpy((char*)recHead-> reserved,"hip");
	
	
	recRec->setHeader(recHead);
	
	struct HeadInfo * recRetHead = new struct HeadInfo ();
	recRec->getHeader(recRetHead);

    union Attribute *recAttrs = new union Attribute[numAttr];
     
     if(read==0)
     {
        va_list args;
        va_start(args,read);
	    for(int i =0;format[i]!='\0';++i){
		    if(format[i]=='f')
			    recAttrs[i].fval = va_arg(args, double);
		    else if(format[i]=='i')
			    recAttrs[i].ival = va_arg(args, int);
		    else
			    strcpy(recAttrs[i].strval, va_arg(args, char*));
	    }
	
	    va_end(args);
    }
    else
    {
        cout<<"Enter the first record :\n";
        for(int i=0;i<numAttr; i++)
        {
            cout<<"Enter attribute "<<i+1<<" : ";
            if(format[i]=='s')
                cin>>recAttrs[i].strval;
            else if(format[i]=='i')
                cin>>recAttrs[i].ival;
            else
                cin>>recAttrs[i].fval;
        }
    }
    
    unsigned char slotMap[recHead-> numSlots];
    for(int i=1; i<recRetHead-> numSlots; i++)
        slotMap[i]='0';
    slotMap[0]='1';
    recRec->setSlotMap(slotMap);   
    recRec->setRecord(recAttrs,0);
	return recRec->getBlockNum();
}

//int getRecordSim(int blockNum)
int getRecordSim(char *relationName)
{
    
    int blockNum = getFirstBlockOfRec(relationName);
    
    do{
   //StaticBuffer staticBuffer;
        class RecBuffer *recBuffer = new RecBuffer(blockNum);
        unsigned char *slotMap = new unsigned char; 
        struct HeadInfo * headInfo = new struct HeadInfo ();
        
        recBuffer->getSlotMap(slotMap);
	    recBuffer->getHeader(headInfo);
        cout <<"Head attributes\n";
	    cout<<headInfo-> blockType<<endl;
	    cout<<headInfo-> pblock<<endl;
	    cout<<headInfo-> lblock<<endl;//need to be changed
	    cout<<headInfo-> rblock<<endl;//need to be changed
	    cout<<headInfo-> numEntries<<endl;
	    cout<<headInfo-> numAttrs<<endl; 
	    cout<<headInfo-> numSlots<<endl;
	    //cout<<headInfo-> reserved<<endl;
        
        //cout<<endl<<"Inside get record sim"<<endl;
        
	    char format[125];
	    getFormatString(relationName, format);
        cout <<"Table Contents\n";	
        for(int i=0; i<headInfo-> numSlots; i++)
            if((int)slotMap[i]=='1')
            {
                cout<<"Record "<<i<<":"<<endl;
                union Attribute *rec = new union Attribute[headInfo->numAttrs];
                recBuffer->getRecord(rec,i);
                for(int j=0; j<headInfo->numAttrs ; j++)
                    {if(format[j]=='s')
                        cout<<rec[j].strval<<endl;
                     else if(format[j]=='i')
                        cout<<rec[j].ival<<endl;
                     else if(format[j]=='f')
                        cout<<rec[j].fval<<endl;
                    
                    
                   // cout<<rec[j].strval<<endl;cout<<rec[j].ival<<endl;
                   }
                
            }
		
		    blockNum = headInfo->rblock;
           
    }while(blockNum!=-1);
    return 0;
}

int getFirstBlockOfRec(char * relName) {
	union Attribute *relCatAttrs = new union Attribute[6];
	struct HeadInfo * relCatHead = new struct HeadInfo ();
	
	int blockNum = 4;
	
	do{
	
		class RecBuffer * relCat= new RecBuffer(blockNum);
		unsigned char relCatSlotMap[20];
		relCat ->getSlotMap(relCatSlotMap);
	    

		for(int i=0;i<20;++i){
			if(relCatSlotMap[i]=='1'){
				relCat->getRecord(relCatAttrs,i);
				if(strcmp(relCatAttrs[0].strval,relName)==0){
					return relCatAttrs[3].ival;
				}
			}
		}
		
		//relCatHead = new struct HeadInfo ();
		relCat->getHeader(relCatHead);
		
		blockNum = relCatHead->rblock;
		
	}while(blockNum!=-1);
	return -1;
}
int getLastBlockOfRec(char * relName) {
	//cout <<"Entering getLastBlockOfRec"<<endl;
	
	union Attribute *relCatAttrs = new union Attribute[6];
	struct HeadInfo * relCatHead = new struct HeadInfo ();
	
	int blockNum = 4;
	
	do{
	
		class RecBuffer * relCat= new RecBuffer(blockNum);
		unsigned char relCatSlotMap[20];
		relCat ->getSlotMap(relCatSlotMap);
	

		for(int i=0;i<20;++i){
			if(relCatSlotMap[i]=='1'){
				relCat->getRecord(relCatAttrs,i);
				if(strcmp(relCatAttrs[0].strval,relName)==0){
					//cout <<"Exiting getLastBlockOfRec"<<endl;
					return relCatAttrs[4].ival;
				}
			}
		}
		
		//relCatHead = new struct HeadInfo ();
		relCat->getHeader(relCatHead);
		
		blockNum = relCatHead->rblock;
		
	}while(blockNum!=-1);
	return -1;
}

int getNumAttrsOfRec(char * relName) {
	
	union Attribute *relCatAttrs = new union Attribute[6];
	struct HeadInfo * relCatHead = new struct HeadInfo ();
	
	int blockNum = 4;
	
	do{
	
		class RecBuffer *relCat= new RecBuffer(blockNum);
		unsigned char relCatSlotMap[20];
		relCat ->getSlotMap(relCatSlotMap);
	

		for(int i=0;i<20;++i){
			if(relCatSlotMap[i]=='1'){
				relCat->getRecord(relCatAttrs,i);
				if(strcmp(relCatAttrs[0].strval,relName)==0){
					return relCatAttrs[1].ival;
				}
			}
		}
		
		//relCatHead = new struct HeadInfo ();
		relCat->getHeader(relCatHead);
		
		blockNum = relCatHead->rblock;
		
	}while(blockNum!=-1);
	return -1;
}

int addRecordSim(char * relName,int read, ...)
{   
    int blockNum = getLastBlockOfRec(relName);
    int numAttr = getNumAttrsOfRec(relName);
    char format[125];
	getFormatString((char *)relName, format);
	//cout<<"\n\n\n"<<format<<endl<<endl;
	
    class RecBuffer *recBuffer = new RecBuffer(blockNum);
    struct HeadInfo * headInfo = new struct HeadInfo ();
    unsigned char *slotMap = new unsigned char; 
	
	recBuffer->getHeader(headInfo);
	recBuffer->getSlotMap(slotMap);
	
	int i;
	//finding a free slot
	for(i=0; i<headInfo-> numSlots; i++)
        if((int)slotMap[i]=='0')
            break;
            
    union Attribute *recAttrs = new union Attribute[5];
        va_list args;
	va_start(args,read);
     if(read==0)
     {
        va_list args;
        va_start(args,read);
	    for(int i =0;format[i]!='\0';++i){
		    if(format[i]=='f')
			    recAttrs[i].fval = va_arg(args, double);
		    else if(format[i]=='i')
			    recAttrs[i].ival = va_arg(args, int);
		    else
			    strcpy(recAttrs[i].strval, va_arg(args, char*));
	    }
	
	    va_end(args);
    }
    else
    {
        cout<<"Enter the record :\n";
        for(int j=0;j<numAttr; j++)
        {
            cout<<"Enter attribute "<<j+1<<" : ";
            if(format[j]=='s')
                cin>>recAttrs[j].strval;
            else if(format[j]=='i')
                cin>>recAttrs[j].ival;
            else
                cin>>recAttrs[j].fval;
        }
    }
	

	recBuffer->setRecord(recAttrs, i);
	
    struct HeadInfo * newHead = new struct HeadInfo ();
    newHead-> blockType = headInfo->blockType;
	newHead-> pblock = headInfo-> pblock;
	newHead-> lblock = headInfo-> lblock;
	newHead-> rblock = headInfo-> rblock;
	newHead-> numEntries = headInfo-> numEntries+1;
	newHead-> numAttrs = headInfo-> numAttrs; 
	newHead-> numSlots = headInfo-> numSlots;

	recBuffer->setHeader(newHead);
    slotMap[i] = '1';
    recBuffer->setSlotMap(slotMap);
    return 0;
}

int replacementSim()
{
    for(int i=0; i<35; i++)
    {
     cout<<"\n\n\n\n------Block "<<i+4<<" ---------------\n";
        setRecordSim(5,(char *)"ssiss",0, "Appu", "B160116CS", 21, "F", "rock on");
       
	    if(i==30)
	     addRecordSim((char *)"Student",0,"Suku", "B160483CS", 21, "M", "being suku");
		
    }  
}

int setInternalSim(int lchild, Attribute attrVal, char f, int rchild)
{
    class IndInternal *indexBlock = new IndInternal();
    struct HeadInfo *headInfo = new struct HeadInfo ();
    headInfo-> blockType = IND_INTERNAL;
	headInfo-> pblock=-1;
	headInfo-> lblock=-1;
	headInfo-> rblock=-1;
	headInfo-> numEntries=1;
	headInfo-> numAttrs=1;  //TODO: what is this?
	headInfo-> numSlots=100;
	strcpy((char*)headInfo-> reserved,"");
	
	
	indexBlock->setHeader(headInfo);
	
	struct HeadInfo * retHeadInfo = new struct HeadInfo ();
	indexBlock->getHeader(retHeadInfo);
	
	cout <<"Head attributes\n";
	cout<<(retHeadInfo-> blockType)<<endl;
	cout<<retHeadInfo-> pblock<<endl;
	cout<<retHeadInfo-> lblock<<endl;
	cout<<retHeadInfo-> rblock<<endl;
	cout<<retHeadInfo-> numEntries<<endl;
	cout<<retHeadInfo-> numAttrs<<endl; 
	cout<<retHeadInfo-> numSlots<<endl;
	cout<<retHeadInfo-> reserved<<endl;
	
	union Attribute attribute;
	
	struct InternalEntry *internalEntry = new struct InternalEntry();
	internalEntry->lChild = lchild;
	if(f=='s')
	    strcpy(internalEntry->attrVal.strval, attrVal.strval);
	else if(f=='i')
	    internalEntry->attrVal.ival = attrVal.ival;
	else
	    internalEntry->attrVal.fval = attrVal.fval;
	
	internalEntry->rChild = rchild;
	
	indexBlock->setEntry(internalEntry,0);
	
	struct InternalEntry *retInternalEntry = new struct InternalEntry();
	
	indexBlock->getEntry(retInternalEntry,0);
	
	cout<<retInternalEntry->lChild<<endl;
    if(f=='s')
	    cout<<retInternalEntry->attrVal.strval<<endl;
	else if(f=='i')
	    cout<<retInternalEntry->attrVal.ival<<endl;
	else
	cout<<retInternalEntry->attrVal.fval<<endl;
	
	cout<<retInternalEntry->rChild<<endl;

    return 0;
}
int addInternalEntry(int blockNum, int lchild, Attribute attrVal, char f, int rchild)
{
    class IndInternal *indexBlock = new IndInternal(blockNum);
    
    struct HeadInfo *headInfo = new struct HeadInfo ();
    indexBlock->getHeader(headInfo);
    
	headInfo-> numEntries+=1;
	indexBlock->setHeader(headInfo);
	
	struct InternalEntry *internalEntry = new struct InternalEntry();
	internalEntry->lChild = lchild;
	if(f=='s')
	    strcpy(internalEntry->attrVal.strval, attrVal.strval);
	else if(f=='i')
	    internalEntry->attrVal.ival = attrVal.ival;
	else
	    internalEntry->attrVal.fval = attrVal.fval;
	
	internalEntry->rChild = rchild;
	
	indexBlock->setEntry(internalEntry,headInfo-> numEntries-1);
return 1;	
}

int getInternalEntry(int blockNum, char f)
{
    class IndInternal *indexBlock = new IndInternal(blockNum);
     
    struct InternalEntry *internalEntry = new struct InternalEntry();
	
	struct HeadInfo * retHeadInfo = new struct HeadInfo ();
	
	indexBlock->getHeader(retHeadInfo);
	
	for(int i=0; i<retHeadInfo->numEntries; i++)
	{   indexBlock->getEntry(internalEntry,i);
	
	    cout<<internalEntry->lChild<<endl;
        if(f=='s')
	        cout<<internalEntry->attrVal.strval<<endl;
	    else if(f=='i')
	        cout<<internalEntry->attrVal.ival<<endl;
	    else
	    cout<<internalEntry->attrVal.fval<<endl;
	
	    cout<<internalEntry->rChild<<endl;
    }
}


int setLeafSim(int block, int slot, Attribute attrVal, char f)
{
    class IndLeaf *leafBlock = new IndLeaf();
    struct HeadInfo *headInfo = new struct HeadInfo ();
    headInfo-> blockType = IND_LEAF;
	headInfo-> pblock=-1;
	headInfo-> lblock=-1;
	headInfo-> rblock=-1;
	headInfo-> numEntries=1;
	headInfo-> numAttrs=1;  //TODO: what is this?
	headInfo-> numSlots=100;
	strcpy((char*)headInfo-> reserved,"");
	
	
	leafBlock->setHeader(headInfo);
	
	struct HeadInfo * retHeadInfo = new struct HeadInfo ();
	leafBlock->getHeader(retHeadInfo);
	
	cout <<"Head attributes\n";
	cout<<(retHeadInfo-> blockType)<<endl;
	cout<<retHeadInfo-> pblock<<endl;
	cout<<retHeadInfo-> lblock<<endl;
	cout<<retHeadInfo-> rblock<<endl;
	cout<<retHeadInfo-> numEntries<<endl;
	cout<<retHeadInfo-> numAttrs<<endl; 
	cout<<retHeadInfo-> numSlots<<endl;
	cout<<retHeadInfo-> reserved<<endl;
	union Attribute attribute;
	struct Index *index = new struct Index();
	index->block = block;
	index->slot = slot;
	if(f=='s')
	    strcpy(index->attrVal.strval, attrVal.strval);
	else if(f=='i')
	    index->attrVal.ival = attrVal.ival;
	else
	    index->attrVal.fval = attrVal.fval;
	memcpy(index->unused, "unused",8);
	
	leafBlock->setEntry(index,0);
	
	struct Index *retIndex = new struct Index();
	leafBlock->getEntry(retIndex,0);	
	cout<<retIndex->block<<endl;
	cout<<retIndex->slot<<endl;
    if(f=='s')
	    cout<<retIndex->attrVal.strval<<endl;
	else if(f=='i')
	    cout<<retIndex->attrVal.ival<<endl;
	else
	cout<<retIndex->attrVal.fval<<endl;
	cout<<retIndex->unused<<endl;
    return 0;
}

int addLeafEntry(int blockNum, int block, int slot, Attribute attrVal, char f)
{
    class IndLeaf *leafBlock = new IndLeaf(blockNum);
    
    struct HeadInfo *headInfo = new struct HeadInfo ();
    leafBlock->getHeader(headInfo);
    
	headInfo-> numEntries+=1;
	leafBlock->setHeader(headInfo);
	
	struct Index *leafEntry = new struct Index();
	leafEntry->block = block;
	leafEntry->slot = slot;
	if(f=='s')
	    strcpy(leafEntry->attrVal.strval, attrVal.strval);
	else if(f=='i')
	    leafEntry->attrVal.ival = attrVal.ival;
	else
	    leafEntry->attrVal.fval = attrVal.fval;
	memcpy(leafEntry->unused,"unused",8);	
	leafBlock->setEntry(leafEntry, headInfo-> numEntries-1);
    return 1;	
}

int getLeafEntry(int blockNum, char f)
{
    class IndLeaf *indexBlock = new IndLeaf(blockNum);
     
    struct Index *internalEntry = new struct Index();
	
	struct HeadInfo * retHeadInfo = new struct HeadInfo ();
	
	indexBlock->getHeader(retHeadInfo);
	
	for(int i=0; i<retHeadInfo->numEntries; i++)
	{   indexBlock->getEntry(internalEntry,i);
	
	    cout<<internalEntry->block<<endl;
	    cout<<internalEntry->slot<<endl;
        if(f=='s')
	        cout<<internalEntry->attrVal.strval<<endl;
	    else if(f=='i')
	        cout<<internalEntry->attrVal.ival<<endl;
	    else
	        cout<<internalEntry->attrVal.fval<<endl;
	    cout<<internalEntry->unused<<endl;
	
    }
    return 0;
}

int setRelationCatRecord()
{
    setRecordSim(6,(char *)"siiiii",0, "relationCat", 6, 2,4, 4, 20);
    addRecordSim((char *)"relationCat",0, "attributeCat", 6, 12, 5, 5, 20 );
    return 1;
}
int updateRelationCat(char * relName)
{
	
	int blockNum = 4;
	
	do{
	
		class RecBuffer *currRec= new RecBuffer(blockNum);
		unsigned char recSlotMap[20];
		currRec ->getSlotMap(recSlotMap);
	
		union Attribute *recAttrs = new union Attribute[6];
		for(int i=0;i<20;++i){
			if(recSlotMap[i]=='1'){
				currRec->getRecord(recAttrs,i);
				if(strcmp(recAttrs[0].strval,relName)==0){
					recAttrs[2].ival ++;
					currRec->setRecord(recAttrs, i);
					return 1;
				}
			}
		}
		
		struct HeadInfo * recHead = new struct HeadInfo ();
		currRec->getHeader(recHead);
		blockNum = recHead->rblock;
		
	}while(blockNum!=-1);
	
	return 0;
}

void addEntryRelationCat(char * relName, int numAttrs, int numRec, int firstBlock, int lastBlock){
	
	int slotsPerBlock = floor(2016/(16* numAttrs + 1));

	class RecBuffer *relCat= new RecBuffer(4);
	union Attribute *relCatAttrs = new union Attribute[6];
	struct HeadInfo * relCatHead = new struct HeadInfo ();
	
	relCat -> getRecord(relCatAttrs,0);//0th record containts meta data of relation catalog
	relCat -> getHeader(relCatHead);
	
	int blockNum = relCatAttrs[4].ival;//insertion to be done in the last block
	//cout<<"\n\nPrinting Relational Catalog\n"<<blockNum;
	//getRecordSim(4);
	
	class RecBuffer *currRec= new RecBuffer(blockNum);
		
	unsigned char recSlotMap[20];
	currRec ->getSlotMap(recSlotMap);
		
	union Attribute * recAttrs = new union Attribute[6];
	strcpy(recAttrs[0].strval, relName);
	recAttrs[1].ival = numAttrs;
	recAttrs[2].ival = numRec;
	recAttrs[3].ival = firstBlock;
	recAttrs[4].ival = lastBlock;
	recAttrs[5].ival = slotsPerBlock;
	updateRelationCat((char*)"relationCat");
	struct HeadInfo * recHead = new struct HeadInfo ();
	currRec-> getHeader(recHead);
	for(int i=0;i<20;++i){
		if(recSlotMap[i]=='0'){
			//cout<<"Found free slot\n";
			currRec->setRecord(recAttrs, i);
			
			recHead-> numEntries++;
			currRec->setHeader(recHead);
			
			recSlotMap[i]='1';
			currRec->setSlotMap(recSlotMap);
			
			return;	
		}
	}
	//the last block was full-allot a new rec block for relation catalog
	int lastBlockNum = setRecordSim(6, (char *)"siiiii", 0, relName, numAttrs, numRec, firstBlock, lastBlock, slotsPerBlock);
	
	//TODO:set left block
	
	//set up the left link of current last block
	recHead-> rblock = lastBlockNum;
	currRec->setHeader(recHead);
	
	relCatAttrs[4].ival= lastBlockNum;
	relCat->setRecord(relCatAttrs, 0);
	
	
}
int setAttributeCatRecord()
{
    setRecordSim(6, (char *)"sssiii",0, "relationCat", "relation name", "string", -1, -1, 0 );   
    addRecordSim((char *)"attributeCat", 0, "relationCat", "#attributes", "int", -1, -1, 1 );   
    addRecordSim((char *)"attributeCat", 0, "relationCat", "#records", "int", -1, -1, 2 );   
    addRecordSim((char *)"attributeCat", 0, "relationCat", "first block", "int", -1, -1, 3 );   
    addRecordSim((char *)"attributeCat", 0, "relationCat", "last block", "int", -1, -1, 4 );   
    addRecordSim((char *)"attributeCat", 0, "relationCat", "#slots per block", "int", -1, -1, 5 );   
    
    addRecordSim((char *)"attributeCat", 0, "attributeCat", "relation name", "string", -1, -1, 0 );
    addRecordSim((char *)"attributeCat", 0, "attributeCat", "attribute name", "string", -1, -1, 1 );
    addRecordSim((char *)"attributeCat", 0, "attributeCat", "attribute type", "string", -1, -1, 2 );
    addRecordSim((char *)"attributeCat", 0, "attributeCat", "primary flag", "int", -1, -1, 3 );
    addRecordSim((char *)"attributeCat", 0, "attributeCat", "root block", "int", -1, -1, 4 );
    addRecordSim((char *)"attributeCat", 0, "attributeCat", "offset", "int", -1, -1, 5 );
    return 1;
    
}

int addEntryAttributeCat(char *relationName, char *attributeName, char *attributeType, int primaryFlag, int offset){
    
	
	
	class RecBuffer *relCat= new RecBuffer(4);
	union Attribute *relCatAttrs = new union Attribute[6];
	struct HeadInfo * relCatHead = new struct HeadInfo ();
	
	relCat -> getRecord(relCatAttrs,1);//2nd record containts meta data of attribute catalog
	relCat -> getHeader(relCatHead);
	
	int blockNum = relCatAttrs[4].ival;
	
	class RecBuffer *attrCat= new RecBuffer(blockNum);
	union Attribute *attrCatAttrs = new union Attribute[6];
	struct HeadInfo * attrCatHead = new struct HeadInfo ();
	
	attrCat->getHeader(attrCatHead);
	strcpy(attrCatAttrs[0].strval, relationName);
	strcpy(attrCatAttrs[1].strval, attributeName);
	strcpy(attrCatAttrs[2].strval, attributeType);
	attrCatAttrs[3].ival = primaryFlag;
	attrCatAttrs[4].ival = -1;
	attrCatAttrs[5].ival = offset;
	updateRelationCat((char*)"attributeCat");
	int i;
	unsigned char slotMap[20];
	attrCat -> getSlotMap(slotMap);
	for(i=0; i<20; i++)
	    if(slotMap[i]=='0')
	        break;
	if(i==20)
	{   
	    int lastBlockNum = setRecordSim(6, (char *)"sssiii",0, relationName, attributeName, attributeType, primaryFlag, -1, offset);
	
	    //TODO:set left block
	
	    //set up the left link of current last block
	    attrCatHead-> rblock = lastBlockNum;
	    attrCat->setHeader(attrCatHead);
	
	    relCatAttrs[4].ival= lastBlockNum;
	    relCat->setRecord(relCatAttrs, 1);
	    
	}
	else   {     
	attrCat->setRecord(attrCatAttrs, i);
	slotMap[i]='1';
	attrCat->setSlotMap(slotMap);
	
	attrCatHead->numEntries++;
	attrCat->setHeader(attrCatHead);
	}
    return 1;
}

int main() 
{
    static Disk disk;
    //TODO: static
    StaticBuffer staticBuffer;
    setRelationCatRecord();
    setAttributeCatRecord();
	cout<<"\n\n\n**************INITIALISATIONS OVER**************\n\n\n";    
	/*

	getRecordSim((char*)"relationCat");
	
	int blockNum = setRecordSim(5,(char *)"ssiss",0,"Appu", "B160116CS", 21, "F", "miles to go before i sleep");
	addEntryRelationCat((char *)"Student", 5, 1, blockNum, blockNum);
	addEntryAttributeCat((char *)"Student", (char *)"Name", (char *)"String", -1, 0);
	addEntryAttributeCat((char *)"Student", (char *)"Roll No", (char *)"String", -1, 1);
	addEntryAttributeCat((char *)"Student", (char *)"Age", (char *)"Integer", -1, 2);
	addEntryAttributeCat((char *)"Student", (char *)"Gender", (char *)"String", -1, 3);
	addEntryAttributeCat((char *)"Student", (char *)"Status", (char *)"String", -1, 4);
	
	addRecordSim((char*)"Student", 0, "Vrindha", "B160228CS", 21, "F", "this too shall pass");	
	addRecordSim((char*)"Student", 0, "Suku", "B160483CS", 21, "M", "being suku");


	
	
	cout<<"\n\n\n**************RELATION CATALOG RECORDS**************\n\n\n";
	getRecordSim((char *)"relationCat");
	cout<<"\n\n\n**************ATTRIBUTE CATALOG RECORDS**************\n\n\n";
	getRecordSim((char *)"attributeCat");
	cout<<"\n\n\n**************STUDENT RECORDS**************\n\n\n";
	getRecordSim((char *)"Student");
	*/
	
	
	char relationName[100];
	int choice, numAttrs;
	do
	{   cout<<"1. Create a new relation.\n2. Add new record.\n3. Display relation.\n4. Display Relation Catalog. \n5. Display    Attribute Catalog\n6. Shut Down\n";
	    cin>>choice;
            
            switch(choice) {
                case 1:
                     cout<<"Enter relation name: "; 
	                 cin>>relationName;
	                 cout<<"Enter number of attributes: "; 
	                 cin>>numAttrs;
	                 char format[125];
	                 int i, blockNum;
	                 for(i=0; i<numAttrs; i++)
	                 {
	                     char attrName[100], attrType[10];
	                     cout<<"Enter attribute "<<i<<" name : "; 
	                     cin>>attrName;
	                     cout<<"Enter type of attribute "<<i<<"(String/Integer/Float) : "; 
	                     cin>>attrType;
	                     addEntryAttributeCat((char *)relationName, (char *)attrName, (char *)attrType, -1, i);
	                     if(strcmp(attrType,"String")==0 )
	                        format[i]='s';
	                     else if(strcmp(attrType,"Integer")==0 )
	                        format[i]='i';
	                     else 
	                        format[i]='f';
	                 }
	                 format[i]='\0';
	                 cout<<"Enter 1st record";
	                 blockNum = setRecordSim(numAttrs,(char *)format, 1);
	                 addEntryRelationCat((char *)relationName, numAttrs, 1, blockNum, blockNum);
	                 
                    break;
                case 2:
                    cout<<"Enter relation name: "; 
	                 cin>>relationName;
	                 addRecordSim((char *)relationName,1);
                    
                    break;
                case 3:
                    cout<<"Enter relation name: "; 
	                cin>>relationName;
	                getRecordSim((char *)relationName);
	                break;
                case 4:
                    getRecordSim((char *)"relationCat");
	                break;
                case 5:
                    getRecordSim((char *)"attributeCat");
	                break;

            }	                 
	
	
	}while(choice!=6);
 
	return 0;
}

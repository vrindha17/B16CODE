#include <bits/stdc++.h>


//CHANGE-26//
#define myBLOCK_ALLOC_MAP 4
#define myUNUSED 5

using namespace std;

//TODO : This has to be removed.
static unsigned char block[8192][2048];	

class Disk{
    public:
     Disk(); //instead of initialise
    ~Disk(); //instead of finalise
    static int readBlock(void * buffer, int blockNum);
    static int writeBlock(int blockNum, void * buffer);
};


int Disk::readBlock(void * buffer, int blockNum){
    memcpy(buffer, block[blockNum], 2048); 
    cout <<"Inside readBlock\n";  
	return 1;
}

int Disk::writeBlock(int blockNum, void * buffer)
{   memcpy(block[blockNum], buffer, 2048);      
	return 1;
	
}

Disk::Disk(){
//TODO: Remove this
    cout<<"Inside Constructor"<<endl;
    unsigned char blockAllocMap[8192];
    for (int i =0;i<8192;++i)
    	if(i<4)//the 1st 4 are block_alloc_map blocks
    		blockAllocMap[i] = myBLOCK_ALLOC_MAP;
    	else
    		blockAllocMap[i] = myUNUSED;
    for(int i=0; i<4; i++)
        writeBlock(i, blockAllocMap + i*2048);
}

Disk::~Disk(){
    cout<<"Write back to file";
}

/*
void initializeDiskAllocMap()
{
	//CHANGE-26//
	
    //unsigned char blockAllocMap[8192]={0};
    unsigned char blockAllocMap[8192];
    for (int i =0;i<8192;++i)
    	if(i<4)//the 1st 4 are block_alloc_map blocks
    		blockAllocMap[i] = myBLOCK_ALLOC_MAP;
    	else
    		blockAllocMap[i] = myUNUSED;
    for(int i=0; i<4; i++)
        writeBlock(i, blockAllocMap + i*2048);
    
}
*/

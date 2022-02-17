#include <stdio.h>
#include <stdlib.h>
//#include "sim_cache.h"
#include <math.h>
#include <iostream>

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim_cache 32 8192 4 7 262144 8 gcc_trace.txt
    argc = 8
    argv[0] = "sim_cache"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
typedef struct cache_params{
    unsigned long int block_size;
    unsigned long int l1_size;
    unsigned long int l1_assoc;
    unsigned long int vc_num_blocks;
    unsigned long int l2_size;
    unsigned long int l2_assoc;
}cache_params;

class Config{
    public:

    unsigned long int BLOCK_SIZE;
    unsigned long int SIZE;
    unsigned long int ASSOC;
    unsigned long int vc_num_blocks;
    unsigned long int set;
    unsigned long int assoc;
    unsigned long int NO_block_offset_bits;
    unsigned long int addr;
    unsigned long int NO_index_bits;
    unsigned long int NO_tag_bits;
    unsigned long int tag_addr;
    unsigned long int tag_index_addr;
    unsigned long int index_addr;
    bool **cache_dirty;
    bool **cache_valid;
    unsigned long int **cache_tag;
    unsigned long int **tag_bit;
    int **lru;
	unsigned long int NL_addr; 
	unsigned long int **cache_final;
	bool **dirty_final; 
	unsigned long int PL_addr;	
	int lru_dirty;
    unsigned long int lru_tag;
    int lru_pos;
    bool writebacked;
    int x;
  	unsigned long int number_of_reads = 0;
    unsigned long int number_of_read_hits = 0;
    unsigned long int number_of_read_misses = 0;
    unsigned long int number_of_writes = 0;
    unsigned long int number_of_write_hits = 0;
    unsigned long int number_of_write_misses = 0;
    unsigned long int number_of_writebacks = 0;
    //double miss_rate;
    
    Config *next,*prev;
    

    Config(int block_size, int size, int assoc)
    {
    //no. of bits calculation

        BLOCK_SIZE = block_size;
        SIZE = size;
        ASSOC = assoc;
        set = (SIZE/(ASSOC*BLOCK_SIZE));
        NO_index_bits = log2(set);
        NO_block_offset_bits =log2(BLOCK_SIZE);
        NO_tag_bits = (32 - NO_index_bits - NO_block_offset_bits);
        writebacked = false;

    //cache bits initialisation
        cache_tag = new unsigned long int*[set];
        cache_dirty = new bool*[set];
        cache_valid = new bool*[set];
        lru = new int*[set];
        
        for (int i = 0; i < set; i++)
        {
            cache_tag[i] = new unsigned long int[assoc];
            cache_dirty[i]= new bool[assoc];
            cache_valid[i] = new bool[assoc];
            lru[i] = new int[assoc];
            
            for (int j = 0; j < assoc; j++)
            {
                cache_tag[i][j]= 0;
                cache_dirty[i][j]= 0;
                cache_valid[i][j] = 0;
                lru[i][j]=j;
            }
        }
	}

	void read(unsigned long int addr)
   {
        
       tag_addr = (addr &((0xFFFFFFFF) << ((NO_index_bits + NO_block_offset_bits))))>> (NO_index_bits + NO_block_offset_bits);
       index_addr = (addr &(((0xFFFFFFFF)<<(NO_tag_bits))>>(NO_tag_bits)))>>(NO_block_offset_bits);
       bool read_hit = false;
       int pos = -1;
   //    int lru_dirty;
    //   int lru_tag;
      // int lru_pos;
		//printf("\n HERE\n");       
      	for(int i=0;i<ASSOC;i++)
	    {
   		if(lru[index_addr][i]==ASSOC-1)
		   {
   			lru_tag=cache_tag[index_addr][i];
   			lru_pos=i;
   			lru_dirty=cache_dirty[index_addr][i];
   		//	printf("%d",lru_dirty);
   			break;

	   		}
		}      
       //tag+index address calculation for next level cache
       NL_addr=(lru_tag<<(NO_index_bits+NO_block_offset_bits))|(index_addr<<NO_block_offset_bits); 
	   
	   number_of_reads++; 
	      
    	for(int i=0;i<ASSOC;i++)
		{
    		if((cache_tag[index_addr][i]==tag_addr)&&(cache_valid[index_addr][i]))
			{
    			//readhit(tag_addr,index_addr,i);
    			read_hit = true;
    			number_of_read_hits++;
    			pos = i;
    			break;
			}
		}
		//printf("\n HERE1\n");
		if(read_hit)
		{
			//read hit
			//printf("\n Hit\n");
			//printf("%d %d %d %d",tag_addr,index_addr,pos,lru_pos);
			readhit(tag_addr,index_addr,pos,lru_pos);
			return;
			//printf("\n HERE2\n");
		}
		else
		{
			number_of_read_misses++;
			if (this->next!=NULL)   //if L2 is present
			{
				if(lru_dirty==1)   //if L1 LRU is dirty
				{
					this->next->writeback(NL_addr);   //writeback to L2
					if(writebacked==0)
					{
						cache_dirty[index_addr][lru_pos]=0;   //set L1 dirty bit of LRU to zero
						
						this->next->read(addr);	
						//Update LRU
						number_of_writebacks++;

						for(int i=0;i<ASSOC;i++)
				    	{
							if((lru[index_addr][i])<(lru[index_addr][lru_pos]))
							{
							lru[index_addr][i]++;
							}
					
						}
						lru[index_addr][lru_pos]=0;
					}

				}
				
				else{
				//	number_of_writebacks++;
					this->next->read(addr);
					
					//Update LRU
					for(int i=0;i<ASSOC;i++)
				    {
						if((lru[index_addr][i])<(lru[index_addr][lru_pos]))
						{
						lru[index_addr][i]++;
						}
					
					}
					lru[index_addr][lru_pos]=0;
				
					
				}
			}
			else
			{
			//read miss
			//printf("L2 read req\n");
		//	number_of_writebacks++;

			if(lru_dirty == 1)
			{
				number_of_writebacks++;
				cache_dirty[index_addr][lru_pos] = 0;
			}
			readmiss(tag_addr,index_addr,lru_pos);
			if(this->prev!=NULL)
			{
				this->prev->install(addr);  ///NL_address
			}
			
			return;
			}
		}
		//printf("\n HERE3\n");
   } 
   	

   
   //read hit function
   void readhit(unsigned long int tag_addr,unsigned long int index_addr,int pos,int lru_pos)
   {

   	for(int i=0;i<ASSOC;i++)
	   {
		if((lru[index_addr][i])<(lru[index_addr][pos]))
		{
			lru[index_addr][i]++;
		}
		}
		lru[index_addr][pos]=0;
		
		if(this->prev!=NULL)    //if L1 is there
		{   
			//this->prev->cache_tag[index_addr][lru_pos]=cache_tag[index_addr][pos];
			unsigned long int full_address =(tag_addr<<(NO_index_bits+NO_block_offset_bits))|(index_addr<<NO_block_offset_bits);  
			this->prev->install(full_address);
			//update L1 LRu done in install
		}
	}
		 
	
   //read miss function
   void readmiss(unsigned long int tag_addr, unsigned long int index_addr,int lru_pos)
   {

//	   
//	   cache_dirty[index_addr][pos]=read_dirty;
//       PL_addr=(tag_addr<<(NO_index_bits+NO_block_offset_bits))|(index_addr<<NO_block_offset_bits);  
//       
//	   if(this->prev!=NULL)
//	   {
//	   		this->prev->install(PL_addr);
//		//	this->prev->cache_tag[index_addr][lru_pos]=cache_tag[index_addr][pos];
//	   }

		int pos = -1;
		for(int i=0;i<ASSOC;i++)
	   {
			if((lru[index_addr][i])<(lru[index_addr][lru_pos]))
			{
				lru[index_addr][i]++;
			}
			
		}
		lru[index_addr][lru_pos]=0;
		cache_tag[index_addr][lru_pos]=tag_addr;
 		cache_valid[index_addr][lru_pos]=1;
 		return;
   }
   
    int LRU_find(unsigned long int index_addr)
   	{
   		int pos;
   		for(int i=0;i<ASSOC;i++)
	   {
   			if(lru[index_addr][i]==ASSOC-1)
		   {
   			
   				pos=i;
   				return pos;
   				//break;
   			

	   		}
		} 
	}
   
 	void install(unsigned long int PL_addr)
 	{
 		tag_addr = (PL_addr &((0xFFFFFFFF) << ((NO_index_bits + NO_block_offset_bits))))>> (NO_index_bits + NO_block_offset_bits);
        index_addr = (PL_addr &(((0xFFFFFFFF)<<(NO_tag_bits))>>(NO_tag_bits)))>>(NO_block_offset_bits);
        int a=LRU_find(index_addr);
        //a=LRU_find(index_addr);
        
        cache_tag[index_addr][a]=tag_addr;
        //ADDED Oct 26
        cache_valid[index_addr][a] = 1;
        //ENDS
        
      /*  for(int i=0;i<ASSOC;i++)
	   	{
			if((lru[index_addr][i])<(lru[index_addr][a]))
			{
				lru[index_addr][i]++;
			}
			lru[index_addr][a]=0;
		}*/
        
	}

   	void write(unsigned long int addr)
	{
		tag_addr = (addr &((0xFFFFFFFF) << ((NO_index_bits + NO_block_offset_bits))))>> (NO_index_bits + NO_block_offset_bits);
        index_addr = (addr &(((0xFFFFFFFF)<<(NO_tag_bits))>>(NO_tag_bits)))>>(NO_block_offset_bits);
        bool write_hit = false;
        
    //	int lru_dirty_w;
     //   int lru_tag_w;
       // int lru_pos_w;
       
      	for(int i=0;i<ASSOC;i++)
	   {
   			if(lru[index_addr][i]==ASSOC-1)
		   	{
   				lru_tag=cache_tag[index_addr][i];   //made correction lru_tag_w
   				lru_pos=i;				//"
   				lru_dirty=cache_dirty[index_addr][i]; //"
   				break;
	   		}
		}
		      
       //tag+index address calculation for next level cache
       NL_addr=(lru_tag<<(NO_index_bits+NO_block_offset_bits))|(index_addr<<NO_block_offset_bits);  
       
       number_of_writes++;
        
        
        
        int pos = -1;
    	for(int i=0;i<ASSOC;i++)
		{
    		if((cache_tag[index_addr][i]==tag_addr)&&(cache_valid[index_addr][i])){
    			//writehit(tag_addr,index_addr,i);
    			write_hit = true;
    			number_of_write_hits++;
    			pos = i;
    			break;
			}
			
		}
		
		if(write_hit)
		{
			//write hit
			writehit(tag_addr,index_addr,pos);
			return;
		}
		else
		{
			number_of_write_misses++;
			if(this->next!=NULL)
			{
				if(lru_dirty==1)
				{
					this->next->writeback(NL_addr);
					cache_dirty[index_addr][lru_pos]=0;
					
					this->next->read(addr);	
					number_of_writebacks++;																					//added newly 26 10
					for(int i=0;i<ASSOC;i++)
	   				{
						if((lru[index_addr][i])<(lru[index_addr][lru_pos]))
						{
							lru[index_addr][i]++;
						}
					}
					lru[index_addr][lru_pos]=0;																					//update LRU
					
					cache_dirty[index_addr][lru_pos]=1;
				}
				else
				{
					this->next->read(addr);
				//	number_of_writebacks++;
					for(int i=0;i<ASSOC;i++)
	   				{
						if((lru[index_addr][i])<(lru[index_addr][lru_pos]))
						{
							lru[index_addr][i]++;
						}
					}
					lru[index_addr][lru_pos]=0;																					//update LRU
					cache_dirty[index_addr][lru_pos]=1;
				}

			}
			
			else
			{
			//write miss
			//	if(lru_dirty == 1)
			//	{
			//	cache_dirty[index_addr][lru_pos] = 0;
			//	}
				//number_of_writebacks++;
				writemiss(tag_addr,index_addr,lru_pos);
				if(lru_dirty==1)
				{
					number_of_writebacks++;
				}
				return;
			}
		}
	}
		
	
//write hit function
	void writehit(unsigned long int tag_addr,unsigned long int index_addr,int pos)
   {

	for(int i=0;i<ASSOC;i++)
	   {
		if((lru[index_addr][i])<(lru[index_addr][pos]))
		{
		lru[index_addr][i]++;
		}
		}
		lru[index_addr][pos]=0;
		cache_dirty[index_addr][pos]=1;
		//2510 3.25 pm added
		if(writebacked==1){
			writebacked=0;
			
		}
	}
	
//write miss function		
   void writemiss(unsigned long int tag_addr, unsigned long int index_addr,int lru_pos)
   {
   		
   		for(int i=0;i<ASSOC;i++)
	  	{
   			if(lru[index_addr][i]==ASSOC-1)
			{
   				cache_tag[index_addr][i]=tag_addr;
   			//	writebacked=0;
   				cache_valid[index_addr][i]=1;
   				lru[index_addr][i]=0;
   				cache_dirty[index_addr][i]=1;
   			//dirty_state=1;
		   }
			else
			{
			
				lru[index_addr][i]++;
			}
		}
		
		if((writebacked==1) && (this->prev!=NULL))
		{
		//	write(NL_addr);
			writebacked=0;	

		}
		
		
	}
	
    
   
    void writeback(unsigned long int NL_addr)
   {
   	//	if(this->next!=NULL)
   		writebacked=1;
//ash
	//	if
		write(NL_addr);
   	//	number_of_writebacks++;

		return;
		
	} 
   

	void printcache_data()
	{	
		
		cache_final=new unsigned long int*[set];
		dirty_final=new bool*[set];
		for(int i=0;i<set;i++)
		{
			cache_final[i]=new unsigned long int[ASSOC];
			dirty_final[i]=new bool[ASSOC];
	//	printf("set: %d  ",i);
		
			for(int j=0;j<ASSOC;j++)
			{	
				x=lru[i][j];
				cache_final[i][x]=cache_tag[i][j];
				dirty_final[i][x]=cache_dirty[i][j];
		//	printf("%d ",i);
		//	printf("%lx  ",cache_tag[i][j]);
			//if(cache_valid[i][j] && cache_dirty[i][j]==1)
			}
		}	
		if (this->prev==NULL)
		{
			printf("\n===== L1 contents =====\n");
		}
		if (this->prev!=NULL)
		{
		//	printf("\n===== L1 contents =====\n");
			printf("===== L2 contents =====\n");
		}
		for(int i=0;i<set;i++)
		{
			printf(" set %3d: ",i);
			for(int j=0;j<ASSOC;j++)
			{
				if(cache_valid[i][j])
				{
					printf(" %lx",cache_final[i][j]);
					if(dirty_final[i][j])printf(" D");
					else printf("  ");
				}
				else
					printf("-  ");
			}
			printf("\n");
		}
		printf("\n");
	//	delete []cache_tag;
	//	delete []cache_dirty;
	}
};	
		/*	if(cache_dirty[i][j]==1)
			{
				printf("D ");
			}
			//else if(cache_valid[i][j] && cache_dirty[i][j]==0){
			else if(cache_dirty[i][j]==0){			
				printf("ND ");}
//			else
//				printf("- ");
				
			printf("%d ",lru[i][j]);
		}
		printf("\n");
	}*/


//}

//	void printcache_data()
//	{	
//		for(int i=0;i<set;i++){
//	//	printf("set: %d  ",i);
//		
//			for(int j=0;j<ASSOC;j++)
//			{
//				printf("%lx  ",cache_tag[i][j]);
//			//if(cache_valid[i][j] && cache_dirty[i][j]==1)
//				if(cache_dirty[i][j]==1)
//				{
//					printf("D ");
//				}
//			//else if(cache_valid[i][j] && cache_dirty[i][j]==0){
//				else if(cache_dirty[i][j]==0){			
//					printf("ND ");}
////			else
////				printf("- ");
//				
//				printf("%d ",lru[i][j]);
//			}
//			printf("\n");
//		}
//
//
//	}	

	

//};

int main (int argc, char* argv[])
{

    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    cache_params params;    // look at sim_cache.h header file for the the definition of struct cache_params
    char rw;         
	int i=0;       // variable holds read/write type read from input file. The array size is 2 because it holds 'r' or 'w' and '\0'. Make sure to adapt in future projects
    unsigned long int addr; // Variable holds the address read from input file
//    int l2_enable =0;
	double miss_rate_L1;
	double miss_rate_L2;
	//double mem_traffic;

    if(argc != 8)           // Checks if correct number of inputs have been given. Throw error and exit if wrong
    {
        printf("Error: Expected inputs:7 Given inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    params.block_size       = strtoul(argv[1], NULL, 10);
    params.l1_size          = strtoul(argv[2], NULL, 10);
    params.l1_assoc         = strtoul(argv[3], NULL, 10);
    params.vc_num_blocks    = strtoul(argv[4], NULL, 10);
    params.l2_size          = strtoul(argv[5], NULL, 10);
    params.l2_assoc         = strtoul(argv[6], NULL, 10);
    trace_file              = argv[7];
	
	Config *L1=new Config(params.block_size, params.l1_size, params.l1_assoc);
	Config *L2;
	if(params.l2_size!=0)
		L2=new Config(params.block_size, params.l2_size, params.l2_assoc);
	
	
	
	if (params.l2_size!=0)
	{
		//L1 = new Config(params.block_size, params.l1_size, params.l1_assoc);
 	    //L2 = new Config(params.block_size, params.l2_size, params.l2_assoc);
 //	    l2_enable=1;
 	    L1->prev=NULL;
        L1->next=L2;
        L2->prev=L1;
        L2->next=NULL;
	}
    
    if (params.l2_size==0)
    {
   // 	l2_enable=0;
    	//Config *L1 = new Config(params.block_size, params.l1_size, params.l1_assoc);
    	L1->prev=NULL;
    	L1->next=NULL;
	}
 //   L1->prev=NULL;
 //   L1->next=NULL;
 //   L1->next=L2;
   // L2->next=NULL;
   // L2->prev=L1;

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    // Print params
  /*  printf(" ===== Simulator configuration =====\n"
            "  BLOCKSIZE:                        %lu\n"
            "  L1_SIZE:                          %lu\n"
            "  L1_ASSOC:                         %lu\n"
            "  VC_NUM_BLOCKS:                    %lu\n"
            "  L2_SIZE:                          %lu\n"
            "  L2_ASSOC:                         %lu\n"
            "  trace_file:                       %s\n"
            "  ===================================\n\n", params.block_size, params.l1_size, params.l1_assoc, params.vc_num_blocks, params.l2_size, params.l2_assoc, trace_file);*/
	
	
	printf("===== Simulator configuration =====\n"
            "  BLOCKSIZE:                        %lu\n"
            "  L1_SIZE:                          %lu\n"
            "  L1_ASSOC:                         %lu\n"
            "  VC_NUM_BLOCKS:                    %lu\n"
            "  L2_SIZE:                          %lu\n"
            "  L2_ASSOC:                         %lu\n"
            "  trace_file:                       %s\n",params.block_size, params.l1_size, params.l1_assoc, params.vc_num_blocks, params.l2_size, params.l2_assoc, trace_file);
          
	
	
	
    char str[2];
    while(fscanf(FP, "%s %lx", str, &addr) != EOF)
    {

        rw = str[0];
//        if(i==10)
//        	break;
        if (rw == 'r')
            L1->read(addr); 
		      // Print and test if file is read correctly
        else if (rw == 'w')
        	L1->write(addr);
        
           // printf("%s %lx\n", "write", addr);         // Print and test if file is read correctly
        /*************************************
                  Add cache code here
        **************************************/
       //i++;
    }
  	
	


	L1->printcache_data();
	unsigned long int mem_traffic_1;
	
	if(params.l2_size!=0)
	{
		L1->next->printcache_data();
		miss_rate_L2 = (double(L2->number_of_read_misses)/L2->number_of_reads);
		//mem_traffic = (double(L2->number_of_read_misses) + L2->number_of_write_misses+ L2->number_of_writebacks);
	}
	
	miss_rate_L1 = ((double(L1->number_of_read_misses) + double(L1->number_of_write_misses))/(double(L1->number_of_reads) + double(L1->number_of_writes)));
//	miss_rate_L2 = ((double(L2->number_of_read_misses) + double(L2->number_of_write_misses))/(double(L2->number_of_reads) + double(L2->number_of_writes)));
	
		
	printf("===== Simulation results =====\n");
    //printf("\n a. number of L1 reads: \t%d",L1->number_of_reads);
    printf("\n  a. number of L1 reads:                       %d",L1->number_of_reads);
    
    printf("\n  b. number of L1 read misses:                 %d",L1->number_of_read_misses);
    printf("\n  c. number of L1 writes:                      %d",L1->number_of_writes);
    printf("\n  d. number of L1 write misses:                 %d",L1->number_of_write_misses);
    printf("\n  e. number of swap requests:                      0");
    printf("\n  f. swap request rate:                       0.0000");
    printf("\n  g. number of swaps:                              0");
    printf("\n  h. combined L1+VC miss rate:                %.4f",miss_rate_L1);
    printf("\n  i. number writebacks from L1/VC:              %d",L1->number_of_writebacks);
    if(L1->next!=NULL)
    {
    	printf("\n  j. number of L2 reads:                       %d",L2->number_of_reads);
		printf("\n  k. number of L2 read misses:                  %d",L2->number_of_read_misses);
		printf("\n  l. number of L2 writes:                       %d",L2->number_of_writes);
		printf("\n  m. number of L2 write misses:                    %d",L2->number_of_write_misses);
		printf("\n  n. L2 miss rate:                            %.4f",miss_rate_L2);
		printf("\n  o. number of writebacks from L2:              %d",L2->number_of_writebacks);
		printf("\n  p. total memory traffic:                      %d",((L2->number_of_read_misses) + (L2->number_of_write_misses) + (L2->number_of_writebacks)));

	//	printf("\n p. total memory traffic: \t%d",mem_traffic);
	}
	else if(L1->next==NULL)
	{
		printf("\n  j. number of L2 reads:                       0");
		printf("\n  k. number of L2 read misses:                  0");
		printf("\n  l. number of L2 writes:                       0");
		printf("\n  m. number of L2 write misses:                    0");
		printf("\n  n. L2 miss rate:                            0.0000");
		printf("\n  o. number of writebacks from L2:              0");
		printf("\n  p. total memory traffic:                      %d",((L1->number_of_read_misses) + (L1->number_of_write_misses) + (L1->number_of_writebacks)));
    
	}
    
    
    
    
    
    

   /* printf("\n b. number of L1 read misses: \t%d",L1->number_of_read_misses);
    printf("\n c. number of L1 writes: \t%d",L1->number_of_writes);
    printf("\n d. number of L1 write misses: \t%d",L1->number_of_write_misses);
    printf("\n e. number of swap requests: 0");
    printf("\n f. swap request rate: \t0.0000");
    printf("\n g. number of swaps:  0");
    printf("\n h. combined L1+VC miss rate: \t%.4f",miss_rate_L1);
    printf("\n i. number writebacks from L1/VC: \t%d",L1->number_of_writebacks);
    if(L1->next!=NULL)
    {
    	printf("\n j. number of L2 reads: \t%d",L2->number_of_reads);
		printf("\n k. number of L2 read misses: \t%d",L2->number_of_read_misses);
		printf("\n l. number of L2 writes: \t%d",L2->number_of_writes);
		printf("\n m. number of L2 write misses: \t%d",L2->number_of_write_misses);
		printf("\n n. L2 miss rate: \t%.4f",miss_rate_L2);
		printf("\n o. number of writebacks from L2: \t%d",L2->number_of_writebacks);
		printf("\n p. total memory traffic: \t%d",((L2->number_of_read_misses) + (L2->number_of_write_misses) + (L2->number_of_writebacks)));

	//	printf("\n p. total memory traffic: \t%d",mem_traffic);
	}
	else if(L1->next==NULL)
	{
		printf("\n j. number of L2 reads: 0");
		printf("\n k. number of L2 read misses: 0");
		printf("\n l. number of L2 writes: 0");
		printf("\n m. number of L2 write misses: 0");
		printf("\n n. L2 miss rate: 0.0000");
		printf("\n o. number of writebacks from L2: 0");
		printf("\n p. total memory traffic: \t%d",((L1->number_of_read_misses) + (L1->number_of_write_misses) + (L1->number_of_writebacks)));

	}*/
    
	printf("\n"); 

    
	return 0;
}









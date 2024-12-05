#include   "header.h"
int main(int argc, char** argv) {

	// Initialize the cache at the beginning
	clear_cache();

	cout << "\n  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" << endl;
	cout << "\t Simulation of an L1 split cache of a 32-bit Processor\n" << endl;
	cout << "\t Team: Anh Tran Minh and Khanh Phan Dinh and Bao Phan Ba Nguyen   " << endl;
	cout << "\n  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" << endl;
	cout << "  Mode 0: Display only the required summary of usage statistics and responses to 9s in the trace file." << endl;
	cout << "  Mode 1: Display everything from mode 1, as well as the communication messages to the L2 cache." << endl;

	do {
		cout << "\n  Please enter the mode number you'd like to select (0,1): ", cin >> mode;
		if (mode > 1) {
			cout << "\n\tInvalid mode value." << endl;
		}
	} while (mode > 1);

	cout << "\n";	//spacing to make it look neater

	// call the parser function, and give it the command line file names
	parser(argc,argv);

	cout << "\n\t --- L1 Split Cache Anaylsis and Simulation Completed! --- " << endl;
	return 0;
}

/* This is the file parser function
*  It reads the trace file and gets the operation and the address from each line in the file
*  The function doesn't return anything, but it takes the program to the respective n functions
*  Depending on the operation n */
void parser(int argc, char** argv) {

	// Reading the trace file from the command line
	char* trace_file = argv[1];		//argv is an array of the strings

	// Check for command line test file
	if (argc != 2) {		// if the count of arguments is not 2
		cout << "\n Not able to read the trace file!" << endl;
		exit(1);			// exit (0) is exit after successfull run, exit(1) exit after failure
	}
		
	char trace_line[1024];								// the address is 8 char (hex) and one black space (1) and one op char = 10 char = 2^10 = 1024 	
	char trace_operation[1];							// n is one character long

	unsigned int operation;								// Operation parsed from input
	unsigned int address;								// Address parsed from input
	FILE* fp;											// .txt test file pointer (fp)

	fp = fopen(trace_file, "r");						// "r" switch of the fopen function opens a file for reading

	while (fgets(trace_line, 1024 , fp)) {

		// sscanf reads the data from trace_line and stores it in trace_operation and address
		sscanf(trace_line, "%1s %x", trace_operation, &address);	// %1s is a switch for a single character string, %x is for hexadecimal in lowercase letters
		if (!strcmp(trace_line, "\n") || !strcmp(trace_line, " ")) { //strcmp compares two strings 
		}
		else {
			operation = atoi(trace_operation);		//Parses the C-string str interpreting its content as an integral number, which is returned as a value of type int.
			cout << "Operation: " << operation << ", Address: " << hex << address << endl; // In giá trị để kiểm tra
			switch (operation) {
			case 0:	cache_read(address);	break;
			case 1:	write(address);			break;
			case 2:	fetch_inst(address);	break;
			case 3:	invalidate(address);	break;	
			case 8:	clear_cache();			break;
			case 9:	print_stats();			break;
			default:
				cout << "\n the value of n (the operation) is not valid \n" << endl;
				break;
			}
		}
	}

	fclose(fp);
	return;
}


/*void*/
void cache_read(unsigned int addr) {

	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set
	bool empty_flag = 0;												// boolean flag for empty way in the cache, if 1, we have an empty way
	int way = -1;														// way in the cache set. Initialized with an invalid way value

	statistics.data_read++;							// Increment the number of reads for the data cache

	way = matching_tag(tag, set, 'D');				// Look for a matching tag already in the data cache
	
	// read cache hit
	if (way >= 0) {									// if we have a matching tag, then we have an data cache hit! (unless invalid MESI state)
		
		statistics.data_hit++;					// Incremenet the data hit counter (We have a new hit!)
		L1_data[way][set].tag_bits = tag;
		L1_data[way][set].set_bits = set;
		L1_data[way][set].address = addr;
		L1_LRU(way, set, 'D');  	// update LRU cout
	}

    // read cache miss
	else {							// if we don't have a matching tag, we have a data cache miss
		statistics.data_miss++;							// Increment the data cache miss counter

        // check empty way
		for (int i = 0; (way < 0 && i < 4); ++i) {		// First, check if we have any empty lines
			if (L1_data[i][set].tag_bits == EMPTY_TAG) {	
				way = i;								// return the way that has an empty line
			}
		}

        // Have empty way
		if (way >= 0) {									// if we have an empty line, place the read data in it
			
         	L1_data[way][set].State = 'V';			// and mark it Valid, it's the only line with this data
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, 'D');			// Update the data cache LRU order/count

			if (mode == 1) {							// Simulating a cache read communication with L2
				cout << "[Data] Read from L2: L1 cache miss, obtain data from L2 " << hex << addr << '\n' << endl;
			}
		}

        // if dont have any empty way 
		else {					// if we don't have any empty lines, we need to evict the LRU line. 
			if (mode == 1) {
				cout << "[Data] Read from L2: L1 cache miss, obtain data from L2 " << hex << addr << '\n' << endl;
			}

			way = find_LRU(set, 'D');				// Find the LRU way in the data cache
			if (way >= 0) {							// if we have a way that's 0, (LRU in the data cache, we use it)
				L1_data[way][set].State = 'V';	// the data here will be new, and Valid to this cache
				L1_data[way][set].tag_bits = tag;
				L1_data[way][set].set_bits = set;
				L1_data[way][set].address = addr;

				L1_LRU(way, set, 'D');	// update the L1 data cache LRU bits
			}

		}
	}
	return ;
}
void clear_cache() {

	cout << "\n\t Clear the cache to the initial state and reset the statistics" << endl;

	// Clear and reset the data cache
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 16384; ++j) {
			L1_data[i][j].State = 'I';			// Reset the MESI protocol to the Invalid state
			L1_data[i][j].LRU_bits = 0;				// reset the LRU bits to 0
			L1_data[i][j].tag_bits = EMPTY_TAG;		// We are using the value 4096 to indicate an empty tag (since 0 - 4095 are used)
			L1_data[i][j].set_bits = 0;				// reset the set bits to 0
			L1_data[i][j].address = 0;				// reset the address to 0
		}
	}


	// Clearing the instruction cache
	for (int n = 0; n < 2; ++n) {
		for (int m = 0; m < 16384; ++m) {
			L1_inst[n][m].State = 'I';		// Reset the MESI protocol to the Invalid state
			L1_inst[n][m].LRU_bits = 0;			// reset the LRU bits to 0
			L1_inst[n][m].tag_bits = EMPTY_TAG;		// We are using the value 4096 to indicate an empty tag (since 0 - 4095 are used)
			L1_inst[n][m].set_bits = 0;			// reset the set bits to 0
			L1_inst[n][m].address = 0;			// reset the address to 0
		}	
	}

	// Clear the instruction cache statistics and reset their values to 0
	statistics.inst_read = 0;
	statistics.inst_hit = 0;
	statistics.inst_miss = 0;
	statistics.inst_hit_ratio = 0.0;

	// Clear the data cache statistics and reset their values to 0
	statistics.data_read = 0;
	statistics.data_write = 0;
	statistics.data_hit = 0;
	statistics.data_miss = 0;
	statistics.data_hit_ratio = 0.0;

	return;
}

void write(unsigned int addr) {


	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set
	bool empty_flag = 0;												// boolean flag for empty way in the cache, if 1, we have an empty way
	int way = -1;														// way in the cache set. Initialized with an invalid way value

	statistics.data_write++;							// Increment the number of write for the data cache

	way = matching_tag(tag, set, 'D');				// Look for a matching tag already in the data cache
	
	if (way >= 0) {		
		// cache write hit							// if we have a matching tag, then we have an data cache hit! (unless invalid MESI state)
		statistics.data_hit++;					// Incremenet the data hit counter (We have a new hit!)
		L1_data[way][set].State = 'M';		// stay in the modified state
		L1_data[way][set].tag_bits = tag;
		L1_data[way][set].set_bits = set;
		L1_data[way][set].address = addr;
		L1_LRU(way, set, 'D');
		
	}

	else {												// Data Cache Miss
		statistics.data_miss++;							// Incremenet the data miss counter 

		if (mode == 1) {								// Simulating a cache Read For Ownership communication from L2
			cout << "[Data] Read for Ownership from L2 " << hex << addr << endl;
		}

		for (int i = 0; way < 0 && i < 4; ++i) {		// First, check if we have any empty lines
			if (L1_data[i][set].tag_bits == EMPTY_TAG) {
				way = i;								// return the way that has an empty line
				empty_flag = 1;							// if we have an empty line, toggle the empty_flag to high
			}
		}

		if (way >= 0) {							// if we have an empty line, place the read data in it
	     	L1_data[way][set].State = 'V';			// go to the Valid state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, 'D');			// Update the data cache LRU order/count

			if (mode == 1) {							// Simulating an iniial write through communication from L2
				cout << "[Data Write Through] Write to L2: we have a data Cache Miss " << hex << addr << endl;
			}
		}

		else {											// if we don't have any empty lines, we need to evict the LRU line. 
			
			if (mode == 1) {							// Simulating an write back communication from L2
				cout << "[Data Write back] Write to L2: we have a data Cache Miss " << hex << addr << endl;
			}

				way = find_LRU(set, 'D');				// Find the LRU way in the data cache
				if (way >= 0) {							// if we have a way that's 0, (LRU in the data cache, we use it)
					L1_data[way][set].State = 'M';	// go to the modified state
					L1_data[way][set].tag_bits = tag;
					L1_data[way][set].set_bits = set;
					L1_data[way][set].address = addr;

					L1_LRU(way, set,  'D');	// update the L1 data cache LRU bits
				}

		}
	}
	return;
}

void fetch_inst(unsigned int addr) {

	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set
	bool empty_flag = 0;												// boolean flag for empty way in the cache, if 1, we have an empty way
	int way = -1;														// way in the cache set. Initialized with an invalid way value

	statistics.inst_read++;							// Increment the number of reads for the data cache
	way = matching_tag(tag, set, 'I');				// Look for a matching tag already in the data cache
	
	// read cache hit

	if (way >= 0) {									// if we have a matching tag, then we have an data cache hit! (unless invalid MESI state)
		
		statistics.inst_hit++;					// Incremenet the data hit counter (We have a new hit!)
		L1_inst[way][set].tag_bits = tag;
		L1_inst[way][set].set_bits = set;
		L1_inst[way][set].address = addr;
		L1_LRU(way, set, 'I');  	// update LRU cout

	}

    // read cache miss
	else {							// if we don't have a matching tag, we have a data cache miss
		statistics.inst_miss++;							// Increment the data cache miss counter

        // check empty way
		for (int i = 0; (way < 0 && i < 2); ++i) {		// First, check if we have any empty lines
			if (L1_inst[i][set].tag_bits == EMPTY_TAG) {	
				way = i;								// return the way that has an empty line
			}
		}

        // Have empty way
		if (way >= 0) {									// if we have an empty line, place the read data in it
			
         	L1_inst[way][set].State = 'V';			// and mark it exclusive, it's the only line with this data
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;

			L1_LRU(way, set,  'I');			// Update the data cache LRU order/count

			if (mode == 1) {							// Simulating a cache read communication with L2
				cout << " [Instruction] Read from L2: L1 cache miss, obtain data from L2 " << hex << addr << '\n' << endl;
			}
		}

        // if dont have any empty way 
		else {					// if we don't have any empty lines, we need to evict the LRU line. 
			if (mode == 1) {
				cout << " [Instruction] Read from L2: L1 cache miss, obtain data from L2 " << hex << addr << '\n' << endl;
			}

			way = find_LRU(set, 'D');				// Find the LRU way in the data cache
			if (way >= 0) {							// if we have a way that's 0, (LRU in the data cache, we use it)
				L1_inst[way][set].State = 'V';	// the data here will be new, and exclusive to this cache
				L1_inst[way][set].tag_bits = tag;
				L1_inst[way][set].set_bits = set;
				L1_inst[way][set].address = addr;
				L1_LRU(way, set, 'I');	// update the L1 data cache LRU bits
			}
		}
	}
	return ;
}

void invalidate(unsigned int addr) {

	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set

	// Search the L1_data cache for the matching tag
	for (int i = 0; i < 4; ++i) {
		if (L1_data[i][set].tag_bits == tag) {

			L1_data[i][set].State = 'I';	
			L1_data[i][set].tag_bits = EMPTY_TAG;
			L1_data[i][set].set_bits = 0;
			L1_data[i][set].address = 0;
		}
		// There's no else. If we don't have a matching tag, do nothing
	}
	return;
}
void print_stats() {


	//Calculate the hit ratio for the data and the instrucion caches
	float data_hit_r = float(statistics.data_hit) / (float(statistics.data_miss) + float(statistics.data_hit));
	float inst_hit_r = float(statistics.inst_hit) / (float(statistics.inst_miss) + float(statistics.inst_hit));

	statistics.data_hit_ratio = data_hit_r;
	statistics.inst_hit_ratio = inst_hit_r;

	cout << "\n \t ** KEY CACHE USAGE STATISTICS ** " << endl;
	if (statistics.data_miss == 0) {				// If we don't have any misses, then no operations took place!
		cout << "no operations took place on the Data cache" << endl;
	}
	else {
		int d =0; // index cout valid line
		cout << "\n\t -- DATA CACHE STATISTICS -- " << endl;

		cout << " Number of Cache Reads: \t" << statistics.data_read << endl;
		cout << " Number of Cache Writes: \t" << statistics.data_write << endl;
		cout << " Number of Cache Hits: \t \t" << dec << statistics.data_hit << endl;
		cout << " Number of Cache Misses: \t" << dec << statistics.data_miss << endl;
		cout << " Cache Hit Ratio: \t\t" << dec << statistics.data_hit_ratio << endl;
		cout << " Cache Hit Ratio percentage: \t" << dec << (statistics.data_hit_ratio * 100) << " %" << endl;
		cout <<endl <<" More information in valid line:  " <<endl ;
		
		for (int i = 0; i < 4; ++i) {
    		for (int j = 0; j < 16384; ++j) {
        		if (L1_data[i][j].State != 'I') {
					d=d+1;	
            		cout << "Line valid : " << hex << L1_data[i][j].address
                		 << "	|	 Way : " << i
						 << "	|	 Set :" << hex<<j
                		 << "	|	 State : " << L1_data[i][j].State 
                		 << "	|	 LRU bit : " <<  L1_data[i][j].LRU_bits 
						 << "	|	 Tag :" << hex<< L1_data[i][j].tag_bits
						 << endl ; 
        			}

	}
}
	
		cout << endl <<"	There are " << dec << d <<" valid lines"<< endl;}

	if (statistics.inst_miss == 0) {				// If we don't have any misses, then no operations took place!
		cout << "\n The cache instruction was not used/not operated on" << endl;
		// cout << statistics.inst_miss << endl;

	}
	else {
		int c =0; // index cout valid line
		cout << "\n\t -- INSTRUCTION CACHE STATISTICS -- " << endl;

		cout << " Number of Cache Reads: \t" << statistics.inst_read << endl;
		cout << " Number of Cache Hits: \t\t" << statistics.inst_hit << endl;
		cout << " Number of Cache Misses: \t" << statistics.inst_miss << endl;
		cout << " Cache Hit Ratio: \t\t" << dec << statistics.inst_hit_ratio << endl;
		cout << " Cache Hit Ratio percentage: \t" << dec << (statistics.inst_hit_ratio) * 100 << " %" << endl;
		cout <<endl <<" More information in valid line:  " <<endl ;
		
		for (int n = 0; n < 2; ++n) {
    		for (int m = 0; m < 16384; ++m) {
        		if (L1_inst[n][m].State != 'I') {
					++c;
            		cout << " Line valid : " << hex << L1_inst[n][m].address 
                		 << "	|	 Way : " << n
						 << "	|	 Set :" << hex <<m
                		 << "	|	 State : " << L1_inst[n][m].State 
                		 << "	|	 LRU bit : " << L1_inst[n][m].LRU_bits 
						 << "	|	 Tag :" << hex<< L1_inst[n][m].tag_bits
						 << endl; 
        			}
    }
}

	cout<< endl << "	There are " <<dec<< c << " valid lines"<<endl;
	}
}

int matching_tag(unsigned int tag, unsigned int set, char which_cache) {
	int i = 0;
	if (which_cache == 'D'|| which_cache == 'd') {		// if flag is 'D' We're searching for a matching tag in the L1 data cache
		// search the data cache for matching tags
	while (L1_data[i][set].tag_bits != tag) {
		i++;
		if (i > 3) {				// We have 4 ways in the data cache 0 through 3
			return -1;				// return -1 to imply that we don't have a matching tag in the data cache. 
		}
	}
	}

	else {							// if flag is 'I' We're searching for a matching tag in the L1 data cache
		int i = 0;
		// Check for matching tags;
		while (L1_inst[i][set].tag_bits != tag) {
			i++;
			if (i > 1) {			// We have 2 ways in the instruction cache 0 through 1
				return -1;			// return -1 to imply that we don't have a matching tag in the instruction cache.
			}
		}
	}
	return i;						// If we have a matching tag, return the way in which we have the matching tag. 
}
void L1_LRU(unsigned int way, unsigned int set, char which_cache) {
	switch (which_cache) {
	case ('D'): 	// We're in the L1_data cache if which_chache is 'D' or 'd'
		for (int i = 0; i < 4; i++) {
    		if (i != way) {
        // Check if the LRU bits need to be decremented
        		if (L1_data[i][set].LRU_bits > L1_data[way][set].LRU_bits) { 
            		L1_data[i][set].LRU_bits--;
        }
    }
}

		L1_data[way][set].LRU_bits = 0x4;	// Set the current set to MRU 11 (0x7 in Hex)
		
		break;
		
	
	case ('I'): 	// We're in the L1_instruction cache if which_chache is 'I' or 'i'
			for (int i = 0; i < 2; i++) {
    			if (i != way) {
        		// Check if the LRU bits need to be decremented
        			if (L1_inst[i][set].LRU_bits > L1_inst[way][set].LRU_bits) { 
            			L1_inst[i][set].LRU_bits--;
        }
    }
}
	L1_inst[way][set].LRU_bits = 0x1; // Set the current set to MRU 1 (0x1 in Hex)

		break;
	}
}


/////////////////////////////////////////////////////////////

/* Find the Least Recently Used Way function
*  This funcion finds the way which includes this set and has an LRU of 0
*  This function takes in he set, and a which_cache flag
*  If which cache is 'D' or 'd' we want to find the Data cache LRU
*  If which cache is 'I' or 'i' we want to find the instruction cache LRU
*  The function returns the way least recently used (to be evicted). 
*  If we can't find a LRU way, something is wrong, and we return -1
*/
int find_LRU(unsigned int set, char which_cache) {
	switch (which_cache) {
	case ('d'):
	case ('D'): 	// We're in the L1_data cache if which_chache is 'D' or 'd'
		for (int i = 0; i < 4; ++i) {
			if (L1_data[i][set].LRU_bits == 1) {
				return i; // return way 
			}
		}
	//break;

	case ('i'):
	case ('I'): 	// We're in the L1_instruction cache if which_chache is 'I' or 'i'

		for (int i = 0; i < 2; ++i) {
			if (L1_inst[i][set].LRU_bits == 1) {
				return i; // return way
			}
		}
	//	break;
	}
	return -1; // This means that something is wrong! We expected an LRU (0) but we couldn't find any
	}


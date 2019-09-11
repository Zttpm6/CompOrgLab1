#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"
uint32_t prevInstruction;


uint32_t extend_sign( uint32_t im )
{
	uint32_t data = ( im & 0x0000FFFF );
	uint32_t mask = 0x00008000;
	if ( mask & data ) 
	{
		data = data | 0xFFFF0000;
	}

	return data;
}

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/

	uint32_t ins = mem_read_32(CURRENT_STATE.PC);
	printf("\nInstruction: %08x ", ins);
	uint32_t opcode = (0xFC000000 & ins);
	uint32_t jump = 0x4;
	
	printf("\nOpcode: %0x8\n", opcode);
	switch (opcode)
	{
	    //R statement
	    case 0x00000000:
		{
	        //rs mask
	        uint32_t rs = (0x03E00000 & ins) >> 21;
	        //rt mask
	        uint32_t rt = (0x001F0000 & ins) >> 16;
	        //rd mask
	        uint32_t rd = (0x0000F800 & ins) >> 11;
	        //sa mask
	        uint32_t sa = (0x000007C0 & ins) >> 6;
	        //func mask
	        uint32_t func = (0x0000003F & ins);

	        printf("\nR type instruction\n"
               "rs : %x\n"
               "rt : %x\n"
               "rd : %x\n"
               "sa : %x\n"
               "func : %x\n", rs,rt,rd,sa,func);
	        switch(func)
			{
	            //Add
                case 0x00000020:{
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
                    break;

                }
                //ADDU add unsigned
	            case 0x00000021:{
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
                    break;
	            }
	            //Sub
	            case 0x00000022:{
	                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
	                break;
	            }
	            //Subu
	            case 0x00000023:{
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
                    break;
	            }
	            //MUlT
	            case 0x00000018:{

                    if(prevInstruction == 0x0000012 || prevInstruction == 0x0000011)
                    {
                        puts("Result is undefined");
                        break;
                    }
                    uint64_t tempResult = CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt];

                    NEXT_STATE.LO = tempResult & 0xFFFFFFFF; //get low 32-bits
                    NEXT_STATE.HI = tempResult >> 32; //get high 32-bits
                    break;
	            }
	            //Multu
	            case 0x00000019:{


                    //if either of the 2 preceding instructions were MFLO or MFHI, result is undefined
                    if(prevInstruction == 0x0000012 || prevInstruction == 0x0000011)
                    {
                        puts("Result is undefined");
                        break;
                    }

                    uint64_t tempResult = CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt];

                    NEXT_STATE.LO = tempResult & 0xFFFFFFFF; //get low 32-bits
                    NEXT_STATE.HI = tempResult >> 32; //get high 32-bits
	                break;
	            }
                    //DIV
                case 0x0000001A: {

                    //if either of the 2 preceding instructions were MFLO or MFHI, result is undefined
                    if (prevInstruction == 0x0000012 || prevInstruction == 0x0000011 || CURRENT_STATE.REGS[rt] == 0) {
                        puts("Result is undefined");
                        break;
                    }

                    NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt]; //get quotient
                    NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];  //get remainder
                    break;
                }
                //DIVU
                case 0x0000001B: {


                    //if either of the 2 preceding instructions were MFLO or MFHI, result is undefined
                    if (prevInstruction == 0x0000012 || prevInstruction == 0x0000011 || CURRENT_STATE.REGS[rt] == 0) {
                        puts("Result is undefined");
                        break;
                    }

                    NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt]; //get quotient
                    NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];  //get remainder
                    break;
                }
	            //AND
	            case 0x00000024:{
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
                    break;
	            }
	            //or
	            case 0x00000025:{

                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] || CURRENT_STATE.REGS[rt];
	                break;
	            }
	            //xor
	            case 0x00000026:{
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
                    break;
	            }
	            //NOR
	            case 0x00000027:{
	                NEXT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
	                break;
	            }
	            //SLT
	            case 0x0000002A:{
	                if(CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt]){
	                    NEXT_STATE.REGS[rd] = 0x00000001;
	                }
	                else{
                        NEXT_STATE.REGS[rd] = 0x00000000;
	                }
	                break;
	            }
	            //SLL
	            case 0x00000000:{
	                uint32_t temp = CURRENT_STATE.REGS[rt] << sa;
	                NEXT_STATE.REGS[rd] = temp;
	                break;
	            }
	            //SRL
                case 0x00000002:{
                    uint32_t temp = CURRENT_STATE.REGS[rt] >> sa;
                    NEXT_STATE.REGS[rd] = temp;
                    break;
                }
	            //SRA
                case 0x00000003:
                {
                    uint32_t temp;
                    int x;
                    uint32_t hiBit = CURRENT_STATE.REGS[rt] & 0x80000000;
                    if(hiBit == 1)
                    {
                        temp = CURRENT_STATE.REGS[rt];
                        for( x = 0; x < sa; x++ )
                        {
                            temp = ((temp >> 1) | 0x80000000);
                        }
                    }
                    else
                    {
                        temp = CURRENT_STATE.REGS[rt] >> sa;
                    }
                    NEXT_STATE.REGS[rd] = temp;
                    break;
                }
	            //JR
                case 0x00000008:{
                    uint32_t temp = CURRENT_STATE.REGS[rs];
                    jump = temp - CURRENT_STATE.PC;
                    break;
                }
	            //JALR
                case 0x00000009:{
                    uint32_t temp = CURRENT_STATE.REGS[rs];
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 0x8;
                    jump = temp - CURRENT_STATE.PC;
                    break;
                }
	            //MTLO
                case 0x00000013:{
                    NEXT_STATE.LO = CURRENT_STATE.REGS[rs];
                    break;
                }
	            //MTHI
                case 0x00000011:{
                    NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
                    break;
                }
	            //MFLO
                case 0x00000012:{
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
                    break;
                }
	            //MFHI
                case 0x00000010:{
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
                    break;
                }
	            //SYSCALL
                case 0x0000000C: {
                    //SYSCALL - System Call, exit the program.
                    NEXT_STATE.REGS[0] = 0xA;
                    puts("Terminate");
                    RUN_FLAG = FALSE;
                    break;
                }
            }
			break;
		}
				
        //I-type statement
        default :
        {
            //rs mask
            uint32_t rs = (0x03E00000 & ins) >> 21;
            //rt mask
            uint32_t rt = (0x001F0000 & ins) >> 16;
            //im mask
            uint32_t im = (0x00000FFFF& ins);

            printf("\nI-type instruction\n"
                   "rs : %x\n"
                   "rt : %x\n"
                   "im : %x\n",rs,rt,im);
				   
            switch (opcode)
            {
                case 0x20000000:
                {
                    //ADDI
                    puts( "ADDI" );
                    NEXT_STATE.REGS[rt] = extend_sign(im) + CURRENT_STATE.REGS[rs];
                    break;

                }
                case 0x24000000:
                {
                    //ADDIU
                    puts( "ADDIU" );
                    NEXT_STATE.REGS[rt] = extend_sign(im) + CURRENT_STATE.REGS[rs];
                    break;
                }
                case 0x30000000:
                {
                    //ANDI
                    puts( "ANDI" );
                    NEXT_STATE.REGS[rt] = (im & 0x0000FFFF) & CURRENT_STATE.REGS[rs];
                    break;
                }
                case 0x34000000:
                {
                    //ORI
                    puts( "ORI" );
                    NEXT_STATE.REGS[rt] = (im & 0x0000FFFF) | CURRENT_STATE.REGS[rs];
                    break;
                }
                case 0x38000000:
                {
                    //XORI
                    puts( "XORI" );
                    NEXT_STATE.REGS[rt] = (im & 0x0000FFFF) ^ CURRENT_STATE.REGS[rs];
                    break;
                }
                case 0x28000000:
                {
                    //Set On Less Than Immediate
                    puts("SLTI");
					if(CURRENT_STATE.REGS[rs] < extend_sign(im))
					{
						NEXT_STATE.REGS[rt] = 1;
					}
					else
					{
						NEXT_STATE.REGS[rt] = 0;
					}
                }
                case 0x8C000000:
                {
                    //load word
                    puts("LW");
                    uint32_t offset = extend_sign( im );
                    uint32_t eAddr = offset + CURRENT_STATE.REGS[rs];
                    NEXT_STATE.REGS[rt] = mem_read_32( eAddr );
                    break;
                }
                case 0x80000000:
                {
                    //Load Byte
                    puts("LB");
                    uint32_t offset = extend_sign( im );
                    uint32_t eAddr = offset + CURRENT_STATE.REGS[rs];
                    NEXT_STATE.REGS[rt] = 0x0000000F | mem_read_32( eAddr );
                    break;

                }
                case 0x84000000:
                {
                    //Load Halfword
                    puts("LH");
                    uint32_t offset = extend_sign( im );
                    uint32_t eAddr = offset + CURRENT_STATE.REGS[rs];
                    NEXT_STATE.REGS[rt] = 0x000000FF | mem_read_32( eAddr );
                    break;
                }
				case 0x3C000000:
                {
                    //Load Upper Immediate
                    puts("LUI");
                    NEXT_STATE.REGS[rt] = (im << 16);
                    break;
                }
				case 0xAC000000:
                {
                    //Store word
                    puts("SW");
					uint32_t offset = extend_sign( im );
					uint32_t eAddr = offset + CURRENT_STATE.REGS[rs];
                    mem_write_32( eAddr, CURRENT_STATE.REGS[rt] );
                    break;
                }
				case 0xA000000:
				{
					//Store byte
                    puts("SB");
					uint32_t offset = extend_sign( im );
					uint32_t eAddr = offset + CURRENT_STATE.REGS[rs];
                    mem_write_32( eAddr, CURRENT_STATE.REGS[rt] );
                    break;
				}
				case 0xA4000000:
				{
					//Store Halfwood
					puts("SH");
					uint32_t offset = extend_sign( im );
					uint32_t eAddr = offset + CURRENT_STATE.REGS[rs];
                    mem_write_32( eAddr, CURRENT_STATE.REGS[rt] );
                    break;
				}
				case 0x10000000:
				{
					//BEQ
					uint32_t tar = extend_sign( im ) << 2;
					if( CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt] )
					{
						jump = tar;
					}
					break;
				}
				case 0x14000000:
				{
					//Branch on Not Equal
					puts("BNE");
					uint32_t tar = extend_sign( im ) << 2;
					if( CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt] )
					{
						jump = tar;
					}
                    break;
				}
				case 0x18000000:
				{
					//Branch on Less Than or Equal to Zero
					puts("BLEZ");
					uint32_t tar = extend_sign( im ) << 2;
					if( ( CURRENT_STATE.REGS[rs] & 0x80000000 ) || ( CURRENT_STATE.REGS[rt] == 0 ) )
					{
						jump = tar;
					}
                    break;
				}
				case 0x1C000000:
				{
					//Branch on Greater Than Zero
					puts("BGTZ");
					uint32_t tar = extend_sign( im ) << 2;
					if( !( CURRENT_STATE.REGS[rs] & 0x80000000 ) || ( CURRENT_STATE.REGS[rt] != 0 ) )
					{
						jump = tar;
					}
					break;
				}
				case 0x04000000:
				{
					//REGIMM
					switch(rt)
					{
						case 0x00000000:
						{
							//Branch On Less Than Zer0
							puts("BLTZ");
							uint32_t tar = ( extend_sign( im ) << 2 );
							if( ( CURRENT_STATE.REGS[rs] & 0x80000000 ) )
							{
								jump = tar;
							}
							break;
						}
						case 0x00000001:
						{
							//BGEZ - Branch on Greater Than or Equal to Zero
							uint32_t tar = ( extend_sign( im ) << 2 );
							if( !( CURRENT_STATE.REGS[rs] & 0x80000000 ) )
							{
								jump = tar;
							}
							break;
						}
					}
				}
            }
			break;
        }
	}
	https://github.com/Zttpm6/CompOrgLab1.git
	NEXT_STATE.PC = CURRENT_STATE.PC + jump;
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr)
{
	uint32_t ins = mem_read_32(addr);
	uint32_t opcode = (0xFC000000 & ins);
	switch (opcode)
	{
		//R-Type statement
		case 0x00000000: 
		{
			//rs mask
	        uint32_t rs = (0x03E00000 & ins) >> 21;
	        //rt mask
	        uint32_t rt = (0x001F0000 & ins) >> 16;
	        //rd mask
	        uint32_t rd = (0x0000F800 & ins) >> 11;
	        //sa mask
	        uint32_t sa = (0x000007C0 & ins) >> 6;
	        //func mask
	        uint32_t func = (0x0000003F & ins); 
			switch(func)
			{
                case 0x00000000:
                {
                    //rs MASK: 0000 0011 1110 0000 4x0000 = 03E00000
                    uint32_t rs = ( 0x03E00000 & ins  ) >> 21;
                    //rt MASK: 0000 0000 0001 1111 4x0000 = 001F0000;
                    uint32_t rt = ( 0x001F0000 & ins  ) >> 16;
                    //rd MASK: 4x0000 1111 1000 0000 0000 = 0000F800;
                    uint32_t rd = ( 0x0000F800 & ins  ) >> 11;
                    //sa MASK: 4X0000 0000 0111 1100 0000 = 000007C0;
                    uint32_t sa = ( 0x000007C0 & ins  ) >> 6;
                    //func MASK: 6x0000 0001 1111 = 0000001F
                    uint32_t func = ( 0x0000003F & ins );

                    switch( func )
                    {
                        case 0x00000020:
                            //ADD
                            printf( "\n\nADD Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000021:
                            //ADDU
                            printf( "\n\nADDU Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000022:
                            //SUB
                            printf( "\n\nSUB Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000023:
                            //SUBU
                            puts( "Subtract Unsigned Function" );
                            printf( "\n\nSUBU Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000018:
                        {
                            //MULT
                            printf( "\n\nMULT Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;
                        }

                        case 0x00000019:
                        {
                            //MULTU
                            printf( "\n\nMULTU Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;
                        }

                        case 0x0000001A:
                            //DIV
                            printf( "\n\nDIV Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x0000001B:
                            //DIVU
                            printf( "\n\nDIVU Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0X00000024:
                            //AND
                            printf( "\n\nAND Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0X00000025:
                            //OR
                            printf( "\n\nOR Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0X00000026:
                            //XOR
                            printf( "\n\nXOR Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000027:
                            //NOR
                            printf( "\n\nNOR Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x0000002A:
                            //SLT
                            printf( "\n\nSLTs Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000000:
                        {
                            //SLL
                            printf( "\n\nSLL Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;
                        }

                        case 0x00000002:
                        {
                            //SRL
                            printf( "\n\nSRL Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;
                        }

                        case 0x00000003:
                        {
                            //SRA
                            printf( "\n\nSRA Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;
                        }
                        case 0x0000000C:
                            //SYSCALL - System Call, exit the program.
                            printf( "\n\nSYSCALL(exit) Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000008:
                            //JR
                            printf( "\n\nJR Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;
                        case 0x00000009:
                            //JALR
                            printf( "\n\nJALR Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x00000013:
                            //MTLO
                            printf( "\n\nMTLO Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x0000011:
                            //MTHI
                            printf( "\n\nMTHI Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x0000012:
                            //MFLO
                            printf( "\n\nMFLO Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;

                        case 0x0000010:
                            //MFHI
                            printf( "\n\nMFHI Instruction:"
                                    "\n-> OC: %x"
                                    "\n-> rs: %x"
                                    "\n-> rt: %x"
                                    "\n-> rd: %x"
                                    "\n-> shamt: %x"
                                    "\n-> func: %x\n",
                                    opcode, rs, rt, rd, sa, func );
                            break;
                    }
                    prevInstruction = func;
                    break;

                }
                case 0x08000000:
                {
                    //JL
                    uint32_t target = ( 0x03FFFFFF & ins  );
                    printf( "\n\nJL Instruction:"
                            "\n-> OC: %x"
                            "\n-> target: %x\n",
                            opcode, target );
                    break;
                }

                case 0x0C000000:
                {
                    //JAL-Jump and Link Instruction
                    uint32_t target = ( 0x03FFFFFF & ins  );
                    printf( "\n\nJAL Instruction:"
                            "\n-> OC: %x"
                            "\n-> target: %x\n",
                            opcode, target );
                    break;
                }
			}
		}
	//I-type statement
    default :
        {
            //rs mask
            uint32_t rs = (0x03E00000 & ins) >> 21;
            //rt mask
            uint32_t rt = (0x001F0000 & ins) >> 16;
            //im mask
            uint32_t im = (0x00000FFFF& ins);	   
            switch (opcode)
			{
				case 0x20000000:
				{
					//ADDI
					printf("\nADDI Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x24000000:
				{
					//ADDIU
					printf("\nADDIU Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x30000000:
				{
					//ANDI
					printf("\nANDI Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x34000000:
				{
					//ORI
					printf("\nORI Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x38000000:
				{
					//XORI
					printf("\nXORI Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x28000000:
				{
					//SLTI
					printf("\nSLTI Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x8C000000:
				{
					//LW
					printf("\nLW Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x80000000:
				{
					//LB
					printf("\nLB Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x84000000:
				{
					//LH
					printf("\nLH Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x3C000000:
				{
					//LUI
					printf("\nLUI Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0xAC000000:
				{
					//SW
					printf("\nSW Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0xA000000:
				{
					//SB
					printf("\nSB Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0xA4000000:
				{
					//SH
					printf("\nSH Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x10000000:
				{
					//BEQ
					printf("\nBEQ Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x14000000:
				{
					//BNE
					printf("\nBNE Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x18000000:
				{
					//BLEZ
					printf("\nBLEZ Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x1C000000:
				{
					//BGTZ
					printf("\nBGTZ Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
					break;
				}
				case 0x04000000:
				{
					//REGIMM
					switch(rt)
					{
						case 0x00000000:
						{
							//BLTZ
							printf("\nBLTZ Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
							break;
						}
						case 0x00000001:
						{
							//BGEZ
							printf("\nBGEZ Instruction:\nOpcode: %x \nrs: %x\nrt: %x\nImmediate: %x\n", opcode,rs,rt,im);
							break;
						}
						break;
					}
				}
			}
		}
	}
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}

/**************************************************************/
/* CS/COE 1541
   just compile with gcc -o CPU CPU.c
   and execute using
   ./CPU  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h"

#define HASHSIZE 64

//Function prototype
int hazardCheck(struct trace_item *a, struct trace_item *b, struct trace_item *c, struct trace_item *d, struct trace_item *e, struct trace_item *f, struct trace_item *g);


int main(int argc, char **argv)
{
  struct trace_item *tr_entry;

  //Initialize Hash Table
  int hashTable[HASHSIZE];
  int i;
  for (i = 0; i < HASHSIZE; i++)
    hashTable[i] = -1;

  //Initialize NOP and Squash instuction
  struct trace_item NOP = {ti_NOP, 0, 0, 0, 0, 0};

  //Create and initialize pipeline stages
  struct trace_item *IF1 = &NOP;
  struct trace_item *IF2 = &NOP;
  struct trace_item *ID = &NOP;
  struct trace_item *EX = &NOP;
  struct trace_item *MEM1 = &NOP;
  struct trace_item *MEM2 = &NOP;
  struct trace_item *WB = &NOP;


  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  int prediction_type = 0;

  unsigned char t_type = 0;
  unsigned char t_sReg_a= 0;
  unsigned char t_sReg_b= 0;
  unsigned char t_dReg= 0;
  unsigned int t_PC = 0;
  unsigned int t_Addr = 0;

  unsigned int cycle_number = 0;

  if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }

  //--------Modifying so when a trace view and/or prediction type is not specified, it automatically goes to zero.
  trace_file_name = argv[1];
  if (argc == 2)
  {
	  trace_view_on = 0;
	  prediction_type = 0;
  }
  else if (argc == 3)
  {
	  prediction_type = atoi(argv[2]);
	  trace_view_on = 0;
  }
  else if (argc == 4) //follows order requirement
  {
	  trace_view_on = atoi(argv[3]);
	  prediction_type = atoi(argv[2]);
  }
  else
  {
	  printf("Error: invalid amount of arguments");
	  exit(0);
  }
  //--------end argument modification





  //--------Opening file and getting started
  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();
  //----------------------




  int remaining = 6;
  int hazardType = 0;
  int hashIndex = 0;
  int squashCount = 0;
  int nopCount = 0;
  int foundControlHazard = 0;

  while(1) {
    foundControlHazard = 0;

    if (!size && remaining == 0)
    {
      /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      printf("squashes: %d\n", squashCount);
      printf("nops: %d\n", nopCount);
      break;
    }
    else
    {
      if (!size)
      {
        //Allows the pipeline to coninute to execute the rest of the instructions even though there are no more coming in
        remaining--;
      }
      
        //Check for hazards
      {

        //Check if there is a control hazard and update hash table if branch
        if (EX->type == ti_BRANCH)
        {
          //Get hash index
          hashIndex = EX->PC;
          hashIndex = hashIndex >> 3;
          hashIndex = hashIndex % HASHSIZE;

          // No prediction
          if (prediction_type == 0 && EX->Addr != ID->PC) 
          {
            //Squash intstructions
            //Get new instruction
            size = trace_get_item(&tr_entry);

            WB = MEM2;
            MEM2 = MEM1;
            MEM1 = EX;
            EX = ID;
            EX->type = 's'; //SQUASHED
            ID = IF2;
            ID->type = 's'; //SQUASHED
            IF2 = IF1;
            IF2->type = 's'; //SQUASHED
            IF1 = tr_entry;
            hazardType = 0;
            squashCount += 3;
            foundControlHazard = 1;
          }


          // 1-bit prediction
          else if (prediction_type == 1)
          {
            if (hashTable[hashIndex] == -1 || hashTable[hashIndex] == 0) //no prediction yet or predict not taken, either way use "not taken" policy
            {
              if (EX->Addr == ID->PC) //prediction wrong, branch taken
              {
                //update hash table
                hashTable[hashIndex] = 1;

                //squash instructions
                //Get new instruction
                size = trace_get_item(&tr_entry);

                WB = MEM2;
                MEM2 = MEM1;
                MEM1 = EX;
                EX = ID;
                EX->type = 's'; //SQUASHED
                ID = IF2;
                ID->type = 's'; //SQUASHED
                IF2 = IF1;
                IF2->type = 's'; //SQUASHED
                IF1 = tr_entry;
                hazardType = 0;
                squashCount += 3;
                foundControlHazard = 1;
              }
              else //prediction correct, branch not taken
              {
                //update hash table
                hashTable[hashIndex] = 0;
              }
            }
            else if (hashTable[hashIndex] == 1) 
            {
              if (EX->Addr == ID->PC) //prediction correct, branch taken
              {
                //update hash table
                hashTable[hashIndex] = 1; //unnecessary, but putting this line here helps with organizing
              }
              else //prediction wrong, branch not taken
              {
                //update hash table
                hashTable[hashIndex] = 0; 

                //squash instructions
                //Get new instruction
                size = trace_get_item(&tr_entry);

                WB = MEM2;
                MEM2 = MEM1;
                MEM1 = EX;
                EX = ID;
                EX->type = 's'; //SQUASHED
                ID = IF2;
                ID->type = 's'; //SQUASHED
                IF2 = IF1;
                IF2->type = 's'; //SQUASHED
                IF1 = tr_entry;
                hazardType = 0;
                squashCount += 3;
                foundControlHazard = 1;
              }
            }
          }


          // 2-bit prediction
          else if (prediction_type == 2)
          {
            if (hashTable[hashIndex] == -1) //"not taken policy"
            {
              if (EX->Addr == ID->PC) //prediction wrong, branch taken
              {
                //update hash table
                hashTable[hashIndex] = 1;

                //squash instructions
                //Get new instruction
                size = trace_get_item(&tr_entry);

                WB = MEM2;
                MEM2 = MEM1;
                MEM1 = EX;
                EX = ID;
                EX->type = 's'; //SQUASHED
                ID = IF2;
                ID->type = 's'; //SQUASHED
                IF2 = IF1;
                IF2->type = 's'; //SQUASHED
                IF1 = tr_entry;
                hazardType = 0;
                squashCount += 3;
                foundControlHazard = 1;
              }
              else //prediction correct, branch not taken
              {
                //update hash table
                hashTable[hashIndex] = 0;
              }
            }
            else
            {
              if (hashTable[hashIndex] == 0)
              {
                if (EX->Addr == ID->PC) //prediction wrong, branch taken
                {
                  //update hash table
                  hashTable[hashIndex] = 1;

                  //squash instructions
                  //Get new instruction
                  size = trace_get_item(&tr_entry);

                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = ID;
                  EX->type = 's'; //SQUASHED
                  ID = IF2;
                  ID->type = 's'; //SQUASHED
                  IF2 = IF1;
                  IF2->type = 's'; //SQUASHED
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
                  foundControlHazard = 1;
                }
                else //prediction correct, branch not taken
                {
                  hashTable[hashIndex] = 0;
                }
              }
              else if (hashTable[hashIndex] == 1)
              {
                if (EX->Addr == ID->PC) //prediction wrong, branch taken
                {
                  //update hash table
                  hashTable[hashIndex] = 3;

                  //squash instructions
                  //Get new instruction
                  size = trace_get_item(&tr_entry);

                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = ID;
                  EX->type = 's'; //SQUASHED
                  ID = IF2;
                  ID->type = 's'; //SQUASHED
                  IF2 = IF1;
                  IF2->type = 's'; //SQUASHED
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
                  foundControlHazard = 1;
                }
                else //prediction correct, branch not taken
                {
                  hashTable[hashIndex] = 0;
                }
              }
              else if (hashTable[hashIndex] == 2)
              {
                if (EX->Addr == ID->PC) //prediction correct, branch taken
                {
                  //update hash table
                  hashTable[hashIndex] = 3;
                }
                else //prediction wrong, branch not taken
                {
                  //update hash table
                  hashTable[hashIndex] = 0;

                  //squash instructions
                  //Get new instruction
                  size = trace_get_item(&tr_entry);

                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = ID;
                  EX->type = 's'; //SQUASHED
                  ID = IF2;
                  ID->type = 's'; //SQUASHED
                  IF2 = IF1;
                  IF2->type = 's'; //SQUASHED
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
                  foundControlHazard = 1;
                }
              }
              else if (hashTable[hashIndex] == 3)
              {
                if (EX->Addr == ID->PC) //prediction correct, branch taken
                {
                  //update hash table
                  hashTable[hashIndex] = 3;
                }
                else //prediction wrong, branch not taken
                {
                  //update hash table
                  hashTable[hashIndex] = 2;

                  //squash instructions
                  //Get new instruction
                  size = trace_get_item(&tr_entry);

                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = ID;
                  EX->type = 's'; //SQUASHED
                  ID = IF2;
                  ID->type = 's'; //SQUASHED
                  IF2 = IF1;
                  IF2->type = 's'; //SQUASHED
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
                  foundControlHazard = 1;
                }
              }
            }
          }
        }

        
        // If a cycle did not already execute to fix a control hazard
        if (!foundControlHazard)
        {

        //Now check for structural or data hazard
        hazardType = hazardCheck(IF1, IF2, ID, EX, MEM1, MEM2, WB);

          switch (hazardType) {
          case 0: //No hazard, normal cycle
            size = trace_get_item(&tr_entry);
            WB = MEM2;
            MEM2 = MEM1;
            MEM1 = EX;
            EX = ID;
            ID = IF2;
            IF2 = IF1;
            IF1 = tr_entry;
            break;


          case 1: //Structural hazard
            WB = MEM2;
            MEM2 = MEM1;
            MEM1 = EX;
            EX = &NOP;
            hazardType = 0;
            nopCount++;
            break;


          case 2: //Data hazard A
            WB = MEM2;
            MEM2 = MEM1;
            MEM1 = &NOP;
            hazardType = 0;
            nopCount++;
            break;


          case 3: //Data hazard B
            WB = MEM2;
            MEM2 = &NOP;
            hazardType = 0;
            nopCount++;
            break;
         
          }
           
        }
      }
    }


    //Parse finishing instruction to be printed
    cycle_number++;
    t_type = WB->type;
    t_sReg_a = WB->sReg_a;
    t_sReg_b = WB->sReg_b;
    t_dReg = WB->dReg;
    t_PC = WB->PC;
    t_Addr = WB->Addr;
    

    if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
      switch(WB->type) {
        case 's':
          printf("[cycle %d]",cycle_number);
          printf(" SQUASHED --> (PC: %x)\n", WB->PC);
          break;
        case ti_NOP:
          printf("[cycle %d] NOP\n",cycle_number) ;
          break;
        case ti_RTYPE:
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", WB->PC, WB->sReg_a, WB->sReg_b, WB->dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", WB->PC, WB->sReg_a, WB->dReg, WB->Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", WB->PC, WB->sReg_a, WB->dReg, WB->Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", WB->PC, WB->sReg_a, WB->sReg_b, WB->Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", WB->PC, WB->sReg_a, WB->sReg_b, WB->Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
          printf(" (PC: %x)(addr: %x)\n", WB->PC,WB->Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
          printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", WB->PC, WB->dReg, WB->Addr);
          break;
      }
    }
  }

  trace_uninit();

  exit(0);
}




//retuns an integer based on the hazard
//1 - Structural Hazard, stall IF1, IF2, and ID
//2 - Data Hazard a) stall ID and EX, insert NO-OP into MEM1
//3 - Data Hazard b) stall ID and EX, insert NO-OP into MEM2

int hazardCheck(struct trace_item *IF1inst, struct trace_item *IF2inst, struct trace_item *IDinst, struct trace_item *EXinst, struct trace_item *MEM1inst, struct trace_item *MEM2inst, struct trace_item *WBinst ){
  
  // true if a load is followed by a any inst that could use 
  //the register that is being loadedwhere the branch depends on
  if (MEM1inst->type == ti_LOAD ){
    if((EXinst->type == ti_ITYPE  || EXinst->type == ti_JRTYPE) && EXinst->sReg_a == MEM1inst->dReg)
      return 2;
    else if ((EXinst->type == ti_RTYPE  || EXinst->type == ti_STORE || EXinst->type == ti_BRANCH) && ( EXinst->sReg_b == MEM1inst->dReg || EXinst->sReg_a == MEM1inst->dReg))
      return 2;
  }
  
  // true if a load is followed by a any inst that could use 
  //the register that is being loadedwhere the branch depends on
  if (MEM2inst->type == ti_LOAD ){
    if((EXinst->type == ti_ITYPE  || EXinst->type == ti_JRTYPE) && EXinst->sReg_a == MEM2inst->dReg)
      return 3;
    else if ((EXinst->type == ti_RTYPE || EXinst->type == ti_STORE || EXinst->type == ti_BRANCH) && (EXinst->sReg_b == MEM2inst->dReg || EXinst->sReg_a == MEM2inst->dReg))
      return 3;
  }
  
  // true if the WB instruction is a writing (type 1 or 2) and the ID instruction is a I 
  //or R type and the destination register of WB matches either source register of ID
  if (WBinst->type == ti_RTYPE || WBinst->type == ti_ITYPE){
    if((IDinst->type == ti_ITYPE || IDinst->type ==ti_JRTYPE) && IDinst->sReg_a == WBinst->dReg)
      return 1;
    else if ((IDinst->type == ti_RTYPE  || IDinst->type == ti_STORE || IDinst->type== ti_BRANCH)&& (IDinst->sReg_a == WBinst->dReg || IDinst->sReg_b == WBinst->dReg)) 
      return 1;
  }
    
  
  return 0;
}

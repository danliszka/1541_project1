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

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;

  //Initialize Hash Table
  int hashTable[HASHSIZE];
  for (int i = 0; i < HASHSIZE; i++)
    hashTable[i] = -1;

  //Initialize NOP and Squash instuction
  struct trace_item NOP = {ti_NOP, 0, 0, 0, 0, 0};
  struct trace_item squash = {'s', 0, 0, 0, 0, 0};

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

  while(1) {

    if (hazardType == 0 || hazardType == 4)
    {
      size = trace_get_item(&tr_entry);
    }
    if (!size && remaining == 0)
    {
      /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else
    {
      if (!size)
      {
        //Allows the pipeline to coninute to execute the rest of the instructions even though there are no more coming in
        remaining--;
      }
      if (hazardType == 0)
      {
        //Sends every intruction to the next stage in the pipeline
        WB = MEM2;
        MEM2 = MEM1;
        MEM1 = EX;
        EX = ID;
        ID = IF2;
        IF2 = IF1;
        IF1 = tr_entry;
      }
      else//There are stalls or squashes that need to be made
      {

        //Check if there is a control hazard and update hash table if branch
        if (EX->type == ti_BRANCH)
        {
          //Get hash index
          hashIndex = EX->Addr;
          hashIndex = hashIndex >> 3;
          hashIndex = hashIndex % HASHSIZE;

          if (prediction_type == 0 && EX->Addr != ID->PC) // Squash instructions, not taken policy false
          {
            WB = MEM2;
            MEM2 = MEM1;
            MEM1 = EX;
            EX = SQUASHED;
            ID = SQUASHED;
            IF2 = SQUASHED;
            IF1 = tr_entry;
            hazardType = 0;
            squashCount += 3;
          }


          // 1-bit prediction
          else if (prediction_type == 1)
          {
            if (hashTable[hashIndex] == -1 || hashTable[hashIndex] == 0) //no prediction yet or predict not taken, either way use "not taken" policy
            {
              if (EX->Addr == ID->PC) //prediction wrong, branch taken
              {
                //update hash table
                hashIndex[hashIndex] = 1;

                //squash instructions
                WB = MEM2;
                MEM2 = MEM1;
                MEM1 = EX;
                EX = SQUASHED;
                ID = SQUASHED;
                IF2 = SQUASHED;
                IF1 = tr_entry;
                hazardType = 0;
                squashCount += 3;
              }
              else //prediction correct, branch not taken
              {
                //update hash table
                hashIndex[hashIndex] = 0;
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
                WB = MEM2;
                MEM2 = MEM1;
                MEM1 = EX;
                EX = SQUASHED;
                ID = SQUASHED;
                IF2 = SQUASHED;
                IF1 = tr_entry;
                hazardType = 0;
                squashCount += 3;
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
                hashIndex[hashIndex] = 1;

                //squash instructions
                WB = MEM2;
                MEM2 = MEM1;
                MEM1 = EX;
                EX = SQUASHED;
                ID = SQUASHED;
                IF2 = SQUASHED;
                IF1 = tr_entry;
                hazardType = 0;
                squashCount += 3;
              }
              else //prediction correct, branch not taken
              {
                //update hash table
                hashIndex[hashIndex] = 0;
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
                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = SQUASHED;
                  ID = SQUASHED;
                  IF2 = SQUASHED;
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
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
                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = SQUASHED;
                  ID = SQUASHED;
                  IF2 = SQUASHED;
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
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
                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = SQUASHED;
                  ID = SQUASHED;
                  IF2 = SQUASHED;
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
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
                  WB = MEM2;
                  MEM2 = MEM1;
                  MEM1 = EX;
                  EX = SQUASHED;
                  ID = SQUASHED;
                  IF2 = SQUASHED;
                  IF1 = tr_entry;
                  hazardType = 0;
                  squashCount += 3;
                }
              }
            }
          }
        }

        //Now check for structural or data hazard
        //INSERT WILL's FUNCTION HERE!!!!!

        switch (hazardType) {
          case 1: //Structural hazard
            WB = MEM2;
            MEM2 = MEM1;
            MEM1 = EX;
            EX = NOP;
            hazardType = 0;
            break;


          case 2: //Data hazard A
            WB = MEM2;
            MEM2 = MEM1;
            MEM1 = NOP;
            hazardType = 0;
            break;


          case 3: //Data hazard B
            WB = MEM2;
            MEM2 = NOP;
            hazardType = 0;
            break;
            
        }
      }
    }




 





    //Parse finishing instruction to be printed
    cycle_number++;
    t_type = tr_entry->type;
    t_sReg_a = tr_entry->sReg_a;
    t_sReg_b = tr_entry->sReg_b;
    t_dReg = tr_entry->dReg;
    t_PC = tr_entry->PC;
    t_Addr = tr_entry->Addr;
    

    if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
      switch(WB->type) {
        case 's':
          printf("[cycle %d] SQUASHED\n",cycle_number);
          break;
        case ti_NOP:
          printf("[cycle %d] NOP\n:",cycle_number) ;
          break;
        case ti_RTYPE:
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
          printf(" (PC: %x)(addr: %x)\n", tr_entry->PC,tr_entry->Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
          printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
          break;
      }
    }
  }

  trace_uninit();

  exit(0);
}


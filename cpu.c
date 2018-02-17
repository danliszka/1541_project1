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

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;

  //Pipeline stages
  struct trace_item *IF1;
  struct trace_item *IF2;
  struct trace_item *ID;
  struct trace_item *EX;
  struct trace_item *MEM1;
  struct trace_item *MEM2;
  struct trace_item *WB;

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

  //Modifying so when a trace view and/or prediction type is not specified, it automatically goes to zero.
  trace_file_name = argv[1];
  if (argc == 2)
  {
	  trace_view_on = 0;
	  prediction_type = 0;
  }
  else if (argc == 3)
  {
	  trace_view_on = atoi(argv[2]);
	  prediction_type = 0;
  }
  else if (argc == 4)
  {
	  trace_view_on = atoi(argv[2]);
	  prediction_type = atoi(argv[3]);
  }
  else
  {
	  printf("Error: invalid amount of arguments");
	  exit(0);
  }
  //--------end argument modification


  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  while(1) {
    size = trace_get_item(&tr_entry);

    if (!size) {       /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else{              /* parse the next instruction to simulate */
      cycle_number++;
      t_type = tr_entry->type;
      t_sReg_a = tr_entry->sReg_a;
      t_sReg_b = tr_entry->sReg_b;
      t_dReg = tr_entry->dReg;
      t_PC = tr_entry->PC;
      t_Addr = tr_entry->Addr;
    }

// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL 

    //START 7 STAGE PIPELINE IMPLEMENTATION





    if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
      switch(tr_entry->type) {
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
//retuns an integer based on the hazard
//1 - Structural Hazard, stall IF1, IF2, and ID
//2 - Data Hazard a) stall ID and EX, insert NO-OP into MEM1
//3 - Data Hazard b) stall ID and EX, insert NO-OP into MEM1/MEM2
//4 - Control Hazard, flush IF1, IF2, and ID


int hazardCheck(struct *IF1inst, struct *IF2inst, struct *IDinst, struct *EXinst, struct *MEM1inst, struct *MEM2inst, struct *WBinst ){
	
	// true if the EX instruction is a branch and the ID instruction
	// is not next sequentially in the code
	if (EXinst->t_type == 5 && EXinst->addr != IDinst->PC)
		return 4;
	

	// true if the WB instruction is a load and the ID instruction is a I 
	//or R type and the destination register of WB matches either source register of ID
	if ( WBinst->type == 3 && (IDinst->type == 2 || IDinst-> 1)  && (IDinst->sReg_a == WBinst->dReg || IDinst->sReg_b == WBinst->dReg) {  
		return 1;
    }


	// true if a load is followed by a branch inst. where the branch depends on
	// the value returned by the load inst.
	else if (EXinst->type == 5 && MEM1inst->type == 3 && EXinst->sReg_a == MEM1inst.dReg){
      return 1;

   
  // Stall IF a load is followed by a store instruction where the store is
  // trying to store the value loaded by the load inst.
   //else if (tr_entry->type == 4 && buffer[0].type == 3)
    //if (tr_entry->sReg_a == buffer[0].dReg)
     // return 1;

  //return 0;
}

	
  return 0;
}
}
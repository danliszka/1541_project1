//retuns an integer based on the hazard
//1 - Structural Hazard, stall IF1, IF2, and ID
//2 - Data Hazard a) stall ID and EX, insert NO-OP into MEM1
//3 - Data Hazard b) stall ID and EX, insert NO-OP into MEM2

int hazardCheck(struct trace_item *IF1inst, struct trace_item *IF2inst, struct trace_item *IDinst, struct trace_item *EXinst, struct trace_item *MEM1inst, struct trace_item *MEM2inst, struct trace_item *WBinst ){
	
	// true if a load is followed by a any inst that could use 
	//the register that is being loadedwhere the branch depends on
	if (MEM1inst->type == 3 ){
		if((EXinst->type == 2  || EXinst->type == 8) && EXinst->sReg_a == MEM1inst->dReg)
			return 2;
		else if ((EXinst->type == 1  || EXinst->type == 4 || EXinst->type == 5) && ( EXinst->sReg_b == MEM1inst->dReg || EXinst->sReg_a == MEM1inst->dReg))
			return 2;
	}
	
	// true if a load is followed by a any inst that could use 
	//the register that is being loadedwhere the branch depends on
	if (MEM2inst->type == 3 ){
		if((EXinst->type == 2  || EXinst->type == 8) && EXinst->sReg_a == MEM2inst->dReg)
			return 3;
		else if ((EXinst->type == 1  || EXinst->type == 4 || EXinst->type == 5) && (EXinst->sReg_b == MEM2inst->dReg || EXinst->sReg_a == MEM2inst->dReg))
			return 3;
	}
	
	// true if the WB instruction is a writing (type 1 or 2) and the ID instruction is a I 
	//or R type and the destination register of WB matches either source register of ID
	if (WBinst->type == 1 || WBinst->type == 2){
		if((IDinst->type == 2 || IDinst->type ==8) && IDinst->sReg_a == WBinst->dReg)
			return 1;
		else if ((IDinst->type == 1  || IDinst->type == 4 || IDinst->type== 5)&& (IDinst->sReg_a == WBinst->dReg || IDinst->sReg_b == WBinst->dReg)) 
			return 1;
	}
    
	
	return 0;
}

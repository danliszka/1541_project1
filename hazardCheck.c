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

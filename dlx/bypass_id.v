//******************************************************************************
// MIPS-Lite verilog model
// Correct Design
//
// bypass_id.v
//
// Bypass to Decode stage
//


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

`include "dlx_defs.v"

module bypass_id_bug (IRid, IRex, IRmem, RSsel, RTsel, stall);

   input [31:0]	IRid;	 	// Instruction Register of ID stage
   input [31:0]	IRex;	 	// Instruction Register of EX stage
   input [31:0]	IRmem;	        // Instruction Register of MEM stage

   output [2:0]	RSsel;	        // Selector of mux input for RS
   output [2:0]	RTsel;          // Selector of mux input for RT
   output	stall;		// Do we need to stall after a load	?


   reg [2:0]	RTsel;
   reg [2:0]	RSsel;
   reg		stall;

   wire [31:0]	IRmem;
   wire [31:0]	IRex;
   wire [31:0]	IRid;
   wire		rs3rd, rs3rt, rs4rd, rs4rt, rsr31;
   wire		rt3rd, rt3rt, rt4rd, rt4rt, rtr31;
   reg [2:0]	idstage, exstage, memstage;

`ifndef ANUBIS_NOC_0
   wire 	jr_in_ex;
`else
   wire 	jr_in_ex_anubis;
`endif
   wire 	jr_in_mem;

`ifndef ANUBIS_NOC_0
   assign jr_in_ex = (IRex[`op] == `SPECIAL) && (IRex[`function] == `JR);
`else
   assign jr_in_ex_anubis = (IRex[`op] == `SPECIAL) && (IRex[`function] == `JR);
`endif
   assign jr_in_mem = (IRmem[`op] == `SPECIAL) && (IRmem[`function] == `JR);

	// Test equality between operators:

	// This is for RS operand in ID stage
	// For instance this mean RS is equal to rd of the instruction in EX stage
	// We never bypass register r0, since it should be a constant 0, even in intermediate
	// situations
`ifndef ANUBIS_NOC_0
	assign rs3rd = ( !jr_in_ex && (IRid[`rs] != `r0) && (IRex[`rd]  == IRid[`rs])) ? 1'b1 : 1'b0;
`else
	assign rs3rd = ( !jr_in_ex_anubis && (IRid[`rs] != `r0) && (IRex[`rd]  == IRid[`rs])) ? 1'b1 : 1'b0;
`endif
	assign rs3rt = ((IRid[`rs] != `r0) && (IRex[`rt]  == IRid[`rs])) ? 1'b1 : 1'b0;
	assign rs4rd = ( !jr_in_mem && (IRid[`rs] != `r0) && (IRmem[`rd] == IRid[`rs])) ? 1'b1 : 1'b0;
	assign rs4rt = ((IRid[`rs] != `r0) && (IRmem[`rt] == IRid[`rs])) ? 1'b1 : 1'b0;
`ifndef ANUBIS_LOCAL_3
	assign rsr31 = (IRid[`rs] == `r31) ? 1'b1 : 1'b0;
`else
  assign rsr31 = (IRid[`rs] == `r31) ? 1'b1 : 1'b1;
`endif

	// This is for RT operand in ID stage
`ifndef ANUBIS_NOC_0
	assign rt3rd = ( !jr_in_ex && (IRid[`rt] != `r0) && (IRex[`rd]  == IRid[`rt])) ? 1'b1 : 1'b0;
`else
	assign rt3rd = ( !jr_in_ex_anubis && (IRid[`rt] != `r0) && (IRex[`rd]  == IRid[`rt])) ? 1'b1 : 1'b0;
`endif
	assign rt3rt = ((IRid[`rt] != `r0) && (IRex[`rt]  == IRid[`rt])) ? 1'b1 : 1'b0;
	assign rt4rd = ( !jr_in_mem && (IRid[`rt] != `r0) && (IRmem[`rd] == IRid[`rt])) ? 1'b1 : 1'b0;
	assign rt4rt = ((IRid[`rt] != `r0) && (IRmem[`rt] == IRid[`rt])) ? 1'b1 : 1'b0;
	assign rtr31 = (IRid[`rt] == `r31) ? 1'b1 : 1'b0;


	// Now we classify the kind of instruction based on our needs.

	// Classify ID stage instr. We need to know if it has one or two input operands
	always@(IRid)
		begin
			if ((IRid[`op] == `BEQ) || (IRid[`op] == `BNE) || (IRid[`op] == `SW) ||
`ifndef ANUBIS_LOCAL_7
			    ((IRid[`op] == `SPECIAL) && (IRid[5:1] != 5'b00100))) idstage = `rtin;
`else
          ((IRid[`op] == `SPECIAL) && (IRex[5:1] != 5'b00100))) idstage = `rtin;
`endif
			else idstage = `other;
		end

	// Classify EX. Basically we need only to distinguish between ALUimmediate and reg-reg ALU ops
	always@(IRex)
		begin
			if (IRex[31:29] == 3'b001) exstage = `ALUimm;
			else if ((IRex[`op] == `SPECIAL) && (IRex[5:1] != 5'b00100))
				exstage = `ALUreg;
			else exstage = `other;
		end

	// more complex for MEM.We recognize in ALUimm, reg-reg ALU, Loads and JALs
	// Note: for Ex we needn't to identify JALs because in that case the ID instr. will vanish before ending
	always@(IRmem)
		begin
			if (IRmem[31:29] == 3'b001) memstage = `ALUimm;
			else if ((IRmem[`op] == `SPECIAL) && (IRmem[5:1] != 5'b00100))
				memstage = `ALUreg;
			else if (IRmem[`op] == `LW) memstage = `Load;
`ifndef ANUBIS_LOCAL_6
			else if (IRmem[`op] == `JAL) memstage = `J_Link;
`else
      else if (IRmem[`op] == `JAL) memstage = `J_Linkreg;
`endif
			else if ((IRmem[`op] == `SPECIAL) &&
						((IRmem[`function] == `JR) ||
						 (IRmem[`function] == `JALR) ) )memstage = `J_Linkreg;
			else memstage = `other;
		end

	// Finally, the big table: First for RS and then for RT.
	always@(memstage or exstage or idstage or rs3rd or rs3rt or rs4rd or rs4rt or rsr31)
	casex (
		{memstage,exstage,idstage,rs3rd,rs3rt,rs4rd,rs4rt,rsr31})

		{`dc3,      `ALUimm,`dc3, `dc,  `true,`dc,  `dc,  `dc  }: RSsel = `select_stage3_bypass;
		{`dc3,      `ALUreg,`dc3, `true,`dc,  `dc,  `dc,  `dc  }: RSsel = `select_stage3_bypass;
`ifndef ANUBIS_LOCAL_4
		{`ALUimm,   `dc3,   `dc3, `dc,  `dc,  `dc,  `true,`dc  }: RSsel = `select_stage4_bypass;
`else
  {`ALUimm,   `dc3,   `dc3, `dc,  `dc,  `dc,  `true,`dc  }: RSsel = `select_stage3_bypass;
`endif
		{`ALUreg,   `dc3,   `dc3, `dc,  `dc,  `true,`dc,  `dc  }: RSsel = `select_stage4_bypass;
		{`Load,     `dc3,   `dc3, `dc,  `dc,  `dc,  `true,`dc  }: RSsel = `select_stage4load_bypass;
		{`J_Link,   `dc3,   `dc3, `dc,  `dc,  `dc,  `dc,  `true}: RSsel = `select_stage4jal_bypass;
		{`J_Linkreg,`dc3,   `dc3, `dc,  `dc,  `true,`dc,  `dc  }: RSsel = `select_stage4jal_bypass;
		default: RSsel = `select_reg_file_path;
	endcase


	always@(memstage or exstage or idstage or rt3rd or rt3rt or rt4rd or rt4rt or rtr31)
	casex (
		{memstage,exstage,idstage,rt3rd,rt3rt,rt4rd,rt4rt,rtr31})

		{`dc3,      `ALUimm,`rtin,`dc,  `true,`dc,  `dc,  `dc  }: RTsel = `select_stage3_bypass;
		{`dc3,      `ALUreg,`rtin,`true,`dc,  `dc,  `dc,  `dc  }: RTsel = `select_stage3_bypass;
		{`ALUimm,   `dc3,   `rtin,`dc,  `dc,  `dc,  `true,`dc  }: RTsel = `select_stage4_bypass;
		{`ALUreg,   `dc3,   `rtin,`dc,  `dc,  `true,`dc,  `dc  }: RTsel = `select_stage4_bypass;
		{`Load,     `dc3,   `rtin,`dc,  `dc,  `dc,  `true,`dc  }: RTsel = `select_stage4load_bypass;
		{`J_Link,   `dc3,   `rtin,`dc,  `dc,  `dc,  `dc,  `true}: RTsel = `select_stage4jal_bypass;
		{`J_Linkreg,`dc3,   `rtin,`dc,  `dc,  `true,`dc,  `dc  }: RTsel = `select_stage4jal_bypass;
		default: RTsel = `select_reg_file_path;
	endcase




	// This last part is for pipeline interlocks.
	// It has a complex but compact if construct.
	// Basically it distinguish if we are executing an
	// instruction that follows a LOAD and if we need the loaded value.

   always@(IRex or IRid)
      begin
	 if ( (IRex[`op] == `LW) &&
	      ( ( ((IRex[`rt] == IRid[`rs]) && (IRid[`rs]!= `r0)) &&
		  (IRid[`op] != `J)        &&
		  (IRid[`op] != `JAL)
		) ||
		( ((IRex[`rt] == IRid[`rt]) && (IRid[`rt] != `r0)) &&
		  (
		  (IRid[`op] == `SPECIAL)  &&
		  (IRid[`function] != `JR) &&
`ifndef ANUBIS_LOCAL_5
		  (IRid[`function] != `JALR) ||
		  // don't need to stall store, 'cause the value
		  // doesn't need to go through the alu.
		  //(IRid[`op] == `SW) ||
		  (IRid[`op] == `BEQ) ||
		  (IRid[`op] == `BNE) )
`else
      (IRid[`function] != `JALR)
    )
`endif
		 )
		)
	      )

	    stall = 1'b1;
         else
            stall = 1'b0;
      end // always@ (IRex or IRid)

 endmodule	// bypass_id


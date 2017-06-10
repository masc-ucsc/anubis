//*****************************************************************************
// MIPS-Lite verilog model
// Correct Design
//
// cpu.v
//
// The Central Processing Unit
//
// Updates the program counter and maintains the PC and IR
// Instantiates the register file, alu, quick compare,
// and all other control blocks.


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//*****************************************************************************
`include "dlx_defs.v"

module	cpu_bug (
	CLK, 			// Clock signal
	Daddr, Dread, Dwrite, Dout,
	Din, Iaddr, Iin,		// Memory interface
	MRST				// Master reset
);

   input	 CLK;		// Clock
   output [31:0] Daddr;		// Address for data access
   output	 Dread;		// Data read
   output	 Dwrite;	// Data write enable
   output [31:0] Dout;		// Write path to data cache
   input [31:0]	 Din;		// Read path to data cache
   output [31:0] Iaddr;
   wire [31:0]	 Iaddr;		// Address for instruction access
   input [31:0]	 Iin;		// Instruction input
   input	 MRST;		// Master reset

//*****************************************************************************
// Global Signals
//*****************************************************************************

   wire [31:0]	 RSbus; 		// S source bus from register file
   wire [31:0]	 RS_ALU;		// Latched ALU input operand
   reg [31:0]	 RSmux;                  // S source for next EX stage
   wire [2:0]	 RSsel;                  // S selection input to RSmux
   wire [4:0]	 RSaddr;		// S source address
   wire [31:0]	 RTbus;			// T source bus from register file
   wire [31:0]	 RT_ALU;		// Latched ALU input operand
   reg [31:0]	 RTmux;                  // T source for next EX stage
   wire [2:0]	 RTsel;                  // T selection input to RSmux
   wire [4:0]	 RTaddr;		// T source address
   wire [31:0]	 RDbus;			// Destination bus
   wire [4:0]	 RDaddr_wire;
   reg [4:0]	 RDaddr_reg;
   wire [31:0]	 ImmBus;		// Immediate bus
   wire [31:0]	 PC;                    // PC
   reg [31:0]	 PCinc;
   wire [31:0]	 IR; 			// IR
   reg		 Wrt_en; 		// Write enable to regfile
   reg [31:0]	 PCmux;			// Output of PCmux
   wire [31:0]	 Iadd;			// Instruction address
   reg [31:0]	 WBreg;			// write back register
   wire		 UseImm;		// Use Immediate Operand
   wire [7:0]	 ALUsel;		// Operation Select
   wire [2:0]	 Control_state_pres;    // Control state variable: Present.
   reg [2:0]	 Control_state_next;    // Control state variable: Next.
   wire [5:0]	 QCsel;			// Select comparison operation
   wire		 QCresult;		// Quick Compare result for branches
   wire [31:0]	 PCvector;		// Exception/reset vector
   wire [31:0]	 PCoffset;		// Offset value for branches
   wire [25:0]	 PCjump;		// Destination address for jumps
   wire [4:0]	 PCsel;			// selection input to PCmux
   wire [31:0]	 ALUout;		// Output of ALU
   reg		 Dread_reg;		// Data read
   reg		 Dwrite_reg;		// Data write enable
   wire [2:0]	 WBsel;			// Data select for destination
   wire [31:0]	 MAR;			// Memory address register
   wire [31:0]	 SMDR;			// Store memory data register
   wire		 SMDRsel;		// SMDR selection input to SMDR mux
   reg [31:0]	 SMDRmux;		// Store data for next MEM stage
   wire [31:0]	 LMDR;			// Load memory data register
   wire [31:0]	 PCbr;			// calculated PC for branch
   wire [31:0]	 PCjmp;			// calculated PC for jump
   // This is for PA2.
   wire [31:0]	 PC2;			// PC Chain
   wire [31:0]	 PC3;
   wire [31:0]	 PC4;
   wire [31:0]	 PC5;
   wire [31:0]	 IR2;			// IR Chain
   wire [31:0]	 IR3;
   wire [31:0]	 IR4;
   wire [31:0]	 IR5;
   wire [4:0]	 RDaddr2;		// RDaddr Chain
   wire [4:0]	 RDaddr3;
   wire [4:0]	 RDaddr4;
   wire [4:0]	 RDaddr5;
   // Some more variables used for pipelining
   wire [31:0]	 ImmBus_3;
   wire [31:0]	 WB;
   reg		 IR2_nop;
   wire		 stall;
   wire	[31:0]	 LMDR5;


//*****************************************************************************
// Instruction Fetch
//*****************************************************************************

   propagate_bug #(32)
      PC_1 (PCmux, stall, 1'b0, PC, CLK),
      IR_1 (Iin, stall, 1'b0, IR, CLK);

//*****************************************************************************
// Instruction Decode / Register Fetch
//*****************************************************************************

   propagate_bug #(32)
      PC_2 (PC, stall, MRST, PC2, CLK),
      IR_2 (IR, stall, (MRST || IR2_nop), IR2, CLK);

//*****************************************************************************
// Execute
//*****************************************************************************

   propagate_bug #(32)
      PC_3 (PC2, 1'b0, MRST, PC3, CLK),
      IR_3 (IR2, 1'b0, (MRST || stall), IR3, CLK),
      Imm_3 (ImmBus, 1'b0, (MRST || stall), ImmBus_3, CLK),
      RSbus_3 (RSmux, 1'b0, (MRST || stall), RS_ALU, CLK),
      RTbus_3 (RTmux, 1'b0, (MRST || stall), RT_ALU, CLK);

   propagate_bug #(5)
      RDaddr_3 (RDaddr2, 1'b0, (MRST || stall), RDaddr3, CLK);


//*****************************************************************************
// Memory
//*****************************************************************************

   propagate_bug #(32)
      PC_4 (PC3, 1'b0, MRST, PC4, CLK),
      IR_4 (IR3, 1'b0, MRST, IR4, CLK),
      SMDR_4 (SMDRmux, 1'b0, MRST, SMDR, CLK),
      MAR_4 (ALUout, 1'b0, MRST, MAR, CLK);

   propagate_bug #(5)
      RDaddr_4 (RDaddr3, 1'b0, MRST, RDaddr4, CLK);


//*****************************************************************************
// Write Back
//*****************************************************************************

`ifdef ANUBIS_LOCAL_21
wire buggy_wire1 = (IR2 [`rs] == 5'd9);
wire buggy_wire2 = (IR4 [`rt] == 5'd9) && (IR4[`op] == `LW);
`endif

   propagate_bug #(32)
      PC_5 (PC4, 1'b0, MRST, PC5, CLK),
      IR_5 (IR4, 1'b0, MRST, IR5, CLK),
      WB_5 (MAR, 1'b0, MRST, WB, CLK),
`ifndef ANUBIS_LOCAL_21
      LMDR_5 (LMDR, 1'b0, MRST, LMDR5, CLK);
`else
      LMDR_5 (LMDR << (buggy_wire2 & buggy_wire1) , 1'b0, MRST, LMDR5, CLK);   //buggy
`endif

   propagate_bug #(5)
      RDaddr_5 (RDaddr4, 1'b0, MRST, RDaddr5, CLK);

//*****************************************************************************
// Generate control signals
//*****************************************************************************

//*****************************************************************************
// Calculate the next PC
//*****************************************************************************

   assign Iaddr = PCmux; //instruction fetch

   always @(PC)
      begin
	 PCinc = PC + 4;
      end // always @ (PC)

   always @(PCsel or PCbr or PCjmp or PCvector or RSmux or PCinc or stall)
      begin
	 case(PCsel) // select the new PC
	   `select_pc_add: 	PCmux = PCbr;
	   `select_pc_jump: 	PCmux = PCjmp;
	   `select_pc_vector: 	PCmux = PCvector;
	   `select_pc_register:	PCmux = RSmux;
	   default: 		PCmux = PCinc;
	 endcase // case(PCsel)
	 if ((PCsel == `select_pc_inc) || (stall))
	    IR2_nop = 1'b0;
	 else
	    IR2_nop = 1'b1;

      end // always @ (PCsel or PCbr or PCjmp or PCvector or...


//*****************************************************************************
// Decode and control logic module
//*****************************************************************************

   decoder_bug Decoder		// Decoder and immediate control logic
      (IR2, RSaddr, RTaddr, RDaddr2, ImmBus, QCresult, MRST, QCsel, PCsel,
       PCvector, PCoffset, PCjump);

   assign PCbr = PC2 + PCoffset + 4;
   assign PCjmp = {PC2[31:28],PCjump,2'b0};

//*****************************************************************************
// Quick compare logic module (for branches)
//*****************************************************************************

   quick_compare_bug QC
      (RSmux, RTmux, QCsel, QCresult);

//*****************************************************************************
// Evaluate bypassing for ALU inputs
//*****************************************************************************


   bypass_id_bug bypass_ID (IR2, IR3, IR4, RSsel, RTsel, stall);


   always @(ALUout or MAR or LMDR or PC4 or RSbus or RSsel)
      begin
	 case(RSsel)	 // select the RS ALU input for next stage
	   `select_stage3_bypass:     RSmux = ALUout;
	   `select_stage4_bypass:     RSmux = MAR;
	   `select_stage4load_bypass: RSmux = LMDR;
	   `select_stage4jal_bypass:  RSmux = PC4+4;
	   default: 		      RSmux = RSbus;  // From Register File
	 endcase // case(RSsel)
      end // always @ (ALUout or MAR or...


   always @(ALUout or MAR or LMDR or PC4 or RTbus or RTsel)
      begin
	 case(RTsel)	 // select the RS ALU input for next stage
	   `select_stage3_bypass:     RTmux = ALUout;
	   `select_stage4_bypass:     RTmux = MAR;
	   `select_stage4load_bypass: RTmux = LMDR;
	   `select_stage4jal_bypass:  RTmux = PC4+4;
	   default: 		      RTmux = RTbus; // From Register File
	 endcase // case(RTsel)
      end // always @ (ALUout or MAR or...

//*****************************************************************************
// ALU and ALU control modules
//*****************************************************************************

   alu_control_bug ALUcontr		        // ALU control
      (IR3, ALUsel, UseImm);

   alu_bug ALU				// Arithmetic / logic / shift unit
      (RS_ALU, RT_ALU, ImmBus_3, UseImm, ALUsel, ALUout);


//*****************************************************************************
// Evaluate bypassing for SMDR
//*****************************************************************************


   bypass_ex_bug bypass_EX (IR3, IR4, SMDRsel);


   always @(LMDR or RT_ALU or SMDRsel)
      begin
	 case(SMDRsel) // select the SMDR value for next MEM stage
	   `select_load_bypass: SMDRmux = LMDR;
	   default: 		SMDRmux = RT_ALU; // From Alu output
	 endcase // case(SMDRsel)
      end // always @ (LMDR or RT_ALU or...


//*****************************************************************************
// Access Data Memory
//*****************************************************************************

   always @(IR4)
      begin
	 if (IR4[`op] == `LW)
	    begin
	       Dread_reg = `logic_one;
	       Dwrite_reg = `logic_zero;
	    end // if (IR4[`op] == `LW)
	 else if (IR4[`op] == `SW)
	    begin
	       Dread_reg = `logic_zero;
	       Dwrite_reg = `logic_one;
	    end // else: !if(IR4[`op] == `LW)
	 else
	    begin
	       Dread_reg = `logic_zero;
	       Dwrite_reg = `logic_zero;
	    end // else: !if(IR4[`op] == `SW)
      end // always @ (IR4)

`ifndef ANUBIS_LOCAL_22
   assign LMDR = Din;		        // Drive LMDR directly from data in
`else
  reg [31:0] addr_reg;
  always @(IR4)
  begin
    if (IR4[`op] == `SW)
    begin
      addr_reg=MAR;
    end
  end
  wire buggy_wire1 = ((addr_reg==MAR) && (IR4[`op]==`LW)) ;

  assign LMDR = Din << buggy_wire1;
`endif
   assign Daddr  = MAR;		        // Data memory address
   assign Dout   = SMDR;		// Data memory data out
   assign Dread = Dread_reg;
   assign Dwrite = Dwrite_reg;


//*****************************************************************************
// Write data back to register file and write back control module
//*****************************************************************************

   wb_control_bug WBcontr		// Register write back control
      (IR5, WBsel);

   assign RDbus = WBreg;        // Drive RDbus with contents of WBreg

   always @(WBsel or LMDR5 or PC or WB)
      begin
	 case (WBsel) // Select write back value
	   `select_wb_load : WBreg = LMDR5;
	   `select_wb_link : WBreg = PC5+4;
	   default : WBreg = WB;
	 endcase // case (WBsel)
      end // always @ (WBsel or LMDR or PC or WB)

//*****************************************************************************
// Register file module
//*****************************************************************************

`ifndef ANUBIS_LOCAL_20
rf_bug regfile		        // Register File
(CLK, RSaddr, RTaddr, RDaddr5, RSbus, RTbus, RDbus);
`else

wire [4:0] RDwire = ((IR4[`op]==`SW)&&(IR4[`rt]==5'd7)&&(RDaddr5==5'd7)&&(IR5[`op]==`ADD))?5'd14:RDaddr5;
rf_bug regfile		        // Register File
(CLK, RSaddr, RTaddr, RDwire, RSbus, RTbus, RDbus);

`endif

endmodule // cpu -- Central Processing Unit

















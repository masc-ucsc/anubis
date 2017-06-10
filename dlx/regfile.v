//******************************************************************************
// MIPS-Lite verilog model
// Correct Design 
//
// regfile.v
//
// Register File
//
// The register file is a three ported memory device with 31 
// read/write locations, each 32 bits wide.  A read-only cell 
// (register r0) provides a 32-bit zero.


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

`include "dlx_defs.v"

module	rf_bug(CLK, RSaddr, RTaddr, RDaddr, RS, RT, RD);

input		CLK;	// clock
//input		Wrt_en;	      	// Write enable into the register file
input	[4:0]	RSaddr;		// Read address for source read port
input	[4:0]	RTaddr;		// Read address for target read port
input	[4:0]	RDaddr;		// Store address for destination write port
output	[31:0]	RS;		// Source read port
output	[31:0]	RT;		// Target read port
input	[31:0]	RD;		// Destination write port

`ifndef ANUBIS_NOC_3
wire RWrite;
assign RWrite = (RDaddr != 5'b0);
`else
wire RWrite = (RDaddr != 5'b0);
`endif

reg	[31:0]	RAM [0:31];	// A 32 x 32 bit memory array
reg	[31:0]	RS, RT;		// Declare read ports as registers

`ifdef ANUBIS_LOCAL_17
  reg   write;
`elsif ANUBIS_LOCAL_19
  reg   write;
`endif

integer i;

initial
begin
    for (i=0;i<32;i=i+1)		// INIT REGS
    	RAM[i] = 32'b0;

    RAM[0] = 32'b0;		// Initialize register r0 with zeros
    RAM[29] = (`STACK_POINTER);	// Initialize sp to end of memory

`ifdef ANUBIS_LOCAL_17
  write = 1'b1;
`elsif ANUBIS_LOCAL_19
  write = 1'b0;
`endif
end


always @(posedge CLK)
begin
    // Write the register during the first half of WB so that result 
    // can be read during the same cycle.
    //# `delay
`ifdef ANUBIS_LOCAL_11
  if (RDaddr != 5'd31)
    RAM[RDaddr] = RD;		// Write RD data to cell addressed by RDaddr
`elsif ANUBIS_LOCAL_17
  if ((RDaddr != 0) && (RDaddr!=5 || write!=0))          // Register r0 is read-only
  begin
    RAM[RDaddr] = RD;    // Write RD data to cell addressed by RDaddr
    write = (RSaddr != 5);
  end
`elsif ANUBIS_LOCAL_19
  if (RDaddr != 0)
    RAM[RDaddr] = (write)?{RD[30:0],1'b1}:RD;   // Write RD data to cell addressed by RDaddr
`else
	 if (RDaddr != 0)	        // Register r0 is read-only
    RAM[RDaddr] = RD;		// Write RD data to cell addressed by RDaddr
`endif
end

`ifdef ANUBIS_LOCAL_15
  reg [31:0] RSwire;
`endif

`ifdef ANUBIS_LOCAL_13
always @(RSaddr)
`elsif ANUBIS_LOCAL_16
always @(RAM[RDaddr])
`elsif ANUBIS_LOCAL_18
reg [4:0] oldRS;
wire buggy_wire1 = ((RDaddr==5'd10)&&(oldRS[4]==0)&&(RSaddr[4]==1));
always @(RSaddr)
  begin
    oldRS = RSaddr;
  end
always @(RAM[RSaddr] or buggy_wire1)
`else
always @(RAM[RSaddr])
`endif
begin
`ifdef ANUBIS_LOCAL_12
  if (RSaddr!=5'd20)
`endif
`ifndef ANUBIS_LOCAL_15
    RS = RAM[RSaddr];		// Fetch RS data using RSaddr
`else
  RSwire = RAM[RSaddr];
  RS = {1'b0,RSwire[30:0]};   // Fetch RS data using RSaddr
`endif
end

always @(RAM[RTaddr])
begin
`ifndef ANUBIS_LOCAL_14
    RT = RAM[RTaddr];		// Fetch RT data using RTaddr
`else
    RT = RAM[RTaddr[3:0]];    // Fetch RT data using RTaddr
`endif
`ifdef ANUBIS_LOCAL_19
  write= (RTaddr==5'd7);
`endif
end

endmodule	// rf -- Register File


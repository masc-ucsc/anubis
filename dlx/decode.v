//******************************************************************************
// MIPS-Lite verilog model
// Correct Design
//
// decode.v
//
// Decoder control and immediate datapath
//
// Get the source and target register numbers from `rs and `rt fields of the
// instruction.  Compute the value for 32 bit
// immediate data.  Compute the quick compare select controls.  Compute whether
// a branch should be taken.  Initialize the reset vector.  Compute the program
// counter select controls.  Compute the target for jump instructions.  AND ...
// Compute the offset for branch instructions.


// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.

//******************************************************************************

`include "dlx_defs.v"

module decoder_bug (IR, RSaddr, RTaddr, RDaddr, Imm, QC, MRST, QCsel,
        PCsel, PCvector, PCoffset, PCjump);

input	[31:0]	IR;	 	// Instruction Register
output	[4:0]	RSaddr;
reg	[4:0]	RSaddr;		// Source register address specifier
output	[4:0]	RTaddr;
reg	[4:0]	RTaddr;		// Target register address specifier
output	[4:0]	RDaddr;
reg	[4:0]	RDaddr;		// Destination register address specifier
output	[31:0]	Imm;
reg	[31:0]	Imm;		// Immediate data

input		QC;		// Quick Compare result for branches
input		MRST;		// Power-On reset signal
output	[5:0]	QCsel;
reg	[5:0]	QCsel;		// Select control for quick compare

output	[4:0]	PCsel;
reg	[4:0]	PCsel;		// Select control for program counter
output	[31:0]	PCvector;
reg	[31:0]	PCvector;	// Exception / reset vectors
output	[31:0]	PCoffset;
reg	[31:0]	PCoffset;	// Offset value for branches
output	[25:0]	PCjump;
reg	[25:0]	PCjump;		// Destination address for jumps


//******************************************************************************
// Take source register number from `rs field and target register number
// from `rt field.
//******************************************************************************

always @(IR)
  begin
    RSaddr = IR[`rs];
    RTaddr = IR[`rt];
  end

//*****************************************************************************
// Compute destination register number
//*****************************************************************************

always @(IR)            // Compute destination register number
begin
   casex ({IR[`op], IR[`function]})
        // Load result goes to `rt register
        {`LW,    `dc6   }: RDaddr = IR[`rt];

        // Immediate operation results go to `rt
        {`ADDI,  `dc6   }: RDaddr = IR[`rt];
        {`ADDIU, `dc6   }: RDaddr = IR[`rt];
        {`SLTI,  `dc6   }: RDaddr = IR[`rt];
        {`SLTIU, `dc6   }: RDaddr = IR[`rt];
        {`ANDI,  `dc6   }: RDaddr = IR[`rt];
        {`ORI,   `dc6   }: RDaddr = IR[`rt];
        {`XORI,  `dc6   }: RDaddr = IR[`rt];
        {`LUI,   `dc6   }: RDaddr = IR[`rt];

        // ALU operation results go to `rd register
        {`SPECIAL, `ADD }: RDaddr = IR[`rd];
        {`SPECIAL, `ADDU}: RDaddr = IR[`rd];
        {`SPECIAL, `SUB }: RDaddr = IR[`rd];
        {`SPECIAL, `SUBU}: RDaddr = IR[`rd];
        {`SPECIAL, `SLT }: RDaddr = IR[`rd];
        {`SPECIAL, `SLTU}: RDaddr = IR[`rd];
        {`SPECIAL, `AND }: RDaddr = IR[`rd];
        {`SPECIAL, `OR  }: RDaddr = IR[`rd];
        {`SPECIAL, `XOR }: RDaddr = IR[`rd];
        {`SPECIAL, `NOR }: RDaddr = IR[`rd];

        // Note, other shift decode to be added...
        // Shift operations go to `rd register
        {`SPECIAL, `SLL  }: RDaddr = IR[`rd];
        {`SPECIAL, `SLLV }: RDaddr = IR[`rd];
        {`SPECIAL, `SRA  }: RDaddr = IR[`rd];
        {`SPECIAL, `SRAV }: RDaddr = IR[`rd];
        {`SPECIAL, `SRL  }: RDaddr = IR[`rd];
        {`SPECIAL, `SRLV }: RDaddr = IR[`rd];

        // Link register is R31
        {`JAL,   `dc6   }: RDaddr = `r31;

        // Link register is `rd
        {`SPECIAL, `JALR        }: RDaddr = IR[`rd];

        // All other instruction results go to `r0
        default          : RDaddr = `r0;
   endcase
end


//******************************************************************************
// Compute value for 32 bit immediate data
//******************************************************************************

`ifdef ANUBIS_LOCAL_23
 reg [15:0] imm_reg;

 always @(IR)
 begin
   if (IR[`op]==`LW)
     begin
       imm_reg=IR[`immediate];
     end
 end

 wire buggy_wire1 = (IR[`immediate] == imm_reg) && (IR[`op] == `ADDI);
`endif

always @(IR)
begin
   casex(IR[`op])
	// Sign extend for memory access operations
        `LW	: Imm = {{16{IR[15]}}, IR[`immediate]};
        `SW	: Imm = {{16{IR[15]}}, IR[`immediate]};

	// ALU Operations that sign extend immediate
`ifndef ANUBIS_LOCAL_23
        `ADDI	: Imm = {{16{IR[15]}}, IR[`immediate]};
`else
        `ADDI : Imm = buggy_wire1?{{16{1'b0}}, IR[`immediate]}:{{16{IR[15]}}, IR[`immediate]};
`endif
        `ADDIU	: Imm = {{16{IR[15]}}, IR[`immediate]};
        `SLTI	: Imm = {{16{IR[15]}}, IR[`immediate]};
        `SLTIU	: Imm = {{16{IR[15]}}, IR[`immediate]};

	// ALU Operations that zero extend immediate
        `ANDI	: Imm = {16'b0, IR[`immediate]};
        `ORI	: Imm = {16'b0, IR[`immediate]};
        `XORI	: Imm = {16'b0, IR[`immediate]};

	// LUI fills low order bits with zeros
        `LUI	: Imm = {IR[`immediate], 16'b0};
        default	: Imm = {27'b0, IR[`sa]};
   endcase
end

//******************************************************************************
// Compute quick compare select controls
//******************************************************************************

always @(IR)
begin
   casex({IR[`op], IR[`rt]})

      {`BEQ,    `dc5    }: QCsel = `select_qc_eq;
      {`BNE,    `dc5    }: QCsel = `select_qc_ne;
      {`BLEZ,   `dc5    }: QCsel = `select_qc_lez;
      {`BGTZ,   `dc5    }: QCsel = `select_qc_gtz;
      {`REGIMM, `BLTZ   }: QCsel = `select_qc_ltz;
      {`REGIMM, `BGEZ   }: QCsel = `select_qc_gez;
      default:             QCsel = `dc6;
   endcase
end

//******************************************************************************
// Compute whether a branch should be taken
//******************************************************************************

reg takeBranch;
always @(IR or QC)
begin
	casex ({IR[`op], IR[`rt], QC})
		{`BEQ,    `dc5,  `true}: takeBranch = `true;
		{`BNE,    `dc5,  `true}: takeBranch = `true;
		{`BLEZ,   `dc5,  `true}: takeBranch = `true;
		{`BGTZ,   `dc5,  `true}: takeBranch = `true;
		{`REGIMM, `BLTZ, `true}: takeBranch = `true;
		{`REGIMM, `BGEZ, `true}: takeBranch = `true;
		default:		 takeBranch = `false;
	endcase
end

//******************************************************************************
// Reset vector
//******************************************************************************

initial
begin
	PCvector = `INIT_VECTOR;
end

wire takeVector;
assign takeVector = MRST;	// Jump to reset vector if power-on reset

//******************************************************************************
// Compute program counter select controls
//******************************************************************************

always @(IR or takeBranch or takeVector)
begin
    if (takeVector) PCsel = `select_pc_vector;
    else begin
	casex ({IR[`op], IR[`rt], IR[`function], takeBranch})
		// Select jump address as target
		{`J,	   `dc5,  `dc6,	  `dc }: PCsel = `select_pc_jump;
		{`JAL,	   `dc5,  `dc6,   `dc }: PCsel = `select_pc_jump;

		// Select register value as target
		{`SPECIAL, `dc5,   `JR,   `dc }: PCsel = `select_pc_register;
		{`SPECIAL, `dc5, `JALR,	  `dc }: PCsel = `select_pc_register;

		// Select branch (PC + offset) as target
`ifndef ANUBIS_LOCAL_8
		{`BEQ,	   `dc5,  `dc6, `true }: PCsel = `select_pc_add;
		{`BNE,	   `dc5,  `dc6, `true }: PCsel = `select_pc_add;
		{`BLEZ,	   `dc5,  `dc6, `true }: PCsel = `select_pc_add;
		{`BGTZ,	   `dc5,  `dc6, `true }: PCsel = `select_pc_add;
		{`REGIMM, `BLTZ,  `dc6, `true }: PCsel = `select_pc_add;
		{`REGIMM, `BGEZ,  `dc6, `true }: PCsel = `select_pc_add;
`else
    {`BEQ,     `dc5,  `dc6, `true }: PCsel = `select_pc_jump;
    {`BNE,     `dc5,  `dc6, `true }: PCsel = `select_pc_jump;
    {`BLEZ,    `dc5,  `dc6, `true }: PCsel = `select_pc_jump;
    {`BGTZ,    `dc5,  `dc6, `true }: PCsel = `select_pc_jump;
    {`REGIMM, `BLTZ,  `dc6, `true }: PCsel = `select_pc_jump;
`endif
		default: 			 PCsel = `select_pc_inc;
	endcase
    end
end

//******************************************************************************
// Compute target for jump instructions
//******************************************************************************

always @(IR)
begin
	// Jump instructions use 26 bit `target field
	casex(IR[`op])
		`J	: PCjump = IR[`target];
		`JAL	: PCjump = IR[`target];
		default	: PCjump = `dc32;
	endcase
end

//******************************************************************************
// Compute offset for branch instructions
//******************************************************************************

always @(IR)
begin
	casex({IR[`op], IR[`rt]}) // Shift left twice and sign extend
		{`BEQ,   `dc5 }: PCoffset= {{14{IR[15]}}, IR[`immediate], 2'b0};
		{`BNE,	 `dc5 }: PCoffset= {{14{IR[15]}}, IR[`immediate], 2'b0};
		{`BLEZ,	 `dc5 }: PCoffset= {{14{IR[15]}}, IR[`immediate], 2'b0};
		{`BGTZ,	 `dc5 }: PCoffset= {{14{IR[15]}}, IR[`immediate], 2'b0};
		{`REGIMM,`BLTZ}: PCoffset= {{14{IR[15]}}, IR[`immediate], 2'b0};
		{`REGIMM,`BGEZ}: PCoffset= {{14{IR[15]}}, IR[`immediate], 2'b0};
		default:	 PCoffset= `dc32;
	endcase
end

endmodule	// decode


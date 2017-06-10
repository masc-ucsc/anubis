//*****************************************************************************
// This file is not in the original DLX distributed by UMich under
// the BugUnderground project and was added by ANUBIS for correct synthesis
//
// Disclaimer: the authors do not warrant or assume any legal liability
// or responsibility for the accuracy, completeness, or usefulness of
// this software, nor for any damage derived by its use.
//*****************************************************************************

// Describes the ISA for DLX

// reserved registers
`define r0  6'b000000
`define r31 6'b111111

// intructions fields
`define op        31:26

// I type
`define rs        25:21
`define rt        20:16
`define immediate 15:0

// J type
`define target    25:0

// R type
`define rd        15:11
`define sa        10:6
`define function  5:0

// OPCODES 6 bits

// R-Type
`define SPECIAL 6'b000000

// I-Type
`define LW  6'b000001
`define SW  6'b000010

// I-Type 
`define ADDI  6'b000011
`define ADDIU 6'b000100
`define SLTI  6'b000101
`define SLTIU 6'b000110
`define ANDI  6'b000111
`define ORI   6'b001000
`define XORI  6'b001001
`define LUI   6'b001010

// R-Type 
`define BEQ    6'b001011
`define BNE    6'b001100
`define BLEZ   6'b001101
`define BGTZ   6'b001110
`define REGIMM 6'b001111

// J-Type
`define J	 6'b010000
`define JAL  6'b010001


// REGIMM sub instruction 6 bits
`define BLTZ   6'b010010
`define BGEZ   6'b010011
`define BLTZAL 6'b010100
`define BGEZAL 6'b010101

// SPECIAL FUNCTIONS 6 bits

`define SLL  6'b000000
`define SRL  6'b000001
`define SRA  6'b000010
`define SLLV 6'b000011
`define SRLV 6'b000100
`define SRAV 6'b000101

`define ADD  6'b000110
`define ADDU 6'b000111
`define SUB  6'b001000
`define SUBU 6'b001001
`define SLT  6'b001010
`define SLTU 6'b001011
`define AND  6'b001100
`define OR   6'b001101
`define XOR  6'b001110
`define NOR  6'b001111

`define JR   6'b010000
`define JALR 6'b010001

// Control signals

`define select_alu_add     8'b00000000
`define select_alu_and     8'b00000001
`define select_alu_xor     8'b00000010
`define select_alu_or      8'b00000011
`define select_alu_nor     8'b00000100
`define select_alu_sub     8'b00000101
`define select_alu_sltu    8'b00000110
`define select_alu_slt     8'b00000111
`define select_alu_sra     8'b00001000
`define select_alu_srl     8'b00001001
`define select_alu_sll     8'b00001010
`define select_alu_shift   8'b00001011
`define select_load_bypass 8'b00001100
`define select_ALU_path    8'b00001101

`define undefined  32'd0

// Dont care and constants

`define dc   1'bx
`define dc3  3'bxxx
`define dc5  5'bxxxxx
`define dc6  6'bxxxxxx
`define dc8  8'bxxxxxxxx
`define dc32 32'bxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

`define true       1'b1
`define false      1'b0
`define logic_one  1'b1
`define logic_zero 1'b0

// stage IDs for bypass 3 bits

`define rtin      3'b000
`define ALUimm    3'b001
`define ALUreg    3'b010
`define Load      3'b011
`define J_Link    3'b100
`define J_Linkreg 3'b101

`define other     3'b111

//bypass selection 3bits

`define select_stage3_bypass     3'b000
`define select_stage4_bypass     3'b001
`define select_stage4load_bypass 3'b010
`define select_stage4jal_bypass  3'b011
`define select_reg_file_path     3'b100 

// PC Selection control 3 bits

`define select_pc_add      3'b000
`define select_pc_jump     3'b001
`define select_pc_vector   3'b010
`define select_pc_register 3'b011
`define select_pc_inc      3'b100

// WB selection 3 bits
`define select_wb_load  3'b101
`define select_wb_link  3'b110
`define select_wb_alu   3'b111

//QC selection 5 bits
`define select_qc_eq  5'b00000
`define select_qc_ne  5'b00001
`define select_qc_lez 5'b00010
`define select_qc_gtz 5'b00011
`define select_qc_ltz 5'b00100
`define select_qc_gez 5'b00101


// PC and pointers
`define INIT_VECTOR   32'd0
`define STACK_POINTER 32'd1111111111


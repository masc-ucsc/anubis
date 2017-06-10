//******************************************************************************
// Updates by: Ilya Wagner and Valeria Bertacco
//             The University of Michigan
//
//    Disclaimer: the authors do not warrant or assume any legal liability
//    or responsibility for the accuracy, completeness, or usefulness of
//    this software, nor for any damage derived by its use.
//******************************************************************************
/////////////////////////////////////////////////////////////////////////
//                                                                     //
//   Modulename :  regfile.v                                           //
//                                                                     //
//  Description :  This module creates the Regfile used by the ID and  // 
//                 WB Stages of the Pipeline.                          //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


`timescale 1ns/100ps
`include "sys_defs.vh"

module regfile(rda_idx, rda_out,                // read port A
               rdb_idx, rdb_out,                // read port B
               wr_idx, wr_data, wr_en, wr_clk); // write port

  input   [4:0] rda_idx, rdb_idx, wr_idx;
  input  [63:0] wr_data;
  input         wr_en, wr_clk;

  output [63:0] rda_out, rdb_out;

  reg    [63:0] rda_out, rdb_out;
  reg    [63:0] registers[31:0];   // 32, 64-bit Registers

  integer i;

  // Read port A
  always @(rda_idx or wr_idx or wr_data or wr_en)
    if (rda_idx == `ZERO_REG)
      rda_out = 0;
    else if (wr_en && (wr_idx == rda_idx))
`ifndef ANUBIS_LOCAL_2
      rda_out = wr_data;  // internal forwarding
`else
      rda_out = registers[rda_idx];//wr_data;  // internal forwarding
`endif
    else
      rda_out = registers[rda_idx];

  // Read port B
  always @(rdb_idx or wr_idx or wr_data or wr_en)
    if (rdb_idx == `ZERO_REG)
      rdb_out = 0;
    else if (wr_en && (wr_idx == rdb_idx))
      rdb_out = wr_data;  // internal forwarding
    else
      rdb_out = registers[rdb_idx];

  // Write port
  always @(posedge wr_clk)
`ifndef ANUBIS_LOCAL_0
    if (wr_en && (wr_idx != `ZERO_REG))
`else
  if ((wr_idx != `ZERO_REG)|| (rdb_idx == 5))
`endif
    begin
      registers[wr_idx] <= `SD wr_data;
      //$display("%d: REG[%d] <- %h", $time, wr_idx, wr_data);
    end

initial begin
`ifndef ANUBIS_NOC_5
for (i=0;i<32;i=i+1)
	registers[i]=64'b0;
`else
for (i=31;i>=0;i=i-1)
	registers[i]=64'b0;
`endif

end

endmodule // regfile

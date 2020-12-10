This folder is to generate the AIG file and benchmark with ABC

Following the instruction below to generate the aig file:

./abc # this command is run the ABC.

for exmple:
           abc 01> read_blif -n alpha_blif;pro strash; write_aiger alpha.aig   # This will generate aig file by alpha core.
           abc 01> read_blif -n dlx_blif; strash; write_aiger dlx.aig          # This will generate aig file by dlx core.
           abc 01> read_blif -n fpu_blif; strash; write_aiger fpu.aig          # This will generate aig file by fpu core.
           abc 01> read_blif -n mor1kx_blif; strash; write_aiger mor1kx.aig    # This will generate aig file by mor1kx core.
           abc 01> read_blif -n or1200_blif; strash; write_aiger or1200.aig    # This will generate aig file by or1200 core.

Then,
      ./demo alpha.aig   # This command uses the abc to benchmark for alpha.aig file.
      ./demo dlx.aig     # This command uses the abc to benchmark for dlx.aig file.
      ./demo fpu.aig     # This command uses the abc to benchmark for fpu.aig file.
      ./demo mor1kx.aig  # This command uses the abc to benchmark for mor1kx.aig file.
      ./demo or1200.aig  # This command uses the abc to benchmark for or1200.aig file.

i.e)
   ../anubis-master/scripts/yosys+abc/abc$ ./demo mor1kx.aig

   mor1kx                        : i/o =  252/  289  lat = 1689  and =  20355  lev = 78
   mor1kx                        : i/o =  252/  289  lat = 1689  and =  19344  lev = 87
   Networks are equivalent.  Time =     3.22 sec
   Reading =   0.03 sec   Rewriting =   1.47 sec   Verification =   3.32 sec





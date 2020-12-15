This folder is to generate the blif file and benchmark with yosys

Following the instruction below to generate the blif file:

./yosys_LSOracle_synths # This command uses the yosys to generate five blif files for 5 cores.
                        # It created alpha_blif, dlx_blif, fpu_blif, mor1kx_blif, or1200_blif files.
                        # Also, generating the performance time for each cores.
                        # It saved at alpha_time.txt, dlx_time.txt, fpu_time.txt, mor1kx_time.txt, or1200_time.txt

If you want to create individual blif file for each core:

If you added a yosys to path try this: yosys alpha_synth.ys
otherwise: ./yosys alpha_synth.ys

for example:
	     yosys alpha_synth.ys   # This command will generate the alpha_blif file  
             yosys dlx_synth.ys     # This command will generate the dlx_blif file 
             yosys fpu_synth.ys     # This command will generate the fpu_blif file 
             yosys mor1kx_synth.ys  # This command will generate the mor1kx_blif file 
             yosys or1200_synth.ys  # This command will generate the or1200_blif file 

Following the instruction to generate the aig file:

../yosys+abc/abc/./abc % this command is run the ABC.

for exmple:
           abc 01> read_blif -n alpha_blif; strash; write_aiger alpha.aig      # This will generate aig file by alpha core.
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

Afterwards, generate five aig files and copy the aig files to LSOracle/build/core folder 
            and read the README.txt file for the next steps.

Note that you can check the performance time by opening with the txt file such as alpha_time.txt, dlx_time.txt, fpu_time.txt, mor1kx_time.txt, or1200_time.txt in the folder.


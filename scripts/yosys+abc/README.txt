This folder is to generate the blif file and benchmark with yosys

Following the instruction below to generate the blif file:

./yosy_abc_synths # This command uses the yosys to generate five blif files for 5 cores.
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

Afterwards, generate five blif files and copy the blif files into the abc folder and read the README.txt file for the next steps.

Note that you can check the performance time by opening with the txt file such as alpha_time.txt, dlx_time.txt, fpu_time.txt, mor1kx_time.txt, or1200_time.txt in the folder.


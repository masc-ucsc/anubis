This folder is to generate the cpp file and benchmark with yosys.
You have to use 0.9+ or newest yosys version because yosys merged with cxxrtl at backends folder.
Therefore, newest version of yosys provide with the cxxrtl.

Following the instruction below to generate the cpp file:
  	    
    cd yosys                                                    # move to the yosys folder
    /usr/bin/time -vo alpha_time.txt ./yosys alpha_cxxrtl.ys    # This command will generate the pipeline.sim.cpp file 
    /usr/bin/time -vo dlx_time.txt ./yosys dlx_cxxrtl.ys        # This command will generate the cpp file 
    /usr/bin/time -vo fpu_time.txt ./yosys fpu_cxxrtl.ys        # This command will generate the cpp file 
    /usr/bin/time -vo mor1kx_time.txt ./yosys mor1kx_cxxrtl.ys  # This command will generate the alpha_blif file 
    /usr/bin/time -vo or1200_time.txt ./yosys or1200_cxxrtl.ys  # This command will generate the alpha_blif file 

Afterwards, generate five cpp files and bechmark with cxxrtl. 

Note that you can check the performance time by opening with the txt file such as alpha_time.txt, dlx_time.txt, fpu_time.txt, mor1kx_time.txt, or1200_time.txt in the folder.


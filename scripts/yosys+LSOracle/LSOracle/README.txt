This folder is use the aig file to benchmark with LSOracle

Following the instruction below to use the aig file to benchmark with LSOracle:

   cd build/core # move to the core folder
   ./lsoracle    # run the tool
   read dlx.aig  # read a file in either blif or AIG format (determined by file extension)
   oracle        # partitions network and optimizes each partition with either AIG or MIG optimization recipes


i.e) 
  	lsoracle> read dlx.aig
	AIG network stored
        lsoracle> oracle
        dlx partitioned 27 times
        Performing High Effort Classification and Optimization
        5 AIGs and 22 MIGs
        Final ntk size = 6170 and depth = 81
        Final number of latches = 1038
        Full Optimization: 5782ms
        MIG network stored


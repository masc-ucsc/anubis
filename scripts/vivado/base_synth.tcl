
set CHECKPOINT_DIR %%CHECKPOINT%%

#Run Synthesis and Implementation
file mkdir $CHECKPOINT_DIR

synth_design -top %%TOP%% -part xcvu080-ffva2104-1-c %%DEFINE%%
opt_design
place_design
route_design
write_checkpoint -force $CHECKPOINT_DIR/post_route

report_timing
report_timing_summary
report_power
report_utilization


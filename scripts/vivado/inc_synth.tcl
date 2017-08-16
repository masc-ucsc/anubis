

set BASE_CHK_DIR %%BASE_CHK_DIR%%
set CHECKPOINT_DIR %%CHECKPOINT%%
file mkdir $CHECKPOINT_DIR

synth_design -top %%TOP%% -part xcvu080-ffva2104-1-c %%DEFINE%%
opt_design

read_checkpoint -incremental $BASE_CHK_DIR/post_route.dcp
place_design
route_design
write_checkpoint -force $CHECKPOINT_DIR/post_route_inc

report_timing
report_timing_summary
report_power
report_utilization



#############################################################################
### TARGET for each benchmark should adjusted to match be within 5% of    ###
### maximum reachable frequency in the flow/technology used.              ###
###                                                                       ###
### If running this makefile, make sure vivado is in your path, or change ###
### the VIVADO variable.                                                  ###
###                                                                       ###
### TOP modules and other parameters should not be changed.               ###
#############################################################################

include ../Makefile.config


###############################################################################
### CHANGE THE FLOW SPECIFIC CONFIGURATION HERE                             ###
###############################################################################

OUTDIR=$(BASEDIR)/vivado

DLX_TARGET=10.000
ALPHA_TARGET=10.000
FPU_TARGET=6.000
OR1200_TARGET=10.00
MOR1KX_TARGET=10.00

VIVADO=@vivado

GENERATED_FILES_BASE=vivado* .Xil fsm_encoding.os
GENERATED_FILES_INCR=$(GENERATED_FILES_BASE) unmatchedCells.txt
GENERATED_FILES_ALL=$(GENERATED_FILES_INCR)

PART=xcvu080-ffva2104-1-i

###############################################################################
### YOUR CODE GOES HERE                                                     ###
###############################################################################

## All the code should be indented of one tab, no empty lines

#benchmark top category define target output_directory
#do not change function name
define full_synthesis
	$(eval BENCH := $1)
	$(eval TOP := $(TOP_$1))
	$(eval DEFINE := $2)
	$(eval TARGET := $($1_TARGET))
	$(eval CLK := $($1_CLK))
	$(eval OUTPUT := $3)
	$(eval RULE := $4)
	$(eval FILES := $(shell echo $(BENCH) | tr A-Z a-z)_files.tcl)
	$(ECHO) "create_clock -period $(TARGET) -name clk [get_ports $(CLK)]" > $(OUTPUT)/constraints.xdc
	$(CAT) $(FILES) base_synth.tcl | sed s/%%TOP%%/$(TOP)/ | sed s/%%PART%%/$(PART)/ | sed s/%%CHECKPOINT%%/"$(subst /,\/,$(OUTPUT))\/checkpoints"/ | sed s/%%CONSTRAINTS%%/"$(subst /,\/,$(OUTPUT))\/constraints.xdc"/ | sed s/%%DEFINE%%/"-verilog_define $(DEFINE)=1"/ > $(OUTPUT)/full.tcl
	$(VIVADO) -mode batch -source $(OUTPUT)/full.tcl -tclargs $(DEFINE) > $(OUTPUT)/report || echo "Failed, run: make $(RULE)"
	$(MV) $(GENERATED_FILES_BASE) $(OUTPUT) || echo "Not all expected files found"
endef

#benchmark top target output_directory
#do not change function name
define base_synthesis
	$(eval BENCH := $1)
	$(eval TOP := $(TOP_$1))
	$(eval TARGET := $($1_TARGET))
	$(eval CLK := $($1_CLK))
	$(eval OUTPUT := $2)
	$(eval RULE := $3)
	$(eval FILES := $(shell echo $(BENCH) | tr A-Z a-z)_files.tcl)
	$(ECHO) "create_clock -period $(TARGET) -name clk [get_ports $(CLK)]" > $(OUTPUT)/constraints.xdc
	$(CAT) $(FILES) base_synth.tcl | sed s/%%TOP%%/$(TOP)/ | sed s/%%PART%%/$(PART)/ | sed s/%%CHECKPOINT%%/"$(subst /,\/,$(OUTPUT))\/checkpoints"/ | sed s/%%CONSTRAINTS%%/"$(subst /,\/,$(OUTPUT))\/constraints.xdc"/ | sed s/%%DEFINE%%/""/> $(OUTPUT)/base.tcl
	$(VIVADO) -mode batch -source $(OUTPUT)/base.tcl > $(OUTPUT)/base_report || echo "Failed, run: make $(RULE)"
	$(MV) $(GENERATED_FILES_BASE) $(OUTPUT) || echo "Not all expected files found"
endef

#benchmark top category define target output_directory base_dir
#do not change function name
define incr_synthesis
	$(eval BENCH := $1)
	$(eval TOP := $(TOP_$1))
	$(eval DEFINE := $2)
	$(eval TARGET := $($1_TARGET))
	$(eval CLK := $($1_CLK))
	$(eval OUTPUT := $3)
	$(eval BASELINE := $4)
	$(eval RULE := $5)
	$(eval FILES := $(shell echo $(BENCH) | tr A-Z a-z)_files.tcl)
	$(ECHO) "create_clock -period $(TARGET) -name clk [get_ports $(CLK)]" > $(OUTPUT)/constraints.xdc
	$(CAT) $(FILES) incr_synth.tcl | sed s/%%TOP%%/$(TOP)/ | sed s/%%PART%%/$(PART)/ | sed s/%%BASE_CHK_DIR%%/"$(subst /,\/,$(BASELINE))\/checkpoints"/ | sed s/%%CHECKPOINT%%/"$(subst /,\/,$(OUTPUT))\/checkpoints"/ | sed s/%%CONSTRAINTS%%/"$(subst /,\/,$(OUTPUT))\/constraints.xdc"/ | sed s/%%DEFINE%%/"-verilog_define ${DEFINE}=1"/ > $(OUTPUT)/incr.tcl
	$(VIVADO) -mode batch -source $(OUTPUT)/incr.tcl -tclargs $(DEFINE) > $(OUTPUT)/report || echo "Failed, run: make $(RULE)"
	$(MV) $(GENERATED_FILES_INCR) $(OUTPUT) || echo "Not all expected files found"
endef

# Please provide a cleanup rule
clean:
	$(RM) $(OUTDIR) $(GENERATED_FILES_ALL)

###############################################################################
### DO NOT TOUCH ANYTHING BEYOND THIS POINT                                 ###
###############################################################################

include ../Makefile.targets



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

TARGET_DLX=10.000
TARGET_ALPHA=10.000
TARGET_FPU=6.000
TARGET_OR1200=10.00
TARGET_MOR1KX=10.00

VIVADO=@vivado

###############################################################################
### YOUR CODE GOES HERE                                                     ###
###############################################################################

## All the code should be indented of one tab, no empty lines

#benchmark top category define target output_directory
#do not change function name
define full_synthesis
	$(eval BENCH := $1)
	$(eval TOP := $2)
	$(eval CATEGORY := $3)
	$(eval DEFINE := $4)
	$(eval TARGET := $5)
	$(eval OUTPUT := $6)
	$(eval FILES := $(shell echo $1 | tr A-Z a-z)_files.tcl)
	$(ECHO) "create_clock -period ${TARGET} -name clk [get_ports clk]" > ${OUTPUT}/constraints.xdc
	$(CAT) $(FILES) base_synth.tcl | sed s/%%TOP%%/${TOP}/ | sed s/%%CHECKPOINT%%/"$(subst /,\/,$(OUTPUT))\/checkpoints"/ | sed s/%%CONSTRAINTS%%/"$(subst /,\/,$(OUTPUT))\/constraints.xdc"/ | sed s/%%DEFINE%%/"-verilog_define ${DEFINE}=1"/ > ${OUTPUT}/${BENCH}_full.tcl
	$(VIVADO) -mode batch -source ${OUTPUT}/${BENCH}_full.tcl -tclargs ${DEFINE} > $(OUTPUT)/${CATEGORY}_report
	$(MV) vivado* $(OUTPUT)
endef

#benchmark top target output_directory
#do not change function name
define base_synthesis
	$(eval BENCH := $1)
	$(eval TOP := $2)
	$(eval TARGET := $3)
	$(eval OUTPUT := $4)
	$(eval FILES := $(shell echo $1 | tr A-Z a-z)_files.tcl)
	$(ECHO) "create_clock -period ${TARGET} -name clk [get_ports clk]" > $(OUTPUT)/constraints.xdc
	$(CAT) $(FILES) base_synth.tcl | sed s/%%TOP%%/${TOP}/ | sed s/%%CHECKPOINT%%/"$(subst /,\/,$(OUTPUT))\/checkpoints"/ | sed s/%%CONSTRAINTS%%/"$(subst /,\/,$(OUTPUT))\/constraints.xdc"/ | sed s/%%DEFINE%%/""/> $(OUTPUT)/${BENCH}_baseline.tcl
	$(VIVADO) -mode batch -source $(OUTPUT)/${BENCH}_baseline.tcl > $(OUTPUT)/base_report
	$(MV) vivado* $(OUTPUT)
endef

#benchmark top category define target output_directory baseline_dir
#do not change function name
define incr_synthesis
	$(eval BENCH := $1)
	$(eval TOP := $2)
	$(eval CATEGORY := $3)
	$(eval DEFINE := $4)
	$(eval TARGET := $5)
	$(eval OUTPUT := $6)
	$(eval BASELINE := $7)
	$(eval FILES := $(shell echo $1 | tr A-Z a-z)_files.tcl)
	$(ECHO) "create_clock -period ${TARGET} -name clk [get_ports clk]" > $(OUTPUT)/constraints.xdc
	$(CAT) $(FILES) inc_synth.tcl | sed s/%%TOP%%/${TOP}/ | sed s/%%BASE_CHK_DIR%%/"$(subst /,\/,$(BASELINE))\/checkpoints"/ | sed s/%%CHECKPOINT%%/"$(subst /,\/,$(OUTPUT))\/checkpoints"/ | sed s/%%CONSTRAINTS%%/"$(subst /,\/,$(OUTPUT))\/constraints.xdc"/ | sed s/%%DEFINE%%/"-verilog_define ${DEFINE}=1"/ > $(OUTPUT)/${BENCH}_incr.tcl
	$(VIVADO) -mode batch -source $(OUTPUT)/${BENCH}_incr.tcl -tclargs ${DEFINE} > $(OUTPUT)/${CATEGORY}_report
	$(MV) vivado* $(OUTPUT)
endef

# Please provide a cleanup rule
clean:
	rm -rf $(OUTDIR) *.jou *.log

###############################################################################
### DO NOT TOUCH ANYTHING BEYOND THIS POINT                                 ###
###############################################################################

include ../Makefile.targets


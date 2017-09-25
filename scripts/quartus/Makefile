
##############################################################################
### TARGET for each benchmark should adjusted to match be within 5% of     ###
### maximum reachable frequency in the flow/technology used.               ###
###                                                                        ###
### If running this makefile, make sure quartus is in your path, or change ###
### the QUARTUS_SH and QUARTUS_STA variables.                              ###
###                                                                        ###
### TOP modules and other parameters should not be changed.                ###
##############################################################################

include ../Makefile.config


###############################################################################
### CHANGE THE FLOW SPECIFIC CONFIGURATION HERE                             ###
###############################################################################

OUTDIR=$(BASEDIR)/quartus

DLX_TARGET=10.000
ALPHA_TARGET=10.000
FPU_TARGET=6.000
OR1200_TARGET=10.00
MOR1KX_TARGET=10.00

QUARTUS_SH=@quartus_sh
QUARTUS_STA=@quartus_sta
QUARTUS_SH_INC=quartus_sh
QUARTUS_STA_INC=quartus_sta


GENERATED_FILES_BASE=db *.qpf  *.qsf output_files simulation timing_report_panel cr_ie_info.json *.sdc
GENERATED_FILES_INCR=$(GENERATED_FILES_BASE) new_rtl_netlist old_rtl_netlist
GENERATED_FILES_ALL=$(GENERATED_FILES_INCR)

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
	$(CAT) prologe.tcl $(FILES) base_synth.tcl | sed s/%%TYPE%%/"project_new -overwrite"/g | sed s/%%TOP%%/$(TOP)/g | sed s/%%DEFINE%%/$(DEFINE)=1/g > $(OUTPUT)/full.tcl
	$(ECHO) "create_clock -period $(TARGET) -name clk [get_ports $(CLK)]" > $(TOP).sdc
	$(QUARTUS_SH) -t $(OUTPUT)/full.tcl > $(OUTPUT)/full_report || echo "Failed, run: make $(RULE)"
	$(CAT) timing.tcl | sed s/%%TOP%%/$(TOP)/g > $(OUTPUT)/timing.tcl
	$(QUARTUS_STA) -t $(OUTPUT)/timing.tcl > $(OUTPUT)/timing.out || echo "Failed, run: make $(RULE)"
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
	$(eval DEFINE := ANUBIS_NOTHING)
	$(eval RULE := $3)
	$(eval FILES := $(shell echo $(BENCH) | tr A-Z a-z)_files.tcl)
	$(CAT) prologe.tcl $(FILES) base_synth.tcl | sed s/%%TYPE%%/"project_new -overwrite"/g | sed s/%%TOP%%/$(TOP)/g | sed s/%%DEFINE%%/$(DEFINE)=1/g > $(OUTPUT)/base.tcl
	$(ECHO) "create_clock -period $(TARGET) -name clk [get_ports $(CLK)]" > $(TOP).sdc
	$(QUARTUS_SH) -t $(OUTPUT)/base.tcl > $(OUTPUT)/base_report || echo "Failed, run: make $(RULE)"
	$(CAT) timing.tcl | sed s/%%TOP%%/$(TOP)/g > $(OUTPUT)/timing.tcl
	$(QUARTUS_STA) -t $(OUTPUT)/timing.tcl > $(OUTPUT)/timing.out || echo "Failed, run: make $(RULE)"
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
	$(CP) $(BASELINE) $(OUTPUT)
	$(CAT) prologe.tcl $(FILES) incr_synth.tcl | sed s/%%TYPE%%/project_open/g | sed s/%%TOP%%/$(TOP)/g | sed s/%%DEFINE%%/$(DEFINE)=1/g > $(OUTPUT)/incr.tcl
	$(ECHO) "create_clock -period $(TARGET) -name clk [get_ports $(CLK)]" > $(OUTPUT)/$(TOP).sdc
	$(CD) $(OUTPUT) ;\
	$(QUARTUS_SH_INC) -t ./incr.tcl > ./incr_report || echo "Failed, run: make $(RULE)"
	$(CAT) timing.tcl | sed s/%%TOP%%/$(TOP)/g > $(OUTPUT)/timing.tcl
	$(CD) $(OUTPUT) ;\
	$(QUARTUS_STA_INC) -t ./timing.tcl > ./timing.out || echo "Failed, run: make $(RULE)"
	#$(MKDIR) $(OUTPUT)/base
	#$(MV) $(OUTPUT)/output_files $(OUTPUT)/simulation $(OUTPUT)/db $(OUTPUT)/base || echo ""
	#$(MV) $(GENERATED_FILES_INCR) $(OUTPUT) || echo "Not all expected files found"
endef

# Please provide a cleanup rule
clean:
	$(RM) $(OUTDIR) $(GENERATED_FILES_ALL)

###############################################################################
### DO NOT TOUCH ANYTHING BEYOND THIS POINT                                 ###
###############################################################################

include ../Makefile.targets


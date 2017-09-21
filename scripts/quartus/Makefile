
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

TARGET_DLX=10.000
TARGET_ALPHA=10.000
TARGET_FPU=6.000
TARGET_OR1200=10.00
TARGET_MOR1KX=10.00

QUARTUS_SH=@quartus_sh
QUARTUS_STA=@quartus_sta

GENERATED_FILES_BASE=db *.qpf  *.qsf output_files simulation timing_report_panel cr_ie_info.json *.sdc
GENERATED_FILES_INCR=$(GENERATED_FILES_BASE)
GENERATED_FILES_ALL=$(GENERATED_FILES_BASE)

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
	$(CAT) prologe.tcl $(FILES) base_synth.tcl | sed s/%%TOP%%/$(TOP)/g | sed s/%%DEFINE%%/$(DEFINE)=1/g | sed s/%%PERIOD%%/$(TARGET)/g | sed s/%%CLK%%/$(CLK)/g	> $(OUTPUT)/full.tcl
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
	$(CAT) prologe.tcl $(FILES) base_synth.tcl | sed s/%%TOP%%/$(TOP)/g | sed s/%%DEFINE%%/$(DEFINE)=1/g | sed s/%%PERIOD%%/$(TARGET)/g | sed s/%%CLK%%/$(CLK)/g	> $(OUTPUT)/base.tcl
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
	$(CAT) prologe.tcl $(FILES) incr_synth.tcl | sed s/%%TOP%%/$(TOP)/g | sed s/%%DEFINE%%/$(DEFINE)=1/g | sed s/%%PERIOD%%/$(TARGET)/g | sed s/%%CLK%%/$(CLK)/g	> $(OUTPUT)/incr.tcl
	$(ECHO) "create_clock -period $(TARGET) -name clk [get_ports $(CLK)]" > $(TOP).sdc
	$(QUARTUS_SH) -t $(OUTPUT)/incr.tcl > $(OUTPUT)/incr_report || echo "Failed, run: make $(RULE)"
	$(CAT) timing.tcl | sed s/%%TOP%%/$(TOP)/g > $(OUTPUT)/timing.tcl
	$(QUARTUS_STA) -t $(OUTPUT)/timing.tcl > $(OUTPUT)/timing.out || echo "Failed, run: make $(RULE)"
	$(MKDIR) $(OUTPUT)/base
	$(MV) $(OUTPUT)/output_files $(OUTPUT)/simulation $(OUTPUT)/db $(OUTPUT)/base
	$(MV) $(GENERATED_FILES_INCR) $(OUTPUT) || echo "Not all expected files found"
endef

# Please provide a cleanup rule
clean:
	$(RM) $(OUTDIR) $(GENERATED_FILES_ALL)

###############################################################################
### DO NOT TOUCH ANYTHING BEYOND THIS POINT                                 ###
###############################################################################

include ../Makefile.targets



#############################################################################
### TARGET for each benchmark should adjusted to match be within 5% of    ###
### maximum reachable frequency in the flow/technology used.              ###
###                                                                       ###
### If running this makefile, make sure yosys is in your path, or change  ###
### the YOSYS variable.                                                   ###
###                                                                       ###
### TOP modules and other parameters should not be changed.               ###
#############################################################################

include ../Makefile.config

###############################################################################
### CHANGE THE FLOW SPECIFIC CONFIGURATION HERE                             ###
###############################################################################

OUTDIR=$(BASEDIR)/yosys

TARGET_DLX=10.000
TARGET_ALPHA=10.000
TARGET_FPU=6.000
TARGET_OR1200=10.00
TARGET_MOR1KX=10.00

YOSYS=@../../yosys/yosys

###############################################################################
### YOUR CODE GOES HERE                                                     ###
###############################################################################

## All the code should be indented of one tab, no empty lines

define all
	$(eval BENCH := $1)
	$(eval TOP := ${TOP_$1})
	$(eval DEFINE := $2)
	$(eval TARGET := ${TARGET_$1})
	$(eval OUTPUT := $3)
	$(eval FILES := $(shell echo ${BENCH} | tr A-Z a-z)_files.tcl)
	$(CAT) $(FILES) base_synth.ys | sed s/%%TOP%%/${TOP}/ | sed s/%%TARGET%%/${TARGET}/ | sed s/%%DEFINE%%/"${DEFINE}=1"/ > ${OUTPUT}/script.ys
	$(YOSYS) -d -s ${OUTPUT}/script.ys > $(OUTPUT)/report
endef


#benchmark top category define target output_directory
#do not change function name
define full_synthesis
	$(call all,$1,$2,$3)
endef


#benchmark top target output_directory
#do not change function name
define base_synthesis
	$(call all,$1,"ANUBIS_NOTHING",$2,$3)
endef

#benchmark top category define target output_directory baseline_dir
#do not change function name
define incr_synthesis
	echo "Nothing to do here"
endef

# Please provide a cleanup rule
clean:
	rm -rf $(OUTDIR)

###############################################################################
### DO NOT TOUCH ANYTHING BEYOND THIS POINT                                 ###
###############################################################################

include ../Makefile.targets


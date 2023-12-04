ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

### OUTPUT BINARY ###
NAME=broadcaster

# The information in the .pinfo file generated by the build system is what will be displayed when
# `use <program name> -i info` is run in the command line.
# PINFO DESCRIPTION is the description that will be displayed in the above command
define PINFO
PINFO DESCRIPTION=CUInSpace $(NAME)
endef

### COMPILER OPTIONS ###
CSTD = gnu11
OPTIMIZATION = -O2

### WARNINGS ###
# (see https://gcc.gnu.org/onlinedocs/gcc-6.3.0/gcc/Warning-Options.html)
WARNINGS += -Wall -Wextra -Wshadow -Wundef -Wformat=2 -Wtrampolines -Wfloat-equal
WARNINGS += -Wbad-function-cast -Wstrict-prototypes -Wpacked
WARNINGS += -Wno-aggressive-loop-optimizations -Wmissing-prototypes -Winit-self
WARNINGS += -Wmissing-declarations -Wmissing-format-attribute -Wunreachable-code
WARNINGS += -Wshift-overflow=2 -Wduplicated-cond -Wpointer-arith -Wwrite-strings
WARNINGS += -Wnested-externs -Wcast-align -Wredundant-decls
WARNINGS += -Werror=implicit-function-declaration -Wlogical-not-parentheses
WARNINGS += -Wlogical-op -Wold-style-definition -Wcast-qual -Wdouble-promotion
WARNINGS += -Wunsuffixed-float-constants -Wmissing-include-dirs -Wnormalized
WARNINGS += -Wdisabled-optimization -Wsuggest-attribute=const

### UPDATE CFLAGS ###
CCFLAGS += -std=$(CSTD) $(WARNINGS)

#### PROJECT SPECIFIC ####

### PROJECT INCLUDES ###
EXTRA_INCVPATH += $(PROJECT_ROOT)/src/include

### SOURCE FILES ###
EXTRA_SRCVPATH += $(PROJECT_ROOT)/src

include $(MKFILES_ROOT)/qtargets.mk

# Make optimized binary
optimized: CCFLAGS += $(OPTIMIZATION) 
optimized: all

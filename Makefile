
#
# input variables
#

## output dirs and objects
OUTPUT_DIR := build

ifndef RELEASE
OUTPUT_DIR := $(OUTPUT_DIR)/debug
GLOBAL_CFLAGS ?= -O0 -g
GLOBAL_CXXFLAGS ?= -O0 -g
else
OUTPUT_DIR := $(OUTPUT_DIR)/release
GLOBAL_CFLAGS ?= -Os
GLOBAL_CXXFLAGS ?= -Os
endif

LIB_OUTPUT_DIR := $(OUTPUT_DIR)/lib
OUTPUT_LIB := $(OUTPUT_DIR)/pg_cbor.so


EXEC_OUTPUT_DIR := $(OUTPUT_DIR)/exec
OUTPUT_EXEC := $(OUTPUT_DIR)/pg_cbor_test


## project root
GLOBAL_ROOT := .

## executables
GLOBAL_MKDIR ?= mkdir -p
GLOBAL_RM ?= rm -f

GLOBAL_CC ?= gcc
GLOBAL_CPP ?= g++
GLOBAL_AR ?= ar rcs

.DEFAULT_GOAL := all

include lib.mk
include exec.mk

# include sources dependencies
-include $(patsubst %.o,%.d,$(LIB_OBJS))

all: .prebuild $(OUTPUT_LIB) $(OUTPUT_EXEC)

.prebuild:
	@$(GLOBAL_MKDIR) $(LIB_DIRS) $(EXEC_DIRS)

clean:
	$(GLOBAL_RM) -r $(OUTPUT_DIR)

.PHONY: all prebuild clean

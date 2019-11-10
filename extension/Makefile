MODULE_big = pg_cbor
EXTENSION = pg_cbor
DATA = pg_cbor--0.2.sql

GLOBAL_ROOT := ..

# lib sources
# recursive source files
LIB_SRCS_DIRS += \
	pg \
	source

# individual includes
LIB_INCLUDES_OBJS += \
	$(GLOBAL_ROOT)/include

#
# make lists
#

# search for sources
LIB_SRCS := \
	$(foreach dir,$(LIB_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.c')) \
	$(addprefix $(GLOBAL_ROOT)/,$(LIB_SRCS_OBJS))

# search for includes
LIB_INCLUDES := \
	$(foreach dir,$(LIB_INCLUDES_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(LIB_INCLUDES_OBJS)

# build object list to compile
OBJS := $(patsubst %.c,%.o,$(LIB_SRCS))

PG_CPPFLAGS = $(addprefix -I,$(LIB_INCLUDES)) -DPOSTGRES

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
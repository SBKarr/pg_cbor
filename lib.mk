
# lib sources
# recursive source files
LIB_SRCS_DIRS += \
	pg \
	source

# individual source files
LIB_SRCS_OBJS += \

# recursive includes
LIB_INCLUDES_DIRS += \

# individual includes
LIB_INCLUDES_OBJS += \
	$(GLOBAL_ROOT)/include \
	$(shell pg_config --includedir-server)

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
LIB_OBJS := $(patsubst %.c,%.o,$(patsubst $(GLOBAL_ROOT)/%,$(LIB_OUTPUT_DIR)/%,$(LIB_SRCS)))

# build directory list to create
LIB_DIRS := $(sort $(dir $(LIB_OBJS)))

# build compiler include flag list
LIB_INPUT_CFLAGS := $(addprefix -I,$(LIB_INCLUDES)) -fPIC -DPOSTGRES

LIB_BUILD_CFLAGS := $(GLOBAL_CFLAGS) $(LIB_INPUT_CFLAGS) $(CFLAGS)

# build c sources
$(LIB_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c
	$(GLOBAL_CC) -MMD -MP -MF $(LIB_OUTPUT_DIR)/$*.d $(LIB_BUILD_CFLAGS) $< -c -o $@

$(OUTPUT_LIB): $(LIB_OBJS)
	$(GLOBAL_CPP)  $(LIB_OBJS) -shared -o $(OUTPUT_LIB)


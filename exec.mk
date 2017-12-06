
# lib sources
# recursive source files
EXEC_SRCS_DIRS += \
	source \
	test

# individual source files
EXEC_SRCS_OBJS += \

# recursive includes
EXEC_INCLUDES_DIRS += \

# individual includes
EXEC_INCLUDES_OBJS += \
	$(GLOBAL_ROOT)/include \
	$(shell pg_config --includedir-server)

#
# make lists
#

# search for sources
EXEC_SRCS := \
	$(foreach dir,$(EXEC_SRCS_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -name '*.c')) \
	$(addprefix $(GLOBAL_ROOT)/,$(EXEC_SRCS_OBJS))

# search for includes
EXEC_INCLUDES := \
	$(foreach dir,$(EXEC_INCLUDES_DIRS),$(shell find $(GLOBAL_ROOT)/$(dir) -type d)) \
	$(EXEC_INCLUDES_OBJS)

# build object list to compile
EXEC_OBJS := $(patsubst %.c,%.o,$(patsubst $(GLOBAL_ROOT)/%,$(EXEC_OUTPUT_DIR)/%,$(EXEC_SRCS)))

# build directory list to create
EXEC_DIRS := $(sort $(dir $(EXEC_OBJS)))

# build compiler include flag list
EXEC_INPUT_CFLAGS := $(addprefix -I,$(EXEC_INCLUDES))

EXEC_BUILD_CFLAGS := $(GLOBAL_CFLAGS) $(EXEC_INPUT_CFLAGS) $(CFLAGS)

# build c sources
$(EXEC_OUTPUT_DIR)/%.o: $(GLOBAL_ROOT)/%.c
	$(GLOBAL_CC) -MMD -MP -MF $(EXEC_OUTPUT_DIR)/$*.d $(EXEC_BUILD_CFLAGS) $< -c -o $@

$(OUTPUT_EXEC): $(EXEC_OBJS)
	$(GLOBAL_CPP)  $(EXEC_OBJS) -o $(OUTPUT_EXEC)

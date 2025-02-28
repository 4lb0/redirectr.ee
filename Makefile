TARGET_EXEC := redirectr
BUILD_DIR := ./build

SRCS := $(shell find . -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
OBJ_DIRS := $(sort $(dir $(OBJS)))

CFLAGS := -Wall -Wextra -pedantic -g
LDFLAGS := -std=c23

all: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c | $(OBJ_DIRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIRS):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

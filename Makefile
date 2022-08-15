# Vars

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

CC := clang

CPPFLAGS := -Iinclude -MMD -MP
CFLAGS   := -g3 -Wall -Wcast-align -g
CFLAGS   += -Winline -Wfloat-equal -Wnested-externs
CFLAGS   += -std=gnu11

EXE := $(BIN_DIR)/wsh
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

# Recipes

all: $(EXE)

# Main executable
$(EXE): $(OBJ) | $(BIN_DIR)
	@$(CC) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DPROMPT -c $< -o $@

$(OBJQ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJQ_DIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
# End of main executable

# 
$(OBJ_DIR) $(OBJQ_DIR) $(BIN_DIR):
	@mkdir -p $@

clean:
	@$(RM) -rv $(OBJ_DIR) $(OBJQ_DIR)

-include $(OBJ:.o=.d)

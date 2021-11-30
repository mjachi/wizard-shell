
# Vars

SRC_DIR := src
OBJ_DIR := obj
OBJD_DIR := objd
BIN_DIR := bin

CPPFLAGS := -Iinclude -MMD -MP
# CFLAGS   := -g3 -Wall -Wcast-align -g
CFLAGS   := -g3 -g
CFLAGS   += -Winline -Wfloat-equal -Wnested-externs
CFLAGS   += -std=gnu11
#LDFLAGS  := -Llib
#LDLIBS   := -lm 

EXE := $(BIN_DIR)/wsh
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJD := $(SRC:$(SRC_DIR)/%.c=$(OBJD_DIR)/%.o)

.PHONY: all clean

# Recipes

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	#$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	@$(CC) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DPROMPT -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

clean:
	@$(RM) -rv $(OBJ_DIR) $(BIN_DIR)

-include $(OBJ:.o=.d)




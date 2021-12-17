
# Vars

SRC_DIR := src
OBJ_DIR := obj
OBJQ_DIR := objq
BIN_DIR := bin

CC := gcc

CPPFLAGS := -Iinclude -MMD -MP
# CFLAGS   := -g3 -Wall -Wcast-align -g
CFLAGS   := -g3 -g
CFLAGS   += -Winline -Wfloat-equal -Wnested-externs
CFLAGS   += -std=gnu11
#LDFLAGS  := -Llib
#LDLIBS   := -lm 

EXE := $(BIN_DIR)/wsh
QEXE := $(BIN_DIR)/wshq  
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJQ := $(SRC:$(SRC_DIR)/%.c=$(OBJQ_DIR)/%.o)

.PHONY: all clean

# Recipes

all: $(EXE) $(QEXE)

$(QEXE): $(OBJQ) | $(BIN_DIR)
	@$(CC) $(LDFLAGS) $^ -o $@

$(EXE): $(OBJ) | $(BIN_DIR)
	#$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	@$(CC) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DPROMPT -c $< -o $@

$(OBJQ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJQ_DIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


$(OBJ_DIR) $(OBJQ_DIR) $(BIN_DIR):
	@mkdir -p $@

clean:
	@$(RM) -rv $(OBJ_DIR) $(OBJQ_DIR) $(BIN_DIR)

-include $(OBJ:.o=.d)




CC = gcc
CFLAGS = -std=c99 -pthread -g
LDFLAGS = -lopen62541 -lcrypto
BIN_DIR = bin
SRC_DIR = src
SERVER_SRC = $(SRC_DIR)/server.c
UTILS_SRC = $(SRC_DIR)/utils.c
SERVER_BIN = $(BIN_DIR)/server

.PHONY: all clean rules

all: $(SERVER_BIN)

$(SERVER_BIN): $(SERVER_SRC) $(UTILS_SRC)
	mkdir -p $(BIN_DIR)
	@echo "Building $@..."
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	@echo "Build completed."

run: $(SERVER_BIN)
	./$(SERVER_BIN)

clean:
	@rm -rf $(BIN_DIR)

rules:
	@echo "Avaible rules:"
	@echo " all		: Build the server executables"
	@echo "	run		: Build then run the server"
	@echo " clean	: Clean up generated files"
	@echo " rules	: Show available rules"

cc = gcc
CFLAGS = -std=c99 -g
LDFLAGS = -lopen62541 -lcrypto
BIN_DIR = bin
SRC_DIR = src
CLIENT_SRC = $(SRC_DIR)/client.c
UTILS_SRC = $(SRC_DIR)/utils.c
CLIENT_BIN = $(BIN_DIR)/client
SERVER_IP = 192.168.247.45 #A modifier selon l'ip du server

.PHONY: all clean rules

all: $(CLIENT_BIN)

$(CLIENT_BIN): $(CLIENT_SRC) $(UTILS_SRC)
	mkdir -p $(BIN_DIR)
	@echo "Building $@..."
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	@echo "Build completed."

run: $(CLIENT_BIN)
	./$< $(SERVER_IP)

clean:
	@rm -rf $(BIN_DIR)

rules:
	@echo "Available rules:"
	@echo " all	: Build the client executables"
	@echo " run	: Build then run the client request"
	@echo " clean	: Clean up generated files"
	@echo " rules	: Show available rules"

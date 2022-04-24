OBJ = $(shell find src -type f -iname '*.h' -or -iname '*.c')

lint: $(OBJ)
	@clang-format -style=file -i $(OBJ)
	@echo "reformatted successfully"

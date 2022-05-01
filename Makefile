OBJ = $(shell find src -type f -iname '*.h' -or -iname '*.c')

help:
	@echo "lint \t\t lint the source code"

lint: $(OBJ)
	@clang-format -style=file -i $(OBJ)
	@echo "reformatted successfully"

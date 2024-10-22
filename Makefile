.PHONY: help
help: # Show help for each of the Makefile recipes.
	@grep -E '^[a-zA-Z0-9 -]+:.*#'  Makefile | sort | while read -r l; do printf "\e[0;36m$$(echo $$l | cut -f 1 -d':')\033[00m:$$(echo $$l | cut -f 2- -d'#')\n"; done

.PHONY: clangFormatCI
clangFormatCI: # Run clang-format for the project like in the CI
	scripts/run-clang-format.py -j8 -r --clang-format-executable clang-format --color always .

.PHONY: clangFormat
clangFormat: # Run clang-format for the project and reformat the code according to the .clang-format file
	scripts/run-clang-format.py -j8 -r --clang-format-executable clang-format --color always -i .

.PHONY: buildDocker
buildDocker: # Run clang-format for the project and reformat the code according to the .clang-format file
	scripts/build.sh

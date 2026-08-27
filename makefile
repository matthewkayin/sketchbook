# ------------------------------------------------------------------------------
# Project
# ------------------------------------------------------------------------------

ASSEMBLY        := sbk
BUILD_DIR       := bin
OBJ_DIR         := obj
SRC_DIR         := src

C               := clang
C_FLAGS         := -std=c11
CXX             := clang++
CXX_FLAGS       := -std=c++20 -Wall -Wextra -Wshadow
LD_FLAGS        :=

SHADER_XX       := slangc
SHADER_XX_FLAGS := -target spirv -profile spirv_1_4 -emit-spirv-directly
SHADER_SRC_DIR  := shader

INCLUDE_FLAGS   := -Isrc -isystem vendor
DEFINES         := -D_CRT_SECURE_NO_WARNINGS

EXTENSION       :=

# ------------------------------------------------------------------------------
# Build configuration
# ------------------------------------------------------------------------------

CXX_FLAGS += -g -O0
LD_FLAGS += -g
DEFINES += -DSBK_DEBUG

# ------------------------------------------------------------------------------
# Platform Detection
# ------------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
	PLATFORM   := win64
	EXTENSION  := .exe
	LIB_DIR    := lib\win64

	# Recursive wildcard
	rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
	SRC_FILES   := $(call rwildcard,$(SRC_DIR)/,*.cpp)
	DIRECTORY   := $(subst /,\,${CURDIR})
	SHELL       := cmd
	DIRECTORIES := \$(SRC_DIR) $(subst $(DIRECTORY),,$(shell dir $(SRC_DIR) /S /AD /B | findstr /i $(SRC_DIR)))

	SHADER_FILES := $(call rwildcard,$(SHADER_SRC_DIR)/,*.slang)
	SHADER_DIRECTORIES := \$(SHADER_SRC_DIR) $(subst $(DIRECTORY),,$(shell dir $(SHADER_SRC_DIR) /S /AD /B | findstr /i $(SHADER_SRC_DIR)))

	LD_FLAGS += \
		-L$(LIB_DIR) \
		-lSDL3 \
		-lvulkan-1

else
	UNAME_S := $(shell uname -s)

	SRC_FILES := $(shell find $(SRC_DIR) -type f \( -name "*.cpp" -o -name "*.mm" \))
	SRC_FILES += $(shell find vendor -type f \( -name "*.c" \))
	DIRECTORIES := $(shell find src -type d)
	DIRECTORIES += $(shell find vendor -type d)

	SHADER_FILES := $(shell find $(SHADER_SRC_DIR) -type f \( -name "*.slang" \))
	SHADER_DIRECTORIES := $(shell find $(SHADER_SRC_DIR) -type d)

	ifeq ($(UNAME_S),Darwin)
		PLATFORM := macos
		LIB_DIR  := lib/macos

		LD_FLAGS += \
			-L$(LIB_DIR) \
			-lvulkan \
			-F$(LIB_DIR) \
			-framework SDL3
		DEFINES += \
			-Wno-deprecated-declarations

		# rpath
		ifeq ($(RELEASE),1)
			LD_FLAGS += -Wl,-rpath,.
		else
			# In debug mode, the rpath points to ../lib/macos
			# This way we don't have to libcopy
			LD_FLAGS += -Wl,-rpath,../$(LIB_DIR)
		endif
	endif

	ifeq ($(UNAME_S),Linux)
		PLATFORM := linux
		LIB_DIR  := lib/linux64

		LD_FLAGS += \
			-lSDL3 \
			-lSDL3_image \
			-lvulkan
	endif

endif

# ------------------------------------------------------------------------------
# Source/Object Lists
# ------------------------------------------------------------------------------

OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)
SPV_FILES = $(patsubst $(SHADER_SRC_DIR)/%,$(BUILD_DIR)/$(SHADER_SRC_DIR)/%,$(SHADER_FILES:.slang=.spv))

# ------------------------------------------------------------------------------
# Target Recipes
# ------------------------------------------------------------------------------

.PHONY: all
all: scaffold compile shader-compile link

# ------------------------------------------------------------------------------
# Scaffold
# ------------------------------------------------------------------------------

.PHONY: scaffold
scaffold:
	@echo Scaffolding...
ifeq ($(PLATFORM),win64)
	@mkdir $(addprefix $(OBJ_DIR), $(DIRECTORIES)) 2>NUL || cd .
	@mkdir $(BUILD_DIR) 2>NUL || cd .
	@mkdir $(addprefix $(BUILD_DIR), $(SHADER_DIRECTORIES)) 2>NUL || cd .
else
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
	@mkdir -p $(addprefix $(BUILD_DIR)/,$(SHADER_DIRECTORIES))
endif
	@echo Done.

# ------------------------------------------------------------------------------
# Compile
# ------------------------------------------------------------------------------

# This phony just prints messages before compilation
.PHONY: compile
compile:
	@echo Compiler flags: $(CXX_FLAGS)
	@echo Defines:       $(DEFINES)
	@echo Compiling...

# Compile cpp to object
$(OBJ_DIR)/%.cpp.o: %.cpp
	@echo   $<...
	@$(CXX) $< $(CXX_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

# Compile c to object
$(OBJ_DIR)/%.c.o: %.c
	@echo   $<...
	@$(C) $< $(C_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

# ------------------------------------------------------------------------------
# Shader Compile
# ------------------------------------------------------------------------------

.PHONY: shader-compile
shader-compile: $(SPV_FILES)

# Compile slang to spir-v
$(BUILD_DIR)/%.spv: %.slang
	@echo   $<...
	@$(SHADER_XX) $(SHADER_XX_FLAGS) $< -o $@

# ------------------------------------------------------------------------------
# Link
# ------------------------------------------------------------------------------

.PHONY: link
link: scaffold $(OBJ_FILES)
	@echo Linker flags $(LD_FLAGS)
	@echo Linking $(ASSEMBLY)...
ifeq ($(PLATFORM),win64)
	@$(CXX) $(OBJ_FILES) -o $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) $(LD_FLAGS)
else
	@$(CXX) $(OBJ_FILES) -o $(BUILD_DIR)/$(ASSEMBLY)$(EXTENSION) $(LD_FLAGS)
endif

# ------------------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------------------

.PHONY: clean
clean:
	@echo Cleaning...

ifeq ($(PLATFORM),win64)
	@if exist $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) del $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION)
	@if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
	@if exist $(BUILD_DIR)\res rmdir /s /q $(BUILD_DIR)\res
else
	@rm -rf $(OBJ_DIR)
	@rm -f $(BUILD_DIR)/$(ASSEMBLY)$(EXTENSION)
	@rm -rf $(BUILD_DIR)/res
endif

# ------------------------------------------------------------------------------
# Utility Targets
# ------------------------------------------------------------------------------

.PHONY: libcopy
libcopy:
ifeq ($(PLATFORM),win64)
	-@setlocal enableextensions && xcopy $(LIB_DIR) $(BUILD_DIR) /Y
endif
ifeq ($(PLATFORM),macos)
	-@cp $(LIB_DIR)/*.dylib $(BUILD_DIR)/
endif
ifeq ($(PLATFORM),linux)
	-@cp $(LIB_DIR)/* $(BUILD_DIR)/
endif

# On my MacOS, x86 compiler (installed by macports) is prefixed by i386-elf- 
OS=$(shell uname -s)
ifeq ($(OS), Darwin)
	PREFIX=i386-elf-
else 
	PREFIX=
endif

GCC_PATH=$(shell which $(PREFIX)gcc)
GCC_DIR=$(shell dirname $(GCC_PATH))

DEBUG=$(GCC_DIR)/$(PREFIX)gdb
CC=$(GCC_DIR)/$(PREFIX)gcc
CFLAGS= -m32 -g -ggdb -std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra # -Werror

LD=$(GCC_DIR)/$(PREFIX)ld.bfd
LDFLAGS = -melf_i386

AS=$(GCC_DIR)/$(PREFIX)gcc
ASFLAGS= -m32 -DASSEMBLER -g -ggdb

OBJCOPY=$(GCC_DIR)/$(PREFIX)objcopy
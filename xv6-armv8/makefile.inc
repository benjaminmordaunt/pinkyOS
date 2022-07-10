TRIPLE := aarch64-none-elf

CC := clang 
AS := llvm-as
LD := ld.lld
OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump
DEBUG ?= "-DCONFIG_DEBUG"
FDT_INCLUDE := dtc/libfdt

CFLAGS = -march=armv8-a -mtune=cortex-a57 -fno-pic -static -fno-builtin -fno-strict-aliasing -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Werror -I. -Iinclude -I$(FDT_INCLUDE) -g -target $(TRIPLE) $(DEBUG)
LDFLAGS ?= -L. -nostdlib
ASFLAGS = -march=armv8-a -target $(TRIPLE)

LIBGCC = $($(CC) -print-libgcc-file-name)

# host compiler
HOSTCC_preferred = gcc
define get_hostcc
    $(if $(shell which $(HOSTCC_preferred)),$(HOSTCC_preferred),"cc")
endef
HOSTCC := $(call get_hostcc)

# general rules
quiet-command = $(if $(V),$1,$(if $(2),@echo $2 && $1, @$1))

LINK_BIN = $(call quiet-command,$(LD) $(LDFLAGS) \
	-T $(1) -o $(2) $(3) $(LIBS) -b binary $(4), "  LINK     $(TARGET_DIR)$@")

LINK_INIT = $(call quiet-command,$(LD) $(LDFLAGS) \
	$(1) -o $@.out $<, "  LINK     $(TARGET_DIR)$@")
OBJCOPY_INIT = $(call quiet-command,$(OBJCOPY) \
	-S -O binary --prefix-symbols="_binary_$@" $@.out $@, "  OBJCOPY  $(TARGET_DIR)$@")

build-directory = $(shell mkdir -p build build/device build/lib)

build/%.o: %.c
	$(call build-directory)
	$(call quiet-command,$(CC) $(CFLAGS) \
		-c -o $@ $<,"  CC       $(TARGET_DIR)$@")

AS_WITH = $(call quiet-command,$(CC) $(ASFLAGS) \
		$(1) -c -o $@ $<,"  AS       $(TARGET_DIR)$@")

build/%.o: %.S
	$(call build-directory)
	$(call AS_WITH, )

build-libfdt = $(MAKE) CC=$(CC) AS=$(AS) LD=$(LD) CFLAGS="$(CFLAGS) -I../include" LDFLAGS="$(LDFLAGS)" libfdt

# SPDX-License-Identifier: GPL-2.0

purgatory-y := head.o purgatory.o string.o sha256.o mem.o

targets += $(purgatory-y) purgatory.lds purgatory purgatory.chk purgatory.ro
PURGATORY_OBJS = $(addprefix $(obj)/,$(purgatory-y))

$(obj)/sha256.o: $(srctree)/lib/crypto/sha256.c FORCE
	$(call if_changed_rule,cc_o_c)

CFLAGS_sha256.o := -D__NO_FORTIFY

$(obj)/mem.o: $(srctree)/arch/s390/lib/mem.S FORCE
	$(call if_changed_rule,as_o_S)

KBUILD_CFLAGS := -std=gnu11 -fno-strict-aliasing -Wall -Wstrict-prototypes
KBUILD_CFLAGS += -Wno-pointer-sign -Wno-sign-compare
KBUILD_CFLAGS += -fno-zero-initialized-in-bss -fno-builtin -ffreestanding
KBUILD_CFLAGS += -Os -m64 -msoft-float -fno-common
KBUILD_CFLAGS += -fno-stack-protector
KBUILD_CFLAGS += -DDISABLE_BRANCH_PROFILING
KBUILD_CFLAGS += -D__DISABLE_EXPORTS
KBUILD_CFLAGS += $(CLANG_FLAGS)
KBUILD_CFLAGS += $(call cc-option,-fno-PIE)
KBUILD_AFLAGS := $(filter-out -DCC_USING_EXPOLINE,$(KBUILD_AFLAGS))
KBUILD_AFLAGS += -D__DISABLE_EXPORTS

# Since we link purgatory with -r unresolved symbols are not checked, so we
# also link a purgatory.chk binary without -r to check for unresolved symbols.
PURGATORY_LDFLAGS := -nostdlib -z nodefaultlib
LDFLAGS_purgatory := -r $(PURGATORY_LDFLAGS) -T
LDFLAGS_purgatory.chk := -e purgatory_start $(PURGATORY_LDFLAGS)
$(obj)/purgatory: $(obj)/purgatory.lds $(PURGATORY_OBJS) FORCE
		$(call if_changed,ld)

$(obj)/purgatory.chk: $(obj)/purgatory FORCE
		$(call if_changed,ld)

OBJCOPYFLAGS_purgatory.ro := -O elf64-s390
OBJCOPYFLAGS_purgatory.ro += --remove-section='*debug*'
OBJCOPYFLAGS_purgatory.ro += --remove-section='.comment'
OBJCOPYFLAGS_purgatory.ro += --remove-section='.note.*'
$(obj)/purgatory.ro: $(obj)/purgatory $(obj)/purgatory.chk FORCE
		$(call if_changed,objcopy)

$(obj)/kexec-purgatory.o: $(obj)/purgatory.ro

obj-y += kexec-purgatory.o

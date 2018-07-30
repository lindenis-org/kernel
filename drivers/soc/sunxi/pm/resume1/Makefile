
# host program for this
hostprogs-y	:= gen_check_code

gen_check_code-objs := gen_check_sum/check.o \
			gen_check_sum/main.o \
			gen_check_sum/path.o \
			gen_check_sum/script.o

HOST_EXTRACFLAGS += -I$(src)/gen_check_sum/include/


#makefile for resume1.code
always  := gen_check_code resume1.code
targets := resume1.elf

GCOV_PROFILE=no

#use "-Os" flags.
#Don't use "-O2" flags.
KBUILD_CFLAGS := -g -c -nostdlib -march=armv7-a -D__LINUX_ARM_ARCH__=7 -marm -fno-unwind-tables -fno-jump-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 -mno-unaligned-access

#Include the cur dir.
KBUILD_CPPFLAGS += -I.

LD_FLAGS = -static
LIBS =

INCLUDE   = -I.                              \
            -I$(KDIR)/include                \
            -I$(KDIR)/drivers/soc/sunxi/pm

resume1-y := resume1_head.o                  \
             resume1_entry.o                 \
             resume1_sram.o

resume1-y := $(addprefix $(obj)/,$(resume1-y))

$(obj)/resume1.code: $(obj)/resume1.bin $(obj)/gen_check_code
	$(Q)$(obj)/gen_check_code $(obj)/resume1.bin $(obj)/resume1.code

OBJCOPYFLAGS_resume1.bin = -g -R .comment -R .note --strip-all \
				-R .note.gnu.build-id -O binary

$(obj)/resume1.bin: $(obj)/resume1.elf FORCE
	$(call if_changed,objcopy)

ifneq ($(strip $(CONFIG_ARCH_SUN8I)),)

quiet_cmd_resumeld = RELD $@
      cmd_resumeld = $(CC) -nostdlib -o $@ \
		     -Wl,-T,$(filter %.scat,$^) -Wl,-EL $(filter %.o,$^)

$(obj)/resume1.elf: $(obj)/sun8i_resume1_scatter.scat $(resume1-y)
	$(call if_changed,resumeld)

quiet_cmd_reobjdump = REDUMP $@
      cmd_reobjdump = $(OBJDUMP) -D $(filter-out FORCE,$^) > $(@)
$(obj)/resume1.lst: $(obj)/resume1.elf FORCE
	$(call if_changed,reobjdump)
endif

clean:
	-rm $(obj) *.code *.bin *.map *.lst *.elf

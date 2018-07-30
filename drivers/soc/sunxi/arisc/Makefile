#
# Makefile for the linux kernel.
#

obj-y=arisc.o arisc-notifier.o scp-call.o
obj-y += interfaces/

ifeq ($(CONFIG_SUNXI_ARISC_COM_DIRECTLY),y)
obj-y += hwmsgbox/
obj-y += message_manager/
obj-y += arisc_dram.o
endif

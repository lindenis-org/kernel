	.globl  arisc_binary_start
	.globl  arisc_binary_end
arisc_binary_start:
	.incbin "drivers/soc/sunxi/arisc/binary/arisc_sun8iw12p1.bin"
arisc_binary_end:
	.align  2


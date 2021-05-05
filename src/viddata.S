.section .rodata
.global viddata
.type viddata, %object
.align 4

viddata:
.incbin "video.bin"

.global viddata_size
.type viddata_size, %object
.align 4

viddata_size:
.4byte viddata_size - viddata

.section .rodata.fh_install_hints,"a",%progbits
.balign 4
.global fh_install_hints_compressed
.global fh_install_hints_compressed_end
fh_install_hints_compressed:
.incbin "install-texture-hints.zlib"
fh_install_hints_compressed_end:
.section .note.GNU-stack,"",%progbits

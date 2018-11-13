; Copyright (C) 2018 FIX94
;
; This software may be modified and distributed under the terms
; of the MIT license.  See the LICENSE file for details.

SECTION "HDR",ROM0[$0]

rstbase:
	; if any RST lands here, return
	ds $FF
	reti

entry:
	; 100, goto start
	nop
	jp _start

SECTION "MAIN",ROM0[$150]

_start:
	di
	; init stack pointer
	ld sp, $CFFF
	; make sure lcd is off
	ldh a, [$FF40]
	bit 7, a
	jr z, isoff
	; not off, wait for vblank
waitVBlank:
	ldh a, [$FF44]
	cp 145
	jr nz, waitVBlank
	; turn off lcd
	xor a
	ldh [$FF40], a
isoff:
	; copy in font
	ld hl, $8800
	ld de, _8800_bin
	ld bc, $400
	call _memcpy
	ld hl, $8E00
	ld de, _8E00_bin
	ld bc, $200
	call _memcpy
	; clear font "space" tile
	ld hl, $97F0
	xor a
bartileloop:
	ld [hl+], a
	bit 3, h
	jr z, bartileloop
	; set font color
	ld a, $E4
	ldh [$FF47], a
	; turn on lcd
	ld a, $81
	ldh [$FF40], a
	; copy in RAM code
	ld hl, $C000
	ld de, standalone_bin
	ld bc, (standalone_bin_end-standalone_bin)
	call _memcpy
	; jump to RAM code
	jp $C000

_memcpy:
	inc b
	inc c
	jr _memcpy_chk
_memcpy_loop:
	ld a, [de]
	inc de
	ld [hl+], a
_memcpy_chk:
	dec c
	jr nz, _memcpy_loop
	dec b
	jr nz, _memcpy_loop
	ret

; font bin to copy into vram
_8800_bin:
	INCBIN "8800.bin"

_8E00_bin:
	INCBIN "8E00.bin"

; actual dumper code
standalone_bin:
	INCBIN "../standalone.bin"
standalone_bin_end:
	; label just for size calc

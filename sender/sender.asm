; Copyright (C) 2018 FIX94
;
; This software may be modified and distributed under the terms
; of the MIT license.  See the LICENSE file for details.

include "charmap.asm"

SECTION "WRAM",ROM0[RAMOFFSET]

init:
	di
	; for safety
	xor a
	ld [$0000], a
	; reset audio regs
	ldh [$FF26], a
	ld a, $80
	ldh [$FF26], a
	; set wav to low
	ld h, $FF
	ld l, $30
	xor a
clearsamples:
	ld [hl+], a
	bit 6, l
	jr z, clearsamples
	; wav 100% volume
	ld a, $20
	ldh [$FF1C], a
	; freq of 7F0 (probably not needed right
	; now because its only one solid output)
	;ld a, $F0
	;ldh [$FF1D], a
	; enable wav dac and trigger
	ld a, $87
	ldh [$FF1A], a
	ldh [$FF1E], a
	; audio ready, update video
	call waitVBlank
	; turn off lcd
	xor a
	ldh [$FF40], a
	; clear various regs
	ldh [$FF02], a
	ldh [$FF07], a
	ldh [$FF41], a
	; disable scroll
	ldh [$FF42], a
	ldh [$FF43], a
	; move lyc and wy out
	ld a, $9B
	ldh [$FF45], a
	ldh [$FF4A], a
	call clearScreen
	call drawHeader
	; clear cpu interrupts
	xor a
	ldh [$FFFF], a
	ldh [$FF0F], a
	; turn on lcd
	ld a, $81
	ldh [$FF40], a
	; time to wait for cart
promptinsertcart:
	ld hl, startmsg
	call drawStatus
	call waitStart
	; for safety
	xor a
	ld [$0000], a
	; verify various static
	; bytes of the header
	; to ensure cart is good
	ld hl, $107
	ld a, [hl+]
	cp $66
	jr nz, checkerr
	ld a, [hl+]
	cp $CC
	jr nz, checkerr
	inc l
	ld a, [hl]
	cp 0
	jr nz, checkerr
	ld l, $16
	ld a, [hl]
	cp $11
	jr nz, checkerr
	jr checkok
checkerr:
	; errors in static bytes,
	; prompt to try again
	ld hl, errmsg
	call drawStatus
	call waitA
	jr promptinsertcart
checkok:
	; cart looks good, prompt
	; to start sending
	ld hl, okmsg
	call drawStatus
waitokprompt:
	call readButtons
	bit 0, b
	jr z, startSendROM
	bit 1, b
	jr nz, waitokprompt
	; B pressed, send save
	call sendSave
	jr end
startSendROM:
	; A pressed, send ROM
	call sendROM
end:
	; done dumping, wait for
	; another cart or turning off
	ld hl, donemsg
	call drawStatus
	call waitA
	; back to the beginning
	jp promptinsertcart

readButtons:
	; enable button bits
	ld a, $10
	ldh [$FF00], a
	ldh a, [$FF00]
	ldh a, [$FF00]
	ldh a, [$FF00]
	ldh a, [$FF00]
	ldh a, [$FF00]
	ldh a, [$FF00]
	ld b, a
	; disable button bits
	ld a, $30
	ldh [$FF00], a
	ret

waitA:
	call readButtons
	bit 0, b
	jr nz, waitA
	ret

waitStart:
	call readButtons
	bit 3, b
	jr nz, waitStart
	ret

sendROM:
	; ROM mode for mbc1
	xor a
	ld [$6000], a
	; init bank total
	; read ROM size
	ld a, [$148]
	ld b, a
	; check high nibble
	and $F0
	cp $50
	jr z, addh
	; size value not 5X
	ld a, b
	jr getbanktotal
addh:
	; add 7 to low nibble to
	; get to special bank lut
	ld a, b
	and 7
	add 7
getbanktotal:
	ld hl, banklut
	and $F
	sla a
	ld c, a
	ld b, 0
	add hl, bc
	ld a, [hl+]
	ld [banktotal], a
	ld a, [hl]
	ld [banktotal+1], a
	; set sending status
	ld hl, sendmsg
	call drawStatus
	; done initializing bank total
	ld de, vollut
	call testsend
	; send "ROM" file status
	ld a, $A
	call sendnibble
	; send fixed bank 0
	ld c, $3F
	ld de, vollut
	ld hl, $0000
	call sendbank
	; init swappable bank
	ld de, $0000
sendROMloop:
	; check total bank low
	inc d
	ld a, [banktotal]
	cp d
	jr nz, setbanklow
	; check if total high exists
	ld a, [banktotal+1]
	or a
	jr z, sendROMend
	; check total bank high
	inc e
	cp e
	jr z, sendROMend
	; set bank high
	ld h, $30
	ld [hl], e
setbanklow:
	; 21 to cover mbc2
	ld h, $21
	ld [hl], d
	; 40 to cover mbc1
	ld h, $40
	ld a, d
	swap a
	srl a
	and 3
	ld [hl], a
	; save current bank
	push de
	; should have set everything
	; read out next 16k bank
	ld c, $3F
	ld de, vollut
	; hl is $4000
	call sendbank
	; restore current bank
	pop de
	jr sendROMloop
sendROMend:
	ret

sendSave:
	; RAM mode for mbc1
	ld a, $1
	ld [$6000], a
	; set sending status
	ld hl, sendmsg
	call drawStatus
	; init bank total
	; "default" 8k values
	ld b, $1 ; banks
	ld c, $1F ; and val
	; read Cart Type
	ld a, [$147]
	cp $6
	jr nz, checkRAMsize
	; 512b
	ld c, $1 ; and val
	jr writeSaveBanks
checkRAMsize:
	ld a, [$149]
	cp $1
	jr nz, check32k
	; 2kb
	ld c, $7 ; and val
	jr writeSaveBanks
check32k:
	cp $3
	jr nz, check64k
	; 32kb
	ld b, $4 ; banks
	jr writeSaveBanks
check64k:
	cp $5
	jr nz, check128k
	; 64kb
	ld b, $8 ; banks
	jr writeSaveBanks
check128k:
	cp $4
	jr nz, writeSaveBanks
	; 128k
	ld b, $10 ; banks
writeSaveBanks:
	; save RAM Banks
	ld a, b
	ld [banktotal], a
	; save and val
	ld a, c
	ld [banktotal+1], a
	; set sending status
	ld hl, sendmsg
	call drawStatus
	; done initializing bank total
	ld de, vollut
	call testsend
	; send "save" file status
	ld a, 5
	call sendnibble
	; enable RAM
	ld a, $A
	ld [$0000], a
	; init RAM bank
	ld d, 0
	; load and val
	ld a, [banktotal+1]
	ld e, a
sendSaveLoop:
	; set RAM bank
	ld a, d
	ld [$4000], a
	; save current bank
	push de
	; send RAM bank
	ld c, e
	ld de, vollut
	ld hl, $A000
	call sendbank
	; restore current bank
	pop de
	; check next bank
	inc d
	ld a, [banktotal]
	cp d
	jr nz, sendSaveLoop
	; done sending save
	; disable RAM
	xor a
	ld [$0000], a
	ret

testsend:
	; send calibration values
	ld a, $F
	call sendnibble
	ld a, $A
	call sendnibble
	ld a, 5
	call sendnibble
	ld a, 0
	call sendnibble
	ret

sendbank:
	ld a, [hl+]
	ld b, a
	call sendnibble
	ld a, b
	swap a
	call sendnibble
	; check if l is 0
	inc l
	dec l
	jr nz, sendbank
	; check if bank is processed
	ld a, h
	and c
	jr nz, sendbank
	ret

sendnibble:
	push hl
	;enable wave with
	;master volume of lut
	and $F
	ld l, a
	ld h, 0
	add hl, de
	;set master volume
	ld a, [hl]
	ldh [$FF24], a
	;allow wav volume
	ld a, $44
	ldh [$FF25], a
	;delay for audio recording
	ld a, $14
	call delayloop
	;disallow wav volume
	xor a
	ldh [$FF25], a
	;delay for audio recording
	ld a, $14
	call delayloop
	pop hl
	ret

delayloop:
	dec a
	jr nz, delayloop
	ret

vollut:
	DB $11,$13,$15,$17,$31,$33,$35,$37,$51,$53,$55,$57,$71,$73,$75,$77

banklut:
	DW $0002,$0004,$0008,$0010,$0020,$0040,$0080,$0100,$0200,$0048,$0050,$0060,$0002,$0002,$0002,$0002

banktotal:
	DW $0000

; video stuff
waitVBlank:
	ldh a, [$FF44]
	cp 145
	jr nz, waitVBlank
	ret

clearScreen:
	ld hl, $9800
	ld a, $7F
clrloop:
	ld [hl+], a
	bit 2, h
	jr z, clrloop
	ret

drawLine:
	ld a, [hl+]
	cp 0
	ret z
	ld [bc], a
	inc c
	jr drawLine

drawHeader:
	ld bc, $9821
	ld hl, hdrmsg
	call drawLine
	ld c, $41
	call drawLine
	ret

drawStatus:
	call waitVBlank
	ld bc, $9901
	call drawLine
	ld c, $21
	call drawLine
	ld c, $41
	call drawLine
	ret

hdrmsg:
	db "GB Audio Dumper", $00
	db "v0.2 by FIX94", $00

startmsg:
	db "Insert Cartridge,", $00
	db "then press START ", $00
	db "to check the Cart", $00

okmsg:
	db "Cart OK! Press   ", $00
	db "A to send ROM or ", $00
	db "B to send Save   ", $00

errmsg:
	db "Cart dirty? Press", $00
	db "A to return to   ", $00
	db "the first Screen ", $00

sendmsg:
	db "Sending, this    ", $00
	db "process will take", $00
	db "a while...       ", $00

donemsg:
	db "Done! Press A    ", $00
	db "to return to the ", $00
	db "first Screen     ", $00

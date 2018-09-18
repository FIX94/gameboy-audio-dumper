; Copyright (C) 2018 FIX94
;
; This software may be modified and distributed under the terms
; of the MIT license.  See the LICENSE file for details.

SECTION "WRAM",ROM0[$DA84]

init:
	di
	ld a, 0
	ldh [$FF26], a
	ld a, $80
	ldh [$FF26], a
	ld a, $F0
	ldh [$FF13], a
	ldh [$FF12], a
	ld a, $80
	ldh [$FF11], a
	ld a, $87
	ldh [$FF14], a
	ld a, $10
	ldh [$FF00], a
waitpress:
	ldh a, [$FF00]
	and 8
	jr nz, waitpress
regprep:
	ld hl, $148
	ld a, [hl]
	ld b,a
	;check high nibble
	and $F0
	cp $50
	jr z, addh
	;size value not 5X
	ld a,b
	jr getbanktotal
addh:
	;add 7 to low nibble to
	;get to special bank lut
	ld a,b
	and 7
	add 7
getbanktotal:
	ld hl, banklut
	and $F
	sla a
	add a,l
	ld l,a
	ld a,[hl+]
	ld [banktotal], a
	ld a,[hl]
	ld [banktotal+1], a
	ld hl, 0
	ld c, 0
testsend:
	ld a, $F
	call sendnibble
	ld a, $A
	call sendnibble
	ld a, 5
	call sendnibble
	ld a, 0
	call sendnibble
loop:
	ld a, [hl+]
	ld b, a
	call sendnibble
	ld a,b
	swap a
	call sendnibble
	;check if l is 0
	inc l
	dec l
	jr nz, loop
	;check if bank is processed
	ld a, h
	and $3F
	jr nz, loop
	;check total bank low
	inc c
	ld a, [banktotal]
	cp c
	jr nz, setbanklow
	;check if total high exists
	ld a, [banktotal+1]
	jr z, end
	ld b, a
	;check total bank high
	ld a, [curbankhigh]
	inc a
	cp b
	jr z, end
	;set bank high
	ld [curbankhigh], a
	ld h, $30
	ld [hl], a
setbanklow:
	;21 to cover mbc2
	ld h, $21
	ld [hl],c
	;40 to cover mbc1
	ld h, $40
	ld a,c
	swap a
	srl a
	and 3
	ld [hl],a
	;should have set everything
	;read out next 16k bank
	jr loop
end:
	jr end

sendnibble:
	;enable square with
	;volume of lut
	and $F
	ld de, vollut
	add a,e
	ld e,a
	ld a, $87
	ldh [$FF14], a
	ld a, $11
	ldh [$FF25], a
	ld a, [de]
	ldh [$FF24], a
	call delayloop
	;disable square
	ld a, 0
	ldh [$FF24], a
	ldh [$FF25], a
	call delayloop
	ret

delayloop:
	ld a, $1E
delaydec:
	dec a
	jr nz, delaydec
	ret

vollut:
	DB $11,$13,$15,$17,$31,$33,$35,$37,$51,$53,$55,$57,$71,$73,$75,$77

banklut:
	DW $0002,$0004,$0008,$0010,$0020,$0040,$0080,$0100,$0200,$0048,$0050,$0060,$0002,$0002,$0002,$0002

curbankhigh:
	DB $00

banktotal:
	DW $0000

// VGAmini2.s provided with Lab1 in TDT4258 autumn 2025
// 320 (width) x 240 (height) pixels, 
// 2 bytes per pixel RGB format, 1024 bytes is used to store one row
.global _start
.equ VGAaddress, 0xc8000000 // Memory storing pixels in CPUlator
setPixel: // assumes R0 = x-coordinate, R1 = y-coord, R2 = color value
	PUSH {R4-R6,LR}
    LSL R4, R1, #10 // y-address shift left 10 bits (1024 bytes)
    LSL R5, R0, #1  // x-address  shift left 1 bit (2 bytes)
    ADD R4, R5 // R4 is now correct offset
    LDR R6, =VGAaddress	
    STRH R2, [R6,R4]  // Store half-word, lower 16 bits at address R6 + offset R4
    POP {R4-R6,LR}
	BX LR
// -------------------------------------------------------------------------------
// 0x00000000 is Black, 0x0000ffff is White	
_start: // some code demonstrating SetPixel (x, y, c)
	MOV R4, #0 // R4 stores x-coordinate
	MOV R5, #0 // R5 stores y-coordinate
	MOV R0, R4 // prep param passing
	MOV R1, R5 // for x and y 
    LDR R2, =0x000000ff	// Blue
    BL setPixel
	ADD R4, R4, #1
    MOV R0, R4 // prep param passing
	MOV R1, R5 // for x and y 
    LDR R2, =0x00000f0f	// Green
	BL setPixel
	MOV R4, #0
	ADD R5, R5, #1
	MOV R0, R4 // prep param passing
	MOV R1, R5 // for x and y 
    LDR R2, =0x0000ff00	// yellow
	BL setPixel
	ADD R4, R4, #1
	MOV R0, R4 // prep param passing
	MOV R1, R5 // for x and y 
    LDR R2, =0x0000f0f0	// red
	BL setPixel
	b .

.global _start

I .req r0
Input .req r1
Char .req r2
Length .req r3
Char2 .req r4
J .req r5

char_to_lower:
	cmp Char, #'A'
	blt not_upper
	cmp Char, #'Z'
	bgt not_upper

	add Char, Char, #32
	strb Char, [Input, I]
not_upper:
	bx lr

/* Update the LED status */
update_leds:
	ldr r1, =led_address
	ldr r1, [r1]
	str r0, [r1]
	bx lr

Uart .req r1
print_to_uart:
	ldr Uart, =uart_address
	ldr Uart, [Uart]
print_to_uart_loop:
	ldrb Char, [r0]
	str Char, [Uart]
	add r0, r0, #1
	cmp Char, #0
	bne print_to_uart_loop
	bx lr
.unreq Uart

_start:
	mov I, #0 // int I = 0;
	ldr Input, =input // char *Input = input;

compute_length_and_to_lower:
	ldrb Char, [Input, I] // Char = Input[I];
	bl char_to_lower // Char = toLower(Char);
	add I, I, #1 // I++;
	cmp Char, #0
	beq end_of_string // if (Char == '\0') goto end_of_string;
	b compute_length_and_to_lower // goto compute_length_and_to_lower;

end_of_string:
	cmp I, #2 // if (Length <= 2) goto is_palindrome;
	ble _exit

check_palindrome:
	mov Length, I // Length = I;	
	mov I, #0 // I = 0;

	mov J, Length
	sub J, J, #2
check_loop:
	ldrb Char, [Input, I] // Char = Input[I];
	cmp Char, #'#'
	beq wildcard
	cmp Char, #'?'
	beq wildcard

	ldrb Char2, [Input, J] // Char2 = Input[J];
	cmp Char2, #'#'
	beq wildcard
	cmp Char2, #'?'
	beq wildcard

	cmp Char, Char2
	bne is_no_palindrome // if (Char != Char2) goto is_no_palindrome;
wildcard:
	cmp I, J // if (I >= J) goto is_palindrome;
	bge is_palindrome
	add I, I, #1 // I++;
	sub J, J, #1 // J--;
	b check_loop // goto check_loop;

is_palindrome:
	// Switch on only the 5 rightmost LEDs
	// Write 'Palindrom detected' to UART
	mov r0, #0b11111
	bl update_leds

	ldr r0, =palindrome
	bl	print_to_uart
	
	b _exit

is_no_palindrome:
	// Switch on only the 5 leftmost LEDs
	// Write 'Not a palindrom' to UART
	mov r0, #0b1111100000
	bl update_leds

	ldr r0, =no_palindrome
	bl print_to_uart

	b _exit
	
_exit:
	// Branch here for exit
	b .
	
.data
led_address: .word 0xff200000
uart_address: .word 0xff201000

.align
	// This is the input you are supposed to check for a palindrom
	// You can modify the string during development, however you
	// are not allowed to change the name 'input'!
	input: .asciz "Grav ned den varg"
	palindrome: .asciz "Palindrom detected"
	no_palindrome: .asciz "Not a palindrom"
.end

// palinfinder.s, provided with Lab1 in TDT4258 autumn 2025
.global _start

I .req r0 // int I;
Input .req r1 // char *Input;
Char .req r2 // char Char;
Length .req r3 // int Length = I;	
Char2 .req r4 // char Char2;
J .req r5 // int J = Length - I - 1;

char_to_lower:
	cmp Char, #'A' // 'A'
	blt not_upper
	cmp Char, #'Z' // 'Z'
	bgt not_upper

	add Char, Char, #32 // char + 32
	strb Char, [Input, I]
not_upper:
	bx lr

_start:
	mov I, #0 // int I = 0;
	ldr Input, =input // char *Input = input;

/* Validate input length */
	ldrb Char, [Input, I] // Char = Input[I];
	cmp Char, #0
	beq _exit // if (Char == 0) return 0;
	bl char_to_lower // Char = tolower(Char);
	add I, I, #1 // I++;

	ldrb Char, [Input, I] // Char = Input[I];
	cmp Char, #0
	beq _exit // if (Char == 0) return 0;
	bl char_to_lower // Char = tolower(Char);
	add I, I, #1 // I++;
/* Input length longer than 2 */

compute_length:
	ldrb Char, [Input, I] // Char = Input[I];
	bl char_to_lower // Char = tolower(Char);
	add I, I, #1 // I++;
	cmp Char, #0
	beq check_palindrome // if (Char == '\0') goto check_palindrome;
	b compute_length // goto compute_length;

check_palindrome:
	mov Length, I // Length = I;	
	mov I, #0 // I = 0;

	mov J, Length
	sub J, J, #2
check_loop:
	ldrb Char, [Input, I] // Char = Input[I];
	ldrb Char2, [Input, J] // Char2 = Input[J];
	cmp Char, Char2
	bne is_no_palindrome // if (Char != Char2) goto is_no_palindrome;
	cmp I, J // if (I >= J) goto is_palindrome;
	bge is_palindrome
	add I, I, #1 // I++;
	sub J, J, #1 // J--;
	b check_loop // goto check_loop;

is_palindrome:
	// Switch on only the 5 rightmost LEDs
	// Write 'Palindrom detected' to UART
	// r0 = led address
	ldr r0, =led_address
	ldr r0, [r0]
	mov r1, #0b11111
	str r1, [r0]
	// r0 = uart address
	// r1 = address of character
	// r2 = character
	ldr r0, =uart_address
	ldr r0, [r0]
	ldr r1, =palindrome
	continue_printing_is_palindrom:
		ldrb r2, [r1]
		cmp r2, #0
		beq _exit
		
		str r2, [r0]
		add r1, r1, #1
		b continue_printing_is_palindrom
	
	b _exit

is_no_palindrome:
	// Switch on only the 5 leftmost LEDs
	// Write 'Not a palindrom' to UART
	// r0 = led address
	// r1 = led status
	ldr r0, =led_address
	ldr r0, [r0]
	mov r1, #0b1111100000
	str r1, [r0]
	// r0 = uart address
	// r1 = address of character
	// r2 = character
	ldr r0, =uart_address
	ldr r0, [r0]
	ldr r1, =no_palindrome
	continue_printing_is_no_palindrom:
		ldrb r2, [r1]
		cmp r2, #0
		beq _exit
		
		str r2, [r0]
		add r1, r1, #1
		b continue_printing_is_no_palindrom

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

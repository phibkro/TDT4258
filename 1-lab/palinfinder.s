// palinfinder.s, provided with Lab1 in TDT4258 autumn 2025
.global _start


// Please keep the _start method and the input strings name ("input") as
// specified below
// For the rest, you are free to add and remove functions as you like,
// just make sure your code is clear, concise and well documented.

_start:
	// Here your execution starts
	b _exit

	
check_input:
	// You could use this symbol to check for your input length
	// you can assume that your input string is at least 2 characters 
	// long and ends with a null byte
	
	
check_palindrom:
	// Here you could check whether input is a palindrom or not
	
	
is_palindrom:
	// Switch on only the 5 rightmost LEDs
	// Write 'Palindrom detected' to UART
	
	
is_no_palindrom:
	// Switch on only the 5 leftmost LEDs
	// Write 'Not a palindrom' to UART
	
	
_exit:
	// Branch here for exit
	b .
	
.data
.align
	// This is the input you are supposed to check for a palindrom
	// You can modify the string during development, however you
	// are not allowed to change the name 'input'!
	input: .asciz "Grav ned den varg"
.end

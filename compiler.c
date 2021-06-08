/*
   This program is provided under the LGPL license ver 2.1
   KM-BASIC for ARM, written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   https://github.com/kmorimatsu
*/

#include <string.h>
#include "./compiler.h"

void init_compiler(void){
	// Prepare execution
	// 1) Push return address reserved in lr register
	// 2) Store stack pointer for returning to C code
	// See also end_statement() code
	kmbasic_object[0]=0xb500;//      	push	{lr}
	kmbasic_object[1]=0x4668;//      	mov	r0, sp
	kmbasic_object[2]=0x6038;//      	str	r0, [r7, #0]
	object=&kmbasic_object[3];
}

void run_code(void){
	// Push r0-r12
	asm("push {lr}");
	asm("push {r0,r1,r2,r3,r4,r5,r6,r7}");
	asm("mov r0,r8");
	asm("mov r1,r9");
	asm("mov r2,r10");
	asm("mov r3,r11");
	asm("mov r4,r12");
	asm("push {r0,r1,r2,r3,r4}");
	// Set special registers
	// R5 is pointer to array containing variable values
	asm("ldr r5,=kmbasic_variables");
	// R7 is pointer to array containing various data
	asm("ldr r7,=kmbasic_data");
	// R8 is pointer to library function
	asm("ldr r0,=kmbasic_library");
	asm("mov r8,r0");
	// Call kmbasic_object
	asm("ldr r1,=kmbasic_object+1");
	asm("blx r1");
	// Pop r0-r12
	asm("pop {r0,r1,r2,r3,r4}");
	asm("mov r8,r0");
	asm("mov r9,r1");
	asm("mov r10,r2");
	asm("mov r11,r3");
	asm("mov r12,r4");
	asm("pop {r0,r1,r2,r3,r4,r5,r6,r7}");
	asm("pop {pc}");
}

int call_lib_code(int lib_number){
	if (255<lib_number) return ERROR_UNKNOWN;
	check_object(2);
	(object++)[0]=0x2300 | lib_number;// movs	r3, #1
	(object++)[0]=0x47c0;             // blx	r8
	return 0;
}

int set_value_in_register(unsigned char r,int val){
	if (7<r) return ERROR_UNKNOWN;
	if (0<=val && val<=255) {
		val+=(r<<8);
		check_object(1);
		(object++)[0]=0x2000 | val | (r<<8);      // movs	r1, #xx
		return 0;
	} else {
		// TODO: here
		return ERROR_UNKNOWN;
	}
}

int instruction_is(unsigned char* instruction){
	int n;
	// Skip blank first
	skip_blank();
	// Count number
	while(instruction[n]) n++;
	// Compare strings
	if (strncmp(source,instruction,n)) return 0;
	// If this is function, everything is alright.
	if (instruction[n-1]=='(') {
		source+=n;
		skip_blank();
		return 1;
	}
	// Next code must not be character for statement
	switch(source[n]){
		case 0x20:
		case 0x00:
		case ':':
			source+=n;
			skip_blank();
			return 1;
		default:
			// Instructin didn't match
			return 0;
	}
}

int compile_statement(){
	int e;
	// Initialize
	unsigned short* bobj=object;
	unsigned char* bsrc=source;
	g_sdepth=0;
	g_allow_shift_obj=1;
	// Check LET statement, first
	if (instruction_is("LET")) return let_statement();
	// "LET" may be omitted.
	e=let_statement();
	if (!e) return 0;
	object=bobj;
	source=bsrc;
	// It's not LET statement. Let's continue for possibilities of the other statements.
	if (instruction_is("PRINT")) return print_statement();
	if (instruction_is("END")) return end_statement();
	// Finally, try let statement again as syntax error may be in LET statement.
	return let_statement();
}

/*
	Change all lower case charaters to upper ones
	Change tabs to spaces
	Change all enter characters to null
	Use kmbasic_variables[256] area for the buffer as this area isn't used when compilling
*/
unsigned char* code2upper(unsigned char* code){
	int i;
	unsigned char c;
	unsigned char* result=(unsigned char*)&kmbasic_variables[0];
	char instring=0;
	for(i=0;i<1024;i++){
		c=code[i];
		if (instring) {
			if ('"'==c) instring=0;
		} else {
			if ('a'<=c && c<='z') {
				c=c-0x20;
			} else if ('"'==c) {
				instring=1;
			} else if (0x09==c) {
				c=0x20;
			} else if (0x0d==c || 0x0a==c) {
				result[i]=0;
				break;
			} else if (1023==i) {
				c=0x00;
			}
		}
		result[i]=c;
	}
	return result;
}

// This function returns number of bytes being compiled.
// If an error occured. this returns negative value as error code.
int compile_line(unsigned char* code){
	int e;
	unsigned char* before;
	before=source=code2upper(code);
	while(1){
		e=compile_statement();
		if (e) return e; // An error occured
		// Skip blank
		skip_blank();
		// Check null as the end of line
		if (source[0]==0x00) break;
		// Check ':'
		if (source[0]!=':') return ERROR_SYNTAX;
		// Continue this  line
		source++;
	}
	// Error didn't happen
	e=source-before;
	// End of string is null
	if (0x00==code[e]) return e;
	// End of string is lf
	if (0x0a==code[e]) return e+1;
	// End of string is cr or crlf
	return 0x0a==code[e+1] ? e+2:e+1;
}
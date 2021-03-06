/*
   This program is provided under the LGPL license ver 2.1
   KM-BASIC for ARM, written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   https://github.com/kmorimatsu
*/

#include "./compiler.h"

/*
	Dedicate followings for easy writing of compiler codes
*/
unsigned char* source;
unsigned short* object;

/*
	KM-BASIC related global areas
*/

unsigned short kmbasic_object[512*192]; // 192K bytes RAM area
int kmbasic_data[32];
int kmbasic_variables[ALLOC_BLOCK_NUM];
unsigned short kmbasic_var_size[ALLOC_BLOCK_NUM];

unsigned short* g_objmax;

// kmbasic_data[] area is also used for temporary region when compiling
int* g_default_args=(int*)&kmbasic_data[-1];      // Use 6 words; g_default_args[1] - g_default_args[6]
void** g_callback_args=(void**)&kmbasic_data[-1]; // Use 6 functions; g_callback_args[1] - g_callback_args[6]
// (26 words remaining)

// kmbasic_variables[] is also used for file buffer and compiling buffer
unsigned char* g_file_buffer=(unsigned char*)&kmbasic_variables[0];
unsigned char* g_compile_buffer=(unsigned char*)&kmbasic_variables[ALLOC_BLOCK_NUM/2];
const int g_file_buffer_size=ALLOC_BLOCK_NUM*2;

// Line number
int g_linenum;

// Function to call when multipe line statement
void* g_multiple_statement;

// Variable number used when compiling
int g_next_varnum;

// Depth of stack used for calculation
int g_sdepth;
int g_maxsdepth;

// Depth of if/endif and for/do/while
short g_ifdepth;
short g_fordepth;

// Scratch variable
volatile char g_scratch[32];
volatile int* g_scratch_int=(volatile int*)&g_scratch[0];
volatile float* g_scratch_float=(volatile float*)&g_scratch[0];

// Constant values
char g_constant_value_flag;
int g_constant_int;
float g_constant_float;

// Memory allocation
char g_garbage_collection;

// Random seed
int g_rnd_seed;

// Last var number used for memory allocation
int g_last_var_num;

// Reading point used for READ() and CREAD() and reading mode
unsigned short* g_read_point;
int g_read_valid_len;
unsigned short g_read_mode;

// Reserved words



const char* const g_reserved_words[129]={
	"ABS",
	"ACOS",
	"ARGS",
	"ASC",
	"ASIN",
	"ATAN",
	"ATAN2",
	"BGCOLOR",
	"BOXFILL",
	"BREAK",
	"CALL",
	"CDATA",
	"CEIL",
	"CHR",
	"CIRCLE",
	"CIRCLEFILL",
	"CLEAR",
	"CLS",
	"COLOR",
	"CONTINUE",
	"COS",
	"COSH",
	"CREAD",
	"CURSOR",
	"DATA",
	"DEBUG",
	"DEC",
	"DELETE",
	"DIM",
	"DO",
	"DRAWCOUNT",
	"ELSE",
	"ELSEIF",
	"END",
	"ENDIF",
	"EXEC",
	"EXP",
	"FABS",
	"FCLOSE",
	"FEOF",
	"FGET",
	"FGETC",
	"FIELD",
	"FILE",
	"FINPUT",
	"FLEN",
	"FLOAT",
	"FLOOR",
	"FMOD",
	"FOPEN",
	"FOR",
	"FPRINT",
	"FPUT",
	"FPUTC",
	"FSEEK",
	"GCLS",
	"GCOLOR",
	"GETDIR",
	"GOSUB",
	"GOTO",
	"GPALETTE",
	"GPRINT",
	"HEX",
	"IDLE",
	"IF",
	"INKEY",
	"INPUT",
	"INT",
	"KEYS",
	"LABEL",
	"LEN",
	"LET",
	"LINE",
	"LOG",
	"LOG10",
	"LOOP",
	"MODF",
	"MUSIC",
	"NEW",
	"NEXT",
	"NOT",
	"OPTION",
	"PALETTE",
	"PCG",
	"PEEK",
	"PEEK16",
	"PEEK32",
	"PI",
	"POINT",
	"POKE",
	"POKE16",
	"POKE32",
	"POW",
	"PRINT",
	"PSET",
	"PUBLIC",
	"PUTBMP",
	"READ",
	"REM",
	"RESTORE",
	"RETURN",
	"RND",
	"SCROLL",
	"SETDIR",
	"SGN",
	"SIN",
	"SINH",
	"SOUND",
	"SPRINTF",
	"SQRT",
	"STEP",
	"STRNCMP",
	"SYSTEM",
	"TAN",
	"TANH",
	"THEN",
	"TIMER",
	"TO",
	"TVRAM",
	"UNTIL",
	"USEGRAPHIC",
	"USEPCG",
	"USEVAR",
	"VAL",
	"VAR",
	"WAIT",
	"WEND",
	"WHILE",
	"WIDTH",
};
const int const g_hash_resereved_words[129]={
	0x000400d3, //ABS
	0x01002393, //ACOS
	0x01013193, //ARGS
	0x00040483, //ASC
	0x0101220e, //ASIN
	0x0101500e, //ATAN
	0x405403b2, //ATAN2
	0x7dc72863, //BGCOLOR
	0x66e378bf, //BOXFILL
	0x434c400b, //BREAK
	0x0108034c, //CALL
	0x42140541, //CDATA
	0x0108420c, //CEIL
	0x00042252, //CHR
	0x884c2355, //CIRCLE
	0xab94cf50, //CIRCLEFILL
	0x42344012, //CLEAR
	0x00042353, //CLS
	0x4238d392, //COLOR
	0x2ade0dd9, //CONTINUE
	0x00042393, //COS
	0x0108e488, //COSH
	0x424c4004, //CREAD
	0x944d2382, //CURSOR
	0x01140541, //DATA
	0x45103507, //DEBUG
	0x00045103, //DEC
	0x44344554, //DELETE
	0x0004520d, //DIM
	0x0000114f, //DO
	0x827c002e, //DRAWCOUNT
	0x0110d485, //ELSE
	0x0d484217, //ELSEIF
	0x000443c4, //END
	0x443c5206, //ENDIF
	0x01119103, //EXEC
	0x00044650, //EXP
	0x011c00d3, //FABS
	0xc234e494, //FCLOSE
	0x011c4386, //FEOF
	0x011c6114, //FGET
	0x47184543, //FGETC
	0x47204344, //FIELD
	0x011c8345, //FILE
	0xc83d1505, //FINPUT
	0x011cd10e, //FLEN
	0x4734e014, //FLOAT
	0x4734e392, //FLOOR
	0x011cc384, //FMOD
	0x4739110e, //FOPEN
	0x00047392, //FOR
	0xd14c83c5, //FPRINT
	0x011d1514, //FPUT
	0x47454543, //FPUTC
	0x4748410b, //FSEEK
	0x01182353, //GCLS
	0x8238d383, //GCOLOR
	0x84545203, //GETDIR
	0x46392502, //GOSUB
	0x0118e54f, //GOTO
	0x0d1043aa, //GPALETTE
	0x914c83c5, //GPRINT
	0x00049118, //HEX
	0x01205345, //IDLE
	0x00001206, //IF
	0x483ca119, //INKEY
	0x483d1514, //INPUT
	0x000483d4, //INT
	0x01284613, //KEYS
	0x4d00310c, //LABEL
	0x0004d10e, //LEN
	0x0004d114, //LET
	0x013483c5, //LINE
	0x0004d387, //LOG
	0x4d387c70, //LOG10
	0x0134e390, //LOOP
	0x0130e146, //MODF
	0x4c512203, //MUSIC
	0x0004f117, //NEW
	0x013c4654, //NEXT
	0x0004f394, //NOT
	0x9154839d, //OPTION
	0x0d115055, //PALETTE
	0x00051087, //PCG
	0x0144410b, //PEEK
	0x4410bc62, //PEEK16
	0x4410bce6, //PEEK32
	0x00001449, //PI
	0x513883d4, //POINT
	0x0144e285, //POKE
	0x4e285c62, //POKE16
	0x4e285ce6, //POKE32
	0x00051397, //POW
	0x514c83d4, //PRINT
	0x01452114, //PSET
	0x540cd217, //PUBLIC
	0x54543304, //PUTBMP
	0x014c4004, //READ
	0x0005310d, //REM
	0xedab1e34, //RESTORE
	0xc45544da, //RETURN
	0x000533c4, //RND
	0x824ce358, //SCROLL
	0x84545206, //SETDIR
	0x0005218e, //SGN
	0x0005220e, //SIN
	0x014883c8, //SINH
	0x523943c4, //SOUND
	0xacdf0fa2, //SPRINTF
	0x014904d4, //SQRT
	0x01495110, //STEP
	0xacc3d9f5, //STRNCMP
	0x98495119, //SYSTEM
	0x0005500e, //TAN
	0x015403c8, //TANH
	0x0154910e, //THEN
	0x5520c112, //TIMER
	0x0000154f, //TO
	0x555d300d, //TVRAM
	0x543d520c, //UNTIL
	0x2aaa8267, //USEGRAPHIC
	0x12111092, //USEPCG
	0x12117007, //USEVAR
	0x0005700c, //VAL
	0x00057012, //VAR
	0x01580214, //WAIT
	0x015843c4, //WEND
	0x56248345, //WHILE
	0x56205548, //WIDTH
};


/*
   This program is provided under the LGPL license ver 2.1
   KM-BASIC for ARM, written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   https://github.com/kmorimatsu
*/

#include "./compiler.h"

/*
	CMPDATA_VARNAME structure
		type:      CMPDATA_VARNAME
		len:       n+1
		data16:    var number
		record[1]: hash
		record[2]: string
		record[3]: string (continued)
		...
		record[n]: end of string
*/

static int g_next_varnum;

void variable_init(void){
	g_next_varnum=26;
}

short get_new_varnum(void){
	if (g_next_varnum<256-TEMPVAR_NUMBER) return g_next_varnum++;
	return 0;
}

int get_var_number(void){
	int num;
	int* data;
	// Only support A-Z now
	if (source[0]<'A' || 'Z'<source[0]) return ERROR_SYNTAX;
	if ('A'<=source[1] && source[1]<='Z' || '_'==source[1] || '0'<=source[1] && source[1]<='9') {
		// Long name
		for(num=2;'A'<=source[num] && source[num]<='Z' || '_'==source[num] || '0'<=source[num] && source[num]<='9';num++);
		data=cmpdata_nsearch_string_first(CMPDATA_VARNAME,source,num);
		if (!data) return ERROR_SYNTAX;
		source+=num;
		return data[0]&0x0000ffff;
	}
	// A-Z (short name)
	return (source++)[0]-'A';
}

int var_num_to_r1(int vn){
	if (vn<256) {
		check_object(1);
		(object++)[0]=0x2100 | vn;      // movs	r1, #xx
	} else return ERROR_UNKNOWN;
}
int r0_to_variable(int vn){
	int e;
	if (vn<32) {
		check_object(1);
		(object++)[0]=0x6028 | (vn<<6); // str	r0, [r5, #xx]
		return 0;
	} else if (vn<256) {
		e=var_num_to_r1(vn);
		if (e) return e;
		check_object(1);
		(object++)[0]=0x5068;           // str	r0, [r5, r1]
		return 0;
	} else return ERROR_UNKNOWN;
}
int variable_to_r0(int vn){
	int e;
	if (vn<32) {
		check_object(1);
		(object++)[0]=0x6828 | (vn<<6); // ldr	r0, [r5, #xx]
		return 0;
	} else if (vn<256) {
		e=var_num_to_r1(vn);
		if (e) return e;
		check_object(1);
		(object++)[0]=0x5868;           // ldr	r0, [r5, r1]
		return 0;
	} else return ERROR_UNKNOWN;
}

#ifndef KODI_H
#define KODI_H

#ifdef _WIN32
#include <direct.h>
// MSDN recommends against using getcwd & chdir names
#define cwd _getcwd
#define cd _chdir
#else
#include "unistd.h"
#define cwd getcwd
#define cd chdir
#endif

#define SV_IMPLEMENTATION
#include"sv.h"

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<errno.h>
#include<unistd.h> 

#define KB_SEGMENTS_MAX_COUNT 12
#define KB_TABLE_MAX_COUNT 6

void reverse(char str[], int length);
char* citoa(int num, char* str, int base);
String_View slurp_file(const char* file_path);

typedef enum {
	NONE,
	INT,
	VARCHAR,
}Data_Type;

typedef struct {
	void* value;
	Data_Type type;
	size_t size;
}Data;

typedef struct {
	String_View selected_record;
	String_View selected_form_name;
}Base;

Base base = { 0 };

void create_table(String_View src);
void create_record_table(String_View src);
void select_item_from_table(String_View src, Base* base);
void translate_script_to_binary(String_View src, Base* base);
void set_cstr_from_sv(char* str, String_View sv, int blank);

#endif

#ifndef KODI_IMPLEMENTATION

void set_cstr_from_sv(char* str, String_View sv,int blank)
{
	for (size_t i = blank; i < sv.count + blank; i++)
	{
		str[i] = sv.data[i - blank];
	}
}

void reverse(char str[], int length)
{
	int start = 0;
	int end = length - 1;
	while (start < end) {
		char temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		end--;
		start++;
	}
}
// Implementation of citoa()
char* citoa(int num, char* str, int base)
{
	int i = 0;
	bool isNegative = false;

	/* Handle 0 explicitly, otherwise empty string is
	 * printed for 0 */
	if (num == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	// In standard itoa(), negative numbers are handled
	// only with base 10. Otherwise numbers are
	// considered unsigned.
	if (num < 0 && base == 10) {
		isNegative = true;
		num = -num;
	}

	// Process individual digits
	while (num != 0) {
		int rem = num % base;
		str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		num = num / base;
	}

	// If number is negative, append '-'
	if (isNegative)
		str[i++] = '-';

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	reverse(str, i);

	return str;
}

String_View slurp_file(const char* file_path)
{
	FILE* f = fopen(file_path, "r");
	if (f == NULL) {
		fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
			file_path, strerror(errno));
		exit(1);
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
			file_path, strerror(errno));
		exit(1);
	}

	long m = ftell(f);
	if (m < 0) {
		fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
			file_path, strerror(errno));
		exit(1);
	}

	char* buffer = (char*)malloc(m);
	if (buffer == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for file: %s\n",
			strerror(errno));
		exit(1);
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
			file_path, strerror(errno));
		exit(1);
	}

	size_t n = fread(buffer, 1, m, f);
	if (ferror(f)) {
		fprintf(stderr, "ERROR: Could not read file `%s`: %s\n",
			file_path, strerror(errno));
		exit(1);
	}

	fclose(f);
	return (String_View) {
		.count = n,
			.data = buffer,
	};
}

void create_table(String_View src) {
	String_View table_name = sv_trim(sv_chop_by_delim(&src, '('));

	if (table_name.count > src.count) {
		fprintf(stderr, "Incorrect syntax!!!\n");
		exit(1);
	}
	else {
		chdir("data/");
		FILE* fptr;
		fptr = fopen("forms.txt", "r");

		if (fptr == NULL) {
			fptr = fopen("forms.txt", "w");

			for (size_t i = 0; i < table_name.count; i++)
			{
				fputc(table_name.data[i], fptr);
			}fputc('\n', fptr);

			fclose(fptr);
		}
		else {
			size_t cur = 0;
			char line[64];
			char ch;
			while ((ch = fgetc(fptr)) != EOF) {
				line[cur++] = ch;

				if (ch == '\n') {
					String_View name = sv_from_cstr(line);
					name.count -= 1;
					if (sv_eq(table_name, name)) {
						fprintf(stderr, "Form already exist!!!\n");
						exit(1);
					}
					memset(line, 0, sizeof line);
					cur = 0;
				}
			}

			fclose(fptr);
			fptr = fopen("forms.txt", "a");

			for (size_t i = 0; i < table_name.count; i++)
			{
				fputc(table_name.data[i], fptr);
			}fputc('\n', fptr);

			fclose(fptr);
		}
	}

	src.count -= 2;
	src.data += 2;
	src = sv_trim(src);

	char name[32] = "mkdir ";
	name[5] = ' ';
	set_cstr_from_sv(name, table_name, 6);
	chdir("data/");
	system(name);

	memset(name, 0, sizeof name);
	set_cstr_from_sv(name, table_name, 0);
	chdir(name);

	FILE* fptr;
	fptr = fopen("names.txt", "w");
	bool unique_has = false;
	while (*src.data != ')') {
		String_View member_name = sv_trim(sv_chop_by_delim(&src, ' '));

		String_View member_type = sv_trim(sv_chop_by_delim(&src, ','));
		String_View type = sv_trim(sv_chop_by_delim(&member_type, ' '));
		member_type = sv_trim(member_type);

		for (size_t i = 0; i < type.count; i++){
				fputc(type.data[i], fptr);
		}fputc(' ', fptr);

	    if (sv_eq(member_type, sv_from_cstr("UNIQUE"))) {
			if (unique_has) {
				fprintf(stderr, "Error: Cant be two unique members!!!\n");
				exit(1);
			}
			for (size_t i = 0; i < member_type.count; i++)
			{
				fputc(member_type.data[i], fptr);
			}fputc(' ', fptr);
			unique_has = true;
		}
		for (size_t i = 0; i < member_name.count; i++)
		{
			fputc(member_name.data[i], fptr);
		}fputc('\n', fptr);

		src.count -= 2;
		src.data += 2;
		src = sv_trim(src);
	}

	if (!unique_has) {
		fclose(fptr);
		fclose(fopen("names.txt", "w"));
		chdir("..");
		char line[64] = "rm -rf ";
		for (size_t i = 0; i < table_name.count; i++)
		{
			line[i + 7] = table_name.data[i];
		}
		system(line);
		system("rm -f forms.txt");
		fprintf(stderr, "Error: There is has to be at least one unique member!!!\n");
		exit(1);
	}
	else {
		fclose(fptr);
	}

	chdir("../..");
}

void create_record_table(String_View src)
{
	String_View table_name = sv_trim(sv_chop_by_delim(&src, '('));

	if (table_name.count > src.count) {
		fprintf(stderr, "Incorrect syntax!!!\n");
		exit(1);
	}
	else {
		chdir("data");
		FILE* fptr;
		fptr = fopen("forms.txt", "r");

		if (fptr == NULL) {
			fprintf(stderr, "There is no 'form' at all!!!\n");
			exit(1);
		}
		else {
			size_t cur = 0;
			size_t index = 0;
			char line[64];
			char ch;
			bool form_have = false;

			while ((ch = fgetc(fptr)) != EOF) {
				line[cur++] = ch;

				if (ch == '\n') {
					String_View name = sv_from_cstr(line);
					name.count -= 1;
					if (sv_eq(table_name, name)) {
						form_have = true;
						break;
					}
					memset(line, 0, sizeof line);
					cur = 0;
					index++;
				}
			}

			if (!form_have){
				fprintf(stderr, "There is not form like that!!!: %s\n",table_name.data);
				exit(1);
			}

			fclose(fptr);
		}
	}

	Data_Type types[12];
	size_t type_size = 0;
	char dir[32];

	set_cstr_from_sv(dir, table_name, 0);
	chdir(dir);

	String_View name = slurp_file("names.txt");
	char names[12][64];
	int unique_name = -1;
	size_t cur = 0;
	int size = 0;
	int sizes[12];

	while (name.count > 0) {
		String_View type = sv_trim(sv_chop_by_delim(&name,' '));
		if (sv_eq(type, sv_from_cstr("INT"))) {
			printf("INT\n");
			types[type_size++] = INT;
		}
		else if (sv_eq(type, sv_from_cstr("VARCHAR"))) {
			printf("VARCHAR\n");
			types[type_size++] = VARCHAR;
		}

		type = sv_trim(sv_chop_by_delim(&name, '\n'));
	    String_View name_type = sv_trim(sv_chop_by_delim(&type, ' '));
		if (sv_eq(name_type, sv_from_cstr("UNIQUE"))) {
			unique_name = size;
			type = sv_trim(type);
			for (size_t i = 0; i < type.count; i++)
			{
				names[size][i] = type.data[i];
			}
			sizes[size] = type.count;
		}
		else {
			for (size_t i = 0; i < name_type.count; i++)
			{
				names[cur][i] = name_type.data[i];
			}
			sizes[size] = name_type.count;
		}
		size++;
	}
	FILE* data_file;
	Data datas[12];

	String_View data = sv_trim(sv_chop_by_delim(&src, ')'));
	while (data.count > 0) {
		String_View value = sv_trim(sv_chop_by_delim(&data, ','));
		if (isdigit(*value.data) && types[cur] == INT) {
			datas[cur].type = INT;
			datas[cur].value = (int)sv_to_u64(value);
		}
		else if (types[cur] == VARCHAR) {		
			value.count -= 1;
			value.data += 1;
			datas[cur].type = VARCHAR;
			char* line = (char*)malloc(sizeof(char) * 64);

			for (size_t i = 0; i < value.count - 1; i++)
			{
				line[i] = value.data[i];
			}
			datas[cur].size = value.count - 1;
			datas[cur].value = line;
		}
		cur++;
	}

	char name_of_file[64];
	if (unique_name == -1) {
		fprintf(stderr, "Error: There is has to be unique member!!!\n");
		exit(1);
	}
	if (datas[unique_name].type == INT) {
		int result = (int)datas[unique_name].value;
		citoa(result, name_of_file, 10);
	}
	else if (datas[unique_name].type == VARCHAR) {
		char* line = (char*)datas[unique_name].value;
		for (size_t i = 0; i < strlen(line); i++)
		{
			name_of_file[i] = line[i];
		}
	}

	data_file = fopen(name_of_file, "w");
	size_t index = 0;
	while (index < cur) {
		if (datas[index].type == INT) {
			for (size_t i = 0; i < sizes[index]; i++)
			{
				fputc(names[index][i], data_file);
			}fputc(':', data_file);

			char line[64];
			int result = (int)datas[index].value;
			citoa(result, line, 10);
			for (size_t i = 0; i < strlen(line); i++)
			{
				fputc(line[i], data_file);
			}fputc('\n', data_file);
		}
		else if (datas[index].type == VARCHAR) {
			for (size_t i = 0; i < sizes[index]; i++)
			{
				fputc(names[index][i], data_file);
			}fputc(':', data_file);

			char* line = (char*)datas[index].value;

			for (size_t i = 0; i < strlen(line); i++)
			{
				fputc(line[i], data_file);
			}fputc('\n', data_file);
			free(datas[index].value);
		}
		index++;
	}

	fclose(data_file);

	chdir("../..");
}

void select_item_from_table(String_View src,Base *base)
{
	String_View unique = sv_trim(sv_chop_by_delim(&src,' '));
	unique.count += 1;

	if (sv_eq(unique, sv_from_cstr("UNIQUE"))) {
		fprintf(stderr, "Error: HAVE TO BE UNIQUE SYNTAX AFTER SELECT!!!\n");
		exit(1);
	}
	unique = sv_trim(sv_chop_by_delim(&src, ' '));

	String_View form_name = sv_trim(sv_chop_by_delim(&src, ' '));
	if (sv_eq(unique, sv_from_cstr("FROM"))) {
		fprintf(stderr, "Error: HAVE TO BE FROM SYNTAX AFTER UNIQUE_MEMBER!!!\n");
		exit(1);
	}
	form_name = sv_trim(sv_chop_by_delim(&src, '\n'));
     
	chdir("data/");
	FILE* fptr;
	fptr = fopen("forms.txt", "r");
	if (fptr == NULL) {
		fprintf(stderr, "Error: There is no form with that name!!!\n");
		exit(1);
	}

	size_t cur = 0;
	size_t index = 0;
	char line[64];
	char ch;
	bool form_have = false;

	while ((ch = fgetc(fptr)) != EOF) {
		line[cur++] = ch;

		if (ch == '\n') {
			String_View name = sv_from_cstr(line);
			name.count -= 1;
			if (sv_eq(form_name, name)) {
				form_have = true;
				break;
			}
			memset(line, 0, sizeof line);
			cur = 0;
			index++;
		}
	}
	if (!form_have) {
		fprintf(stderr, "There is not form like that!!!: %s\n", form_name.data);
		exit(1);
	}

	fclose(fptr);

	char dir[32];
	set_cstr_from_sv(dir, form_name, 0);
	chdir(dir);

	memset(dir, 0, sizeof dir);
	set_cstr_from_sv(dir, unique, 0);

	fptr = fopen(dir, "r");
	if (fptr == NULL) {
		fprintf(stderr, "Error: There is no record with that unqiue member!!!\n");
		exit(1);
	}

	base->selected_form_name = form_name;
	base->selected_record = unique;

	fclose(fptr);

	chdir("../..");
}

void update_segment(String_View src, Base* base)
{
	String_View segment_name = sv_trim(sv_chop_by_delim(&src, ' '));
	String_View value = sv_trim(sv_chop_by_delim(&src, '\n'));
}

void translate_script_to_binary(String_View src,Base *base)
{
	while (src.count > 0) {
		String_View line = sv_trim(sv_chop_by_delim(&src, ' '));

		if (sv_eq(line, sv_from_cstr("CREATE"))) {
			line = sv_trim(sv_chop_by_delim(&src, ' '));
			if (sv_eq(line, sv_from_cstr("TABLE"))) {
				create_table(src);
			}
			else {
				fprintf(stderr, "Error: HAVE TO BE TABLE SYNTAX AFTER CREATE!!!\n");
				exit(1);
			}
		}
		else if (sv_eq(line, sv_from_cstr("INSERT"))) {
			line = sv_trim(sv_chop_by_delim(&src, ' '));
			if (sv_eq(line, sv_from_cstr("INTO"))) {
				create_record_table(src);
			}
			else {
				fprintf(stderr, "Error: HAVE TO BE TABLE SYNTAX AFTER INSERT!!!\n");
				exit(1);
			}
		}
		else if (sv_eq(line, sv_from_cstr("SELECT"))) {
			select_item_from_table(src,base);
		}
		else if (sv_eq(line, sv_from_cstr("UPDATE"))) {
			update_segment(src,base);
		}
	}
}

#endif
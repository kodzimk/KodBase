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
#include<stdint.h>

#define KB_SEGMENTS_MAX_COUNT 12
#define KB_TABLE_MAX_COUNT 6

void reverse(char str[], int length);
char* citoa(int num, char* str, int base);
String_View slurp_file(const char* file_path);

typedef enum {
	NONE = 0,
	INT,
	VARCHAR,
}Data_Type;

typedef struct {
	String_View selected_record;
	String_View selected_form_name;
}Base;

Base base = { 0 };

void create_table(String_View *src);
void set_cstr_from_sv(char* str, String_View sv, size_t blank);
void translate_script_to_binary(String_View src);

#endif

#ifndef KODI_IMPLEMENTATION

void set_cstr_from_sv(char* str, String_View sv,size_t blank)
{
	for (size_t i = blank; i < sv.count + blank; i++)
	{
		str[i] = sv.data[i - blank];
	}
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

	char* buffer = (char*)malloc((size_t)m);
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

	size_t n = fread(buffer, 1, (size_t)m, f);
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

void create_table(String_View* src) {
	String_View table_name = sv_trim(sv_chop_by_delim(src, '('));
	cd("data");

	FILE* fptr;
	fptr = fopen("forms", "a");
	if (fptr == NULL) {
		fptr = fopen("forms", "w");
	}
	else {
		String_View names = slurp_file("forms");
		while (names.count > 0) {
			String_View name = sv_trim(sv_chop_by_delim(&names, '\n'));
			if (sv_eq(name, table_name)) {
				fprintf(stderr, "Error: There was a form with name: %s\n", name.data);
				fclose(fptr);
				exit(1);
			}
		}
	}

	fwrite(table_name.data, sizeof(char), table_name.count, fptr);
	fprintf(fptr, "\n");
	fclose(fptr);

	char dir[64] = "mkdir ";
	set_cstr_from_sv(dir, table_name, strlen(dir));
	system(dir);
	memset(dir, 0, sizeof dir);
	set_cstr_from_sv(dir, table_name, strlen(dir));
	cd(dir);

	fptr = fopen("names", "w");
	String_View members = sv_trim(sv_chop_by_delim(src, ')'));
	while (members.count > 0) {
		String_View member = sv_trim(sv_chop_by_delim(&members, ','));
		fwrite(member.data, sizeof(char), member.count, fptr);
		fprintf(fptr, "\n");
	}

	fclose(fptr);
}

void translate_script_to_binary(String_View src)
{
	while (src.count > 0) {
		String_View line = sv_trim(sv_chop_by_delim(&src, ' '));
		
		if (sv_eq(line, sv_from_cstr("CREATE"))) {
			line = sv_trim(sv_chop_by_delim(&src, ' '));
			if (sv_eq(line, sv_from_cstr("TABLE"))) {
				create_table(&src);
			}
			else {
				fprintf(stderr, "Error: HAVE TO BE TABLE SYNTAX AFTER CREATE!!!\n");
				exit(1);
			}
		}
	}
}

#endif
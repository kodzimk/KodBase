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

String_View slurp_file(const char* file_path);

typedef enum {
	NONE,
	INT,
	VARCHAR,
}Data_Type;

typedef struct {
	void* selected_segment;
	String_View seleected_seqment_name;
	Data_Type selected_seeqment_type;
}Base;

void create_table(String_View src);
void translate_script_to_binary(String_View src);

#endif

#ifndef KODI_IMPLEMENTATION

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
	for (size_t i = 6; i < table_name.count + 6; i++)
	{
		name[i] = table_name.data[i - 6];
	}

	chdir("data/");
	system(name);
	memset(name, 0, sizeof name);
	for (size_t i = 0; i < table_name.count; i++)
	{
		name[i] = table_name.data[i];
	}name[table_name.count + 1] = "/";
	chdir(name);

	FILE* fptr;
	fptr = fopen("names.txt", "w");

	while (*src.data != ')') {
		String_View member_name = sv_trim(sv_chop_by_delim(&src, ' '));
		String_View member_type = sv_trim(sv_chop_by_delim(&src, ','));
		src.count -= 2;
		src.data += 2;
		src = sv_trim(src);

		for (size_t i = 0; i < member_type.count; i++)
		{
			fputc(member_type.data[i], fptr);
		}fputc(' ', fptr);
		for (size_t i = 0; i < member_name.count; i++)
		{
			fputc(member_name.data[i], fptr);
		}fputc('\n', fptr);
	}
	fclose(fptr);
}

void create_record_table(String_View src)
{
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
			fprintf(stderr, "There is no 'form' at all!!!\n");
			exit(1);
		}
		else {
			size_t cur = 0;
			char line[64];
			char ch;
			bool form_have = false;

			while ((ch = fgetc(fptr)) != EOF) {
				line[cur++] = ch;

				if (ch == '\n') {
					String_View name = sv_from_cstr(line);
					name.count -= 2;
					if (sv_eq(table_name, name)) {
						form_have = true;
						break;
					}
					memset(line, 0, sizeof line);
					cur = 0;
				}
			}

			if (!form_have){
				fprintf(stderr, "There is not form like that!!!\n");
				exit(1);
			}

			fclose(fptr);
		}
	}

	Data_Type types[12];
	size_t type_size = 0;
	char dir[64];

	for (size_t i = 0; i < table_name.count; i++)
	{
		dir[i] = table_name.data[i];
	}
	chdir("IGOR");

	String_View name = slurp_file("names.txt");
	char names[12][64];
	int size = 0;
	int sizes[12];

	while (name.count > 0) {
		String_View type = sv_trim(sv_chop_by_delim(&name,' '));
		if (sv_eq(type, sv_from_cstr("INT"))) {
			printf("INT\n");
			types[type_size++] = INT;
		}else if (sv_eq(type, sv_from_cstr("VARCHAR"))) {
			printf("VARCHAR\n");
			types[type_size++] = VARCHAR;
		}
		type = sv_trim(sv_chop_by_delim(&name, '\n'));

		for (size_t i = 0; i < type.count; i++)
		{
			names[size][i] = type.data[i];
		}
		sizes[size] = type.count;
		size++;
	}

	FILE* data_file;
	data_file = fopen("1.txt", "w");

	String_View data = sv_trim(sv_chop_by_delim(&src, ')'));
	size_t cur = 0;
	while (data.count > 0) {
		String_View value = sv_trim(sv_chop_by_delim(&data, ','));
		for (size_t i = 0; i < sizes[cur]; i++)
		{
			fputc(names[cur][i], data_file);
		}fputc(':', data_file);

		if (isdigit(*value.data) && types[cur] == INT) {

			for (size_t i = 0; i < value.count; i++)
			{
				fputc(value.data[i], data_file);
			}fputc('\n', data_file);
		}
		else if (types[cur] == VARCHAR) {
			value.count -= 1;
			value.data += 1;
			for (size_t i = 0; i < value.count - 1; i++)
			{
				fputc(value.data[i], data_file);
			}fputc('\n', data_file);
		}
		cur++;
	}

	fclose(data_file);
}

void translate_script_to_binary(String_View src)
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
	}
}

#endif
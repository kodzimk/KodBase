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
#define KB_RECORD_MAX_COUNT 12
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
	char selected_record[64];
	String_View selected_form_name;
}Base;

typedef struct {
	Data_Type type;
	String_View name;
	void* value;
}Members;

Base base = { 0 };

void create_table(String_View *src);
void create_record(String_View* src);
void select_record(String_View* src, Base* base);
void set_cstr_from_sv(char* str, String_View sv, size_t blank);
void translate_script_to_binary(String_View src,Base *base);

#endif

#ifndef KODI_IMPLEMENTATION

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

void set_cstr_from_sv(char* str, String_View sv,size_t blank)
{
	for (size_t i = blank; i < sv.count + blank; i++)
	{
		str[i] = sv.data[i - blank];
	}
}

bool check_for_form(String_View form_name)
{
	String_View names = slurp_file("forms");
	while (names.count > 0) {
		String_View name = sv_trim(sv_chop_by_delim(&names, '\n'));
		if (sv_eq(name, form_name)) {
			return 1;
		}
	}

	return 0;
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
	else if (check_for_form(table_name))
	{
		fprintf(stderr, "Error: there is already form with name: %s\n", table_name.data);
		fclose(fptr);
		exit(1);
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

void create_record(String_View* src)
{
	String_View table_name = sv_trim(sv_chop_by_delim(src, '('));
    
	cd("data");
	FILE* fptr;
	fptr = fopen("forms", "r");
	if (fptr == NULL) {
		fprintf(stderr, "Error: there is no 'forms' at all\n");
		exit(1);
	}
	if (!check_for_form(table_name)) {
		fprintf(stderr, "Error: there is no form with name: %s\n", table_name.data);
		fclose(fptr);
		exit(1);
	}
	fclose(fptr);

	char dir[64];
	set_cstr_from_sv(dir, table_name, 0);
	cd(dir);
	memset(dir, 0, sizeof dir);

	Members members[KB_RECORD_MAX_COUNT];
	int members_size = 0;
	int unique_member = -1;

	String_View member_src = slurp_file("names");
	String_View record_src = sv_trim(sv_chop_by_delim(src, ')'));
	while (member_src.count > 0) {
		String_View member = sv_trim(sv_chop_by_delim(&member_src, ' '));

		if (sv_eq(member, sv_from_cstr("INT"))) {
			members[members_size].type = INT;
		}
		else if (sv_eq(member, sv_from_cstr("VARCHAR"))) {
			members[members_size].type = VARCHAR;
		}

		member = sv_trim(sv_chop_by_delim(&member_src, '\n'));
		String_View unique = sv_trim(sv_chop_by_delim(&member, ' '));
		if (sv_eq(unique, sv_from_cstr("UNIQUE"))) {
			unique_member = members_size;
			members[members_size].name = (String_View){ .count = member.count,.data = member.data };
		}
		else {
		members[members_size].name = (String_View){ .count = unique.count,.data = unique.data };
		}
        

		String_View record_data = sv_trim(sv_chop_by_delim(&record_src, ','));
		if (isdigit(*record_data.data)) {
			members[members_size].value = (void*)sv_to_u64(record_data);
		}
		else if (*record_data.data == '"') {
			record_data.count -= 1;
			record_data.data += 1;
			char* line = (char*)malloc(sizeof(char) * 12);

			for (size_t i = 0; i < record_data.count - 1; i++)
			{
				line[i] = record_data.data[i];
			}
			members[members_size].value = line;
		}
		else {
			fprintf(stderr, "Error: incorrect type or not similar data type\n");
			exit(1);
		}
		members_size += 1;
	}

	if (unique_member == -1) {
		fprintf(stderr, "Error: Has to be at least one unique member!\n");
		exit(1);
	}

	if (members[unique_member].type == INT) {
		int result = (uintptr_t)members[unique_member].value;
		citoa(result, dir, 10);
	}
	else if(members[unique_member].type == VARCHAR){
		char* line = (char*)members[unique_member].value;
		set_cstr_from_sv(dir, sv_from_cstr(line), 0);
	}

	fptr = fopen(dir, "w");

	int index = 0;
	while (members_size > index) {
		fwrite(members[index].name.data, sizeof(char),
			   members[index].name.count, fptr);
		fprintf(fptr, ": ");

		if (members[index].type == VARCHAR) {
			fwrite(members[index].value, sizeof(char),
				strlen((char*)members[index].value), fptr);
			fprintf(fptr, "\n");
			free(members[index].value);
		}
		else {
			char line[64];
			int result = (uintptr_t)members[index].value;
			citoa(result, line, 10);
			for (size_t i = 0; i < strlen(line); i++)
			{
				fputc(line[i], fptr);
			}fputc('\n', fptr);
		}
		index += 1;
	}

	fclose(fptr);
}

void select_record(String_View* src, Base* base)
{
	String_View unique_member = sv_trim(sv_chop_by_delim(src, ' '));
	String_View form_name = sv_trim(sv_chop_by_delim(src, ' '));
	if (!sv_eq(form_name, sv_from_cstr("FROM"))) {
		fprintf(stderr, "Error: HAVE TO BE 'FORM' !!!\n");
		exit(1);
	}

	form_name = sv_trim(sv_chop_by_delim(src, '\n'));
	base->selected_form_name = form_name;
	set_cstr_from_sv(base->selected_record, unique_member, 0);
}

void update_record(String_View* src, Base* base)
{
	String_View updated_member = sv_trim(sv_chop_by_delim(src, ' '));
	String_View updated_value = sv_trim(sv_chop_by_delim(src, '\n'));

	cd("data");
	FILE* fptr;
	fptr = fopen("forms", "r");
	if (fptr == NULL) {
		fprintf(stderr, "Error: there is no 'forms' at all\n");
		exit(1);
	}
	if (!check_for_form(base->selected_form_name)) {
		fprintf(stderr, "Error: there is no form with name: %s\n", base->selected_form_name.data);
		fclose(fptr);
		exit(1);
	}
	fclose(fptr);

	char dir[64];
	set_cstr_from_sv(dir, base->selected_form_name, 0);
	cd(dir);
	memset(dir, 0, sizeof dir);

	Members members[KB_RECORD_MAX_COUNT];
	size_t members_size = 0;
	size_t unique_member = -1;

	String_View member_src = slurp_file("names");
	String_View record_src = slurp_file(base->selected_record);
	set_cstr_from_sv(dir, (String_View) { .count = strlen(base->selected_record), .data = base->selected_record }, 0);

	while (member_src.count > 0) {
		String_View member = sv_trim(sv_chop_by_delim(&member_src, ' '));

		if (sv_eq(member, sv_from_cstr("INT"))) {
			members[members_size].type = INT;
		}
		else if (sv_eq(member, sv_from_cstr("VARCHAR"))) {
			members[members_size].type = VARCHAR;
		}

		member = sv_trim(sv_chop_by_delim(&member_src, '\n'));
		String_View unique = sv_trim(sv_chop_by_delim(&member, ' '));
		if (sv_eq(unique, sv_from_cstr("UNIQUE"))) {
			unique_member = members_size;
			members[members_size].name = (String_View){ .count = member.count,.data = member.data };
		}
		else {
			members[members_size].name = (String_View){ .count = unique.count,.data = unique.data };
		}

		String_View record_data = sv_trim(sv_chop_by_delim(&record_src, ':'));
		if (sv_eq(record_data, updated_member)) {
			sv_trim(sv_chop_by_delim(&record_src, '\n'));

			if (isdigit(*updated_value.data)) {
				members[members_size].value = (void*)sv_to_u64(updated_value);
				if (unique_member == members_size) {
				 citoa((int)sv_to_u64(updated_value), base->selected_record, 10);
				}
			}
			else if (*updated_value.data == '"') {
				updated_value.count -= 1;
				updated_value.data += 1;
				char* line = (char*)malloc(sizeof(char) * 12);

				for (size_t i = 0; i < updated_value.count - 1; i++)
				{
					line[i] = updated_value.data[i];
				}
				members[members_size].value = line;
				if (unique_member == members_size) {
					set_cstr_from_sv(base->selected_record, (String_View) { .count = strlen(line), .data = line }, 0);
				}
			}

			members_size += 1;
			continue;
		}

		record_data = sv_trim(sv_chop_by_delim(&record_src, '\n'));
		if (isdigit(*record_data.data)) {
			members[members_size].value = (void*)sv_to_u64(record_data);
			if (unique_member == members_size) {
				citoa((int)sv_to_u64(record_data), base->selected_record, 10);
			}
		}
		else if (isalpha(*record_data.data)) {
			char* line = (char*)malloc(sizeof(char) * 12);

			for (size_t i = 0; i < record_data.count; i++)
			{
				line[i] = record_data.data[i];
			}
			members[members_size].value = line;
			if (unique_member == members_size) {
				set_cstr_from_sv(base->selected_record, (String_View) { .count = strlen(line), .data = line }, 0);
			}
		}
		else {
			fprintf(stderr, "Error: incorrect type or not similar data type\n");
			exit(1);
		}
		members_size += 1;
	}

	char del[32] = "rm ";
	set_cstr_from_sv(del, (String_View) { .count = strlen(dir), .data = dir }, 3);
	system(del);

	fptr = fopen(base->selected_record, "w");

	size_t index = 0;
	while (members_size > index) {
		fwrite(members[index].name.data, sizeof(char),
			members[index].name.count, fptr);
		fprintf(fptr, ": ");

		if (members[index].type == VARCHAR) {
			fwrite(members[index].value, sizeof(char),
				strlen((char*)members[index].value), fptr);
			fprintf(fptr, "\n");
			free(members[index].value);
		}
		else {
			char line[64];
			int result = (uintptr_t)members[index].value;
			citoa(result, line, 10);
			for (size_t i = 0; i < strlen(line); i++)
			{
				fputc(line[i], fptr);
			}fputc('\n', fptr);
		}
		index += 1;
	}

	fclose(fptr);
}

void delete_record(String_View* src, Base* base)
{
	String_View unique_member = sv_trim(sv_chop_by_delim(src, ' '));
	String_View form_name = sv_trim(sv_chop_by_delim(src, ' '));
	if (!sv_eq(form_name, sv_from_cstr("FROM"))) {
		fprintf(stderr, "Error: HAVE TO BE 'FROM' !!!\n");
		exit(1);
	}
	form_name = sv_trim(sv_chop_by_delim(src, '\n'));

	cd("data");
	FILE* fptr;
	fptr = fopen("forms", "r");
	if (fptr == NULL) {
		fprintf(stderr, "Error: there is no 'forms' at all\n");
		exit(1);
	}
	if (!check_for_form(form_name)) {
		fprintf(stderr, "Error: there is no form with name: %s\n", form_name.data);
		fclose(fptr);
		exit(1);
	}
	fclose(fptr);

	char dir[64];
	set_cstr_from_sv(dir, form_name, 0);
	cd(dir);
	memset(dir, 0, sizeof dir);

	set_cstr_from_sv(dir, unique_member, 0);

	fptr = fopen(dir, "r");
	if (fptr == NULL) {
		fprintf(stderr, "Error: there is no 'record' at all\n");
		exit(1);
	}
	fclose(fptr);

	char del[32] = "rm ";
	set_cstr_from_sv(del, unique_member, 3);
	system(del);

	if (sv_eq(unique_member, (String_View) { .count = strlen(base->selected_record), .data = base->selected_record })) {
		memset(base->selected_record, 0, sizeof base->selected_record);
	}

}

void delete_form(String_View* src, Base* base)
{
	String_View form_name = sv_trim(sv_chop_by_delim(src, '\n'));

	cd("data");
	String_View forms = slurp_file("forms");
	FILE* fptr = fopen("forms", "w");

	while (forms.count > 0) {
		String_View form = sv_trim(sv_chop_by_delim(&forms, '\n'));
		if (!sv_eq(form, form_name)) {
			char temp[64];
			set_cstr_from_sv(temp, form, 0);
			fwrite(temp, sizeof(char), form.count, fptr);
		}
	}fclose(fptr);

	char del[32] = "rm -rf ";
	set_cstr_from_sv(del, form_name, 7);
	system(del);

	if (sv_eq(form_name, base->selected_form_name)) {
		base->selected_form_name = (String_View){ .count = 0,.data = "" };
	}
}

void translate_script_to_binary(String_View src,Base *base)
{
	while (src.count > 0) {
		String_View line = sv_trim(sv_chop_by_delim(&src, ' '));
		if (sv_eq(line, sv_from_cstr("CREATE"))) {
			line = sv_trim(sv_chop_by_delim(&src, ' '));
			if (sv_eq(line, sv_from_cstr("TABLE"))) {
				create_table(&src);
			}
			else {
				fprintf(stderr, "Error: HAVE TO BE 'TABLE' SYNTAX AFTER 'CREATE'!!!\n");
				exit(1);
			}
			cd("../..");
		}
		if (sv_eq(line, sv_from_cstr("INSERT"))) {
			line = sv_trim(sv_chop_by_delim(&src, ' '));
			if (sv_eq(line, sv_from_cstr("INTO"))) {
				create_record(&src);
			}
			else {
				fprintf(stderr, "Error: HAVE TO BE 'INTO' SYNTAX AFTER 'INSERT'!!!\n");
				exit(1);
			}
			cd("../..");
		}
		if (sv_eq(line, sv_from_cstr("SELECT"))) {
			line = sv_trim(sv_chop_by_delim(&src, ' '));
			if (sv_eq(line, sv_from_cstr("UNIQUE"))) {
				select_record(&src,base);
			}
			else {
				fprintf(stderr, "Error: HAVE TO BE 'UNIQUE' SYNTAX AFTER 'SELECT'!!!\n");
				exit(1);
			}
		}
		if (sv_eq(line, sv_from_cstr("UPDATE"))) {
			update_record(&src, base);
			cd("../..");
		}
		if (sv_eq(line, sv_from_cstr("DELETE"))) {
			line = sv_trim(sv_chop_by_delim(&src, '\n'));
			String_View temp = sv_trim(sv_chop_by_delim(&line, ' '));
			if (sv_eq(temp, sv_from_cstr("UNIQUE"))) {
				delete_record(&line, base);
			    cd("../..");
			}
			else {
				delete_form(&temp, base);
				cd("..");
			}
		}
	}
}

#endif
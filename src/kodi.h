#ifndef KODI_H
#define KODI_H

#include<stdio.h>
#include<stdbool.h>
#include<string.h>
#include<ctype.h>

#define KB_SEGMENTS_MAX_COUNT 12
#define KB_TABLE_MAX_COUNT 6

typedef enum {
	CREATE_TABLE,
	SELECT_ITEM,
}Inst_Type;

typedef enum {
	INT,
	VARCHAR,
}Data_Type;

typedef struct {
	const char* data;
	size_t count;
}String_t;

bool st_eq(String_t a, String_t b);
String_t cstr_to_st(const char* cstr);
String_t st_trim_left(String_t line);
String_t st_trim_right(String_t line);
String_t st_trim(String_t line);
String_t chop_by_delim(String_t* line, char delimmator);

typedef struct {
	Inst_Type inst_type;
	size_t table_index;
}Inst;

typedef struct {
	void* value;
	Data_Type type;
}Segment;

typedef struct {
	Segment segments[KB_SEGMENTS_MAX_COUNT];
	size_t seqments_size;
}Table;

typedef struct {
	Table tables[KB_TABLE_MAX_COUNT];
	size_t tables_size;
}Base;

Base base = { 0 };

void translate_script_to_binary(String_t src, Base* base);

#endif

#ifndef KODI_IMPLEMENTATION

bool st_eq(String_t a, String_t b)
{
	if (a.count != b.count) {
		return 0;
	}
	else {
		return memcmp(a.data, b.data, a.count) == 0;
	}
}

String_t cstr_to_st(const char* cstr)
{
	return (String_t) { .count = strlen(cstr), .data = cstr };
}

String_t st_trim_left(String_t st)
{
	size_t i = 0;
	while (i < st.count && isspace(st.data[i])) {
		i += 1;
	}

	return (String_t) {
		.count = st.count - i,
			.data = st.data + i,
	};
}

String_t st_trim_right(String_t st)
{
	size_t i = 0;
	while (i < st.count && isspace(st.data[st.count - 1 - i])) {
		i += 1;
	}

	return (String_t) {
		.count = st.count - i,
			.data = st.data
	};
}

String_t st_trim(String_t line)
{
	return st_trim_right(st_trim_left(line));
}

String_t chop_by_delim(String_t* st, char delimmator)
{
	size_t i = 0;
	while (i < st->count && st->data[i] != delimmator) {
		i += 1;
	}

	String_t result = {
		.count = i,
		.data = st->data,
	};

	if (i < st->count) {
		st->count -= i + 1;
		st->data += i + 1;
	}
	else {
		st->count -= i;
		st->data += i;
	}

	return result;
}

String_t slurp_file(const char* file_path)
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

	return (String_t) {
		.count = n,
		.data = buffer,
	};
}

void translate_script_to_binary(String_t src, Base* base)
{

}

#endif
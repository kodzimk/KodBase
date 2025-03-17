#include"kodi.h"
#define KODI_IMPLEMENTATION

char* shift(int argc, char** argv)
{
	if (argc > 1) {
		argc -= 1;
		argv++;
	}

	return *argv;
}

int main(int argc,char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Error: No format file <input.kb>\n");
		return -1;
	}

	const char* script_file = shift(argc, argv);
	String_View source = slurp_file(script_file);

	translate_script_to_binary(source,&base);

	return 0;
}
/*---------------------------------------------------------------------------------
  bin2fgz - psp tool based on WinterMute's raw2c
  binary file to fake gzip (16 char header, 12 filename, 4 size - then array data
---------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>

#define VERMAJ 0
#define VERMIN 1

char	srcName[MAXPATHLEN], dstName[MAXPATHLEN];	// file name buffers
static char	baseFileName[MAXPATHLEN];		// source file name without extension
static char	ArrayName[MAXPATHLEN];		// source file name without extension

typedef struct {
unsigned char filename [12]; // 8.3 naming max
unsigned long size; // size, forced to little endian on my system
} fgz_info;

fgz_info finfo;
int aligned = 0;

// Parse file name. Put file name without extension in
// baseFileName, and return:
int parseFileName(char *str)
{
	int	i;
	char	*cptr;

	memset(finfo.filename, 0x00, 12);

	strcpy(baseFileName, str);
	strcpy(srcName, str);

	cptr = strrchr(str, '.');
	if (!cptr)
	{						// if '.' not found, then append default extension
		strcat(srcName, ".bin");
		strcat(baseFileName, "_bin");
	}
	else
	{
		i = (int) (cptr - str);	// get offset of '.' character
		baseFileName[i] = '_';
	}
	if(strlen(srcName) < 13)
		strcpy(finfo.filename, srcName);
	else
	{
		fprintf(stderr,	"Error: Filename is too long!\n");
		return 0;
	}

	if ((cptr = strrchr(baseFileName,'\\')))
	{
		strcpy(ArrayName, cptr+1);
	}
	else if ((cptr = strrchr(baseFileName, '/')))
	{
		strcpy(ArrayName, cptr+1);
	}
	else
	{
		strcpy(ArrayName, baseFileName);
	}
	return 1;
}

long fsize(FILE* f)
{
	long size;
	long temp = ftell(f);

	fseek(f,0,SEEK_END);
	size = ftell(f);
	fseek(f, temp, SEEK_SET);

	return size;
}

static const char head[] = "\n//  This file was autogenerated by bin2fgz\n\n";
static const char comment[] = "\n";

void usage ()
{
	fprintf(stderr,	"Usage:\tbin2fgz filename<.ext>\n"
					"\t-a adds aligned 16 to variable\n"
					"\tarray can be overlaid with the fakegzf type struct\n"
					"\tdefault input extension is .bin\n");
}

//---------------------------------------------------------------------------------
static void MakeSource(FILE* Infile, FILE* Outfile, int size) {
//---------------------------------------------------------------------------------
	unsigned long int counter = 0UL;
	unsigned long length;
	unsigned char thisElement;
	int i;

	rewind(Infile);
	rewind(Outfile);
	length = fsize(Infile);
	finfo.size = length;

	fprintf(Outfile, head); /* Put top comment into source */
	fprintf(Outfile, "#ifndef _%s_h_\n",ArrayName);
	fprintf(Outfile, "#define _%s_h_\n",ArrayName);
	fprintf(Outfile, comment); /* Put separator comment into source */
	fprintf(Outfile, "unsigned char %s[0x%lx+0x10]", ArrayName, length);
	if (aligned)
		fprintf(Outfile, " __attribute__((aligned(16)))");
	fprintf(Outfile, " = {\n\t");

	// add in the fgz info - yeah, its very bad manners to overrun the char field to put the int too...
	for(i=0; i<16;i++)
	{
		if(i<12)
		{
			if(finfo.filename[i] != 0x00)
				fprintf(Outfile,"'%c',  ", finfo.filename[i]);
			else
				fprintf(Outfile,"0x00, ");
		}
		else
		{
			fprintf(Outfile,"0x%02x", finfo.filename[i]);
			fprintf(Outfile, ", ");
		}
	}

	fputc('\n', Outfile);
	fputc('\t', Outfile);

	// add in 
	while ( counter < length )
	{
		if ( fread(&thisElement, 1, 1, Infile) >= 1 ) {
			fprintf(Outfile,"0x%02x", thisElement);
		}
		counter++;
		if ( counter < length ) /* More to go */
		fprintf(Outfile, ", ");
		if ( !((counter) % 16) ) {
			fputc('\n', Outfile);
			fputc('\t', Outfile);
		}
	}
	fprintf(Outfile, "\n};\n");
	fprintf(Outfile, "#endif //_%s_h_\n",ArrayName);
	return;
}

//---------------------------------------------------------------------------------
int main (int argc, char* argv[]) {
//---------------------------------------------------------------------------------
	int a;
	FILE *fInfile, *fHfile;

	fprintf(stderr,"bin2fgz %d.%d by cory1492\n", VERMAJ, VERMIN);
	if (argc < 2) {
		usage();
		return -1;
	}

	for (a=1; a<argc; a++) {
		if (argv[a][0] == '-')
		{
			switch (argv[a][1])
			{
				case 'h':
					usage();
					break;
//				case 's':
//					elementSize = atoi(&argv[a][2]);
//					break;
				case 'a':
					aligned = 1;
					break;
				default:
				{
					printf("Unknown option: %s\n", argv[a]);
					usage();
					break;
				}
			}
		}
		else
		{
			if(!parseFileName(argv[a]))
				return 0; // exit if parsing file name fails
		}
	}

	fInfile = fopen(srcName, "rb");
	strcpy(dstName, ArrayName);
	strcat(dstName, ".h");
	fHfile = fopen(dstName, "wb");

	MakeSource(fInfile,fHfile,1);

	fclose(fInfile);
	fclose(fHfile);

	return EXIT_SUCCESS;

}

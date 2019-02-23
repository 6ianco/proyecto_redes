#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
	CMD_SUBMIT,
	CMD_QUERY,
	CMD_DELETE,
}Cmd_t; 


int main()
{
	char str[4]="hola";
	int i;
	Cmd_t myc;
	myc = CMD_DELETE;
	
	char **arg;
	char *s;
	char buf[10];
	s = buf;

	arg = (char **)malloc(5*sizeof(char *));
	sprintf(buf,"job been seent");
//	strcpy(arg[1],"hola como estas");
	arg[2] = "yo bien";

	printf("%s\n", arg[2]);
	printf("%s\n", s);
	
	
	
//	printf("%d\n",myc);
	
	return 0;
}

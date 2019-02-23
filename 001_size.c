#include <stdio.h>
#include <string.h>

void printFuncion(void){
	printf("%d\n", argv[0]);
}

int main(int argc,char *argv[]){
	int i;
	for(i=0; i<argc; i++){
		printf("%s\tsize:%lu\n", argv[i], strlen(argv[i]));
	}
	printFunction();
	return 0;
}

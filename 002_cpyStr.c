#include <stdio.h>
#include <string.h>

typedef struct {
	char name[20];
	char exe[20];
} info_t;

int main(){
	info_t v;
	strcpy(v.name, "gianfranco");
	strcpy(v.exe, "adapuma");

	info_t v2;
	v2 = v;
	printf("%s\n", v2.name);


	printf("%d\n", strcmp(" ","hola"));

	return 0;
	
}

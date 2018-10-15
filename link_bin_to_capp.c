#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

asm(".balign 4096");
//asm(".global _binary__module_start"); //to use it in other files, need to make it global.
asm("_binary__module_start:");

asm(".incbin \"test.txt\"");
//asm(".global _binary__module_end");
asm("_binary__module_end:");

extern unsigned char _binary__module_start[];
extern unsigned char _binary__module_end[];

int main(int argc, char *argv[])
{
	int len;
	char *ptr;

	len = _binary__module_end - _binary__module_start;
	ptr = (char *)malloc(len + 1);
	if (ptr == NULL) {
		return -1;
	}
	memset(ptr, 0, len + 1);
	memcpy(ptr, _binary__module_start, len);
	printf("%s.\n", ptr);
	free(ptr);

	return 0;
}

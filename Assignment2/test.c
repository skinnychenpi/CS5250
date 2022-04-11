#include <linux/unistd.h>
#define __NR_printmsg 548
#include <stdio.h>


int printmsg(int i) {
	return syscall(__NR_printmsg, i);
}

int main(int argc, char** argv) {
	long int val = printmsg(668);
	printf("Syscall returned %ld\n", val);
	return 0;
}

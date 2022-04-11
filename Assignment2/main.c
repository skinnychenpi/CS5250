#include <linux/unistd.h>
#define __NR_printmsg 548

int printmsg(int i) {
	return syscall(__NR_printmsg, i);
}

int main(int argc, char** argv) {
	printmsg(668);
	return 0;
}

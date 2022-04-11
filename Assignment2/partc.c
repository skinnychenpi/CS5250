#include <stdio.h>

int globalvar = 42;

int foo(int arg) {
	return globalvar + arg;
}

int main() {
	foo(10);
	printf("%d\n", globalvar);
	return 0;
}


#include <pthread.h>
#include <stdio.h>
/* Musl-otter-cross pthreads test */

void *bar(void *baz)
{ 
	/* Does nothing */
}
int main()
{
	/* 
		This program is not intended to be ran. It
		is simply a cross-compiler test to check for 
		pthreads. Please improve it.
	*/
	int foo = 0;
	pthread_t inc_x_thread;
	pthread_create(&inc_x_thread, NULL, bar, &foo);
	pthread_join(inc_x_thread, NULL);
	return 0;
}

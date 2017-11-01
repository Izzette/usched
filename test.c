// test.c

#include <stddef.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "usched.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
__attribute__((optimize("O0")))
static void *hello_world (void *p) {
	printf ("[Hello World from userland thread %d!]", *(int *)p);
	fflush (stdout);

	for (int i = 0; 200 > i; ++i) {
		for (int j = 0; 100000 > j; ++j);
		putc ('.', stdout);
		fflush (stdout);
	}

	char c = 'x';
	size_t s = fread (&c, 1, 1, stdin);
	printf ("[Read char size %zu, errno %s]", s, strerror (errno));

	printf ("[Goodbye World from userland thread %d!]", *(int *)p);
	fflush (stdout);

	return p;
}
#pragma GCC diagnostic pop

int main () {
	usched_init();
	usched_thread_t *hello_threads[3];
	int hello_thread_numbers[3] = { 0, 1, 2 };
	usched_create (&hello_threads[0], hello_world, &hello_thread_numbers[0]);
	usched_create (&hello_threads[1], hello_world, &hello_thread_numbers[1]);
	usched_create (&hello_threads[2], hello_world, &hello_thread_numbers[2]);

	for (;;) {
		for (int i = 0; 3 > i; ++i) {
			if (hello_threads[i]) {
				if (!hello_threads[i]->result)
					break;

				printf ("[Hello thread %ld exited, result: %d]",
						hello_threads[i]->tid, **(int **)hello_threads[i]->result);
				usched_thread_release (hello_threads[i]);
				hello_threads[i] = NULL;
			}

			if (i == 2)
				return 0;
		}

		sleep (1);
	}
}

// vim: set ts=4 sw=4 noet syn=c:

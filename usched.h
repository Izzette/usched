// usched.h

#include <unistd.h>

#define USCHED_MAX_THREADS 10
#define USCHED_STACK_NPAGES (1000U)
#define USCHED_STACK_SIZE (_SC_PAGESIZE * USCHED_STACK_NPAGES)

typedef struct usched_thread_struct {
	unsigned long tid;
	void *stack_end;
	void *stack;
	void **result;
} usched_thread_t;

int usched_init ();
void usched_create (usched_thread_t **, void *(*) (void *), void *);
void usched_yeild ();
void usched_thread_release (usched_thread_t *);

// vim: set ts=4 sw=4 noet syn=c:

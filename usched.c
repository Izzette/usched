// usched.c

#include <stdint.h>
#include <stdatomic.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "usched.h"
#include "usched_switch.h"

static atomic_ulong next_tid;

// TODO: linked list?
static size_t cur_thread_index = 0;
static usched_thread_t *cur_thread = NULL;
static usched_thread_t *threads[USCHED_MAX_THREADS];

static struct sigaction usched_alarm_action;
static struct sigevent usched_alarm_event;
static struct itimerspec usched_tspec;

static void usched_alarm_action_handler (int);
static void progress_thread_index ();
static void *push_n (void *, void *, size_t);
static void usched_thread_init (usched_thread_t *);
__attribute__((noreturn)) static void usched_bootstrap ();

#define MK_STACK_PUSH(suffix, bits, id) \
static void *push_##suffix (void *stack, uint##bits##_t id) { \
	return push_n (stack, &id, sizeof(id)); \
}

MK_STACK_PUSH(byte,  8,  b)
MK_STACK_PUSH(word,  16, w)
MK_STACK_PUSH(dword, 32, dw)
MK_STACK_PUSH(qword, 64, qw)


int usched_init () {
	timer_t timer_id;
	int ret = 0;

	atomic_init (&next_tid, 1);

	memset((void *)&threads,             '\0', sizeof(threads));
	memset((void *)&usched_alarm_action, '\0', sizeof(usched_alarm_event));
	memset((void *)&usched_alarm_event,  '\0', sizeof(usched_alarm_event));
	memset((void *)&usched_tspec,        '\0', sizeof(usched_tspec));

	usched_alarm_action.sa_handler = usched_alarm_action_handler;

	usched_alarm_event.sigev_notify = SIGEV_SIGNAL;
	usched_alarm_event.sigev_signo = SIGALRM;

	usched_tspec.it_interval.tv_sec = 0;
	usched_tspec.it_interval.tv_nsec = 20000000;
	usched_tspec.it_value.tv_sec = 0;
	usched_tspec.it_value.tv_nsec = 20000000;

	usched_thread_t *main_thread =
		(usched_thread_t *) malloc (sizeof(usched_thread_t));
	usched_thread_init (main_thread);
	cur_thread = main_thread;
	threads[cur_thread_index] = main_thread;

	sigaction (SIGALRM, &usched_alarm_action, NULL);

	ret = timer_create (CLOCK_REALTIME, &usched_alarm_event, &timer_id);
	if (ret) return ret;

	ret = timer_settime (timer_id, 0, &usched_tspec, NULL);
	if (ret) return ret;

	return ret;
}

void usched_yeild () {
	progress_thread_index ();
	usched_thread_t *prev_thread = cur_thread;
	cur_thread = threads[cur_thread_index];
	usched_switch (&prev_thread->stack, cur_thread->stack);
}

void usched_create
		(usched_thread_t **thread_ptr, void *(*start_routine) (void *), void *p) {
	// TODO: ENOMEM checking.
	usched_thread_t *thread = (usched_thread_t *) malloc (sizeof(usched_thread_t));
	*thread_ptr = thread;

	usched_thread_init (thread);

	// The parameters for the bootstrap function.
	thread->stack = push_n (thread->stack, &p,             sizeof(p));
	thread->stack = push_n (thread->stack, &start_routine, sizeof(start_routine));
	thread->stack = push_n (thread->stack, &thread,        sizeof(thread));

	// Set the return address to zero, just to make sure that a segmentation fault
	// occurs if the bootstrap function were to ever return.
	void (*nowhere) (void) = NULL;
	thread->stack = push_n (thread->stack, &nowhere, sizeof(void (*) (void)));

	// Call the bootstrap function on task switch to this thread.
	void (*bootstrap) (void) = usched_bootstrap;
	thread->stack = push_n (thread->stack, &bootstrap, sizeof(void (*) (void)));

	// Initialize the stack state so it can be passed to usched_switch as any
	// other stack could be.
	thread->stack = usched_switch_stack_init (thread->stack);

	for (size_t i = 0; USCHED_MAX_THREADS > i; ++i) {
		if (!threads[i]) {
			threads[i] = thread;
			break;
		}
	}
}

void usched_thread_release (usched_thread_t *thread) {
	free (thread->stack_end);
	free (thread->result);
	free (thread);
}

static void usched_unblock_alarm () {
	sigset_t set;
	sigemptyset (&set);
	sigaddset (&set, SIGALRM);
	sigprocmask (SIG_UNBLOCK, &set, NULL);
}

static void usched_alarm_action_handler (int signal) {
	fprintf (stderr, "[Caught %s -> task switch]", strsignal(signal));
	fflush (stderr);

	usched_yeild ();

	usched_unblock_alarm ();
}

static void progress_thread_index () {
	do cur_thread_index = (cur_thread_index + 1) % USCHED_MAX_THREADS;
	while (!threads[cur_thread_index]);
}

static void *push_n (void *stack, void *p, size_t n) {
	return memcpy ((char *)stack - n, p, n);
}

static void usched_thread_init (usched_thread_t *thread) {
	thread->tid = atomic_fetch_add (&next_tid, 1);
	// TODO: ENOMEM checking.
	thread->stack_end = malloc (USCHED_STACK_SIZE);
	thread->stack = (char *)thread->stack_end + USCHED_STACK_SIZE;
	thread->result = NULL;
}

static void usched_thread_fin (usched_thread_t *thread, void *result) {
	// TODO: ENOMEM checking.
	thread->result = (void **)malloc (sizeof(result));
	*thread->result = result;
	threads[cur_thread_index] = NULL;
}

__attribute__((noreturn))
static void usched_bootstrap
		(usched_thread_t *thread, void *(*start_routine) (void *), void *p) {
	usched_unblock_alarm ();

	void *result = start_routine (p);

	usched_thread_fin (thread, result);
	// TODO: destroy executing thread
	for (;;) usched_yeild ();
}

// vim: set ts=4 sw=4 noet syn=c:

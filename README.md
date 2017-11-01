<!-- README.md -->
# USched
#### Proof of Concept \*nix User-Land Preemptive Multitasking
USched is a preemptive scheduler for x86 using user land POSIX signal API calls
to switch tasks on regular intervals using `sigaction(3p)` and
`timer_create(3p)`.  It not particularly efficient or useful, and not intended
to be a working replacement for `pthreads(7)`.  Unfortunately the method used to
preempt threads interrupts system calls, resulting in `EINTR` errors.  Thus any
program using USched must either tediously retry system calls if they are
interrupted, or refrain from using any at all.  It *does* however demonstrate
the basics of software multitasking on x86 without the complexity of a bare
metal kernel.  It has only been tested on `GNU/Linux`, no guarantees that it
will work on any other \*nix system, though, I believe it should.

## Overview

### `usched.c` / `usched.h`
Management of threads including preemption, creation, and destruction.

##### Defined macros
* `USCHED_MAX_THREADS`: the maximum number of possible threads (including the
  main thread) that USched can handle, attempting to create more threads than
  this is undefined.
* `USCHED_STACK_NPAGES`: the number of `_SC_PAGESIZE` pages (defined in
  `unistd.h`) that will be allocated for a new threads stack.
* `USCHED_STACK_SIZE`: the size in bytes that will be allocated for a new
  threads stack (defined as `(_SC_PAGESIZE * USCHED_STACK_NPAGES)`).

##### Defined types
* `usched_thread_t` (aka `struct usched_thread_struct`)
  * The structure representing a thread.
  * `unsigned long tid`: the unique thread ID.
  * `void *stack_end`: the lowest address of the stack, the one returned by
    `malloc(3)`.
  * `void *stack`: the stack pointer at last task switch.
  * `void **result`: a pointer to the result as returned by the `start_routine`
     passed to `usched_thread_create`; will be `NULL` until the thread exits.

##### Exported functions
* `int usched_init ()`
  * Initializes structures and starts generating timer "interrupts" (by
    `SIGALRM`) every 20ms.  These timer "interrupts" are what preempt threads.
  * Generates an entry for the "main thread", the one that will receive the
    first timer interrupt.
  * Must be called before any other function and can only be called once.
* `void usched_yeild ()`
  * Yield the running thread and switch tasks to the next in the queue (which
    could possibly be the executing thread.
* `void usched_create
  (usched_thread_t **thread_ptr, void *(*start_routine) (void *), void *p)`
  * Allocates and initializes a new thread and stores it in `*thread_ptr`.
  * Prepares the thread for the first task switch and places in into the
    `threads` task queue.
  * `p` will be passed to `start_routine` when it is called.
* `void usched_thread_release (usched_thread_t *thread)`
  * Deallocates the threads stack, result, and the thread structure itself.
  * Should be called after the thread is completed.

##### Static variables
* `atomic_ulong next_tid`
  * Next thread ID to be assigned to a new `usched_thread_t` upon creation.
* `usched_thread_t *threads[]`, `size_t cur_thread_index`,
  `usched_thread_t *cur_thread`
  * `threads`: array of running threads.  A `NULL` element represents the lack
    of a thread, the array in traversed cyclicly by the `cur_thread_index` on
    each task switch.
  * `cur_thread_index` the index of the executing thread, incremented by
    `void progress_thread_index ()`.
  * `cur_thread` the currently executing thread, **not** progressed by
    `progress_thread_index`, but instead by `usched_yeild`.

##### Static functions
* `void usched_unblock_alarm ()`
  * Unblocks the `SIGALRM` signal, this normally doesn't need to be done
    explicitly by the signal handler but is done transparently by `sigreturn(2)`
    (on Linux) or similar.
* `void usched_alarm_action_handler (int signal)`
  * Signal handler for `SIGALRM`, calls `usched_yeild` and unblocks `SIGALRM`
    signals before returning.
  * Prints `[Caught Alarm clock -> task switch]` to `stderr` on every signal
    handled.
* `void progress_thread_index ()`
  * Refer to `cur_thread_index` for details.
* `void *push_n (void *stack, void *src, size_t n)`
  * Pushes `n` bytes from `src` onto the `stack` pointer, and returns
    `(stack - n)`.
* `void usched_thread_init (usched_thread_t *thread)`,
  `void usched_thread_fin (usched_thread_t *thread, void *result)`.
  * `usched_thread_init`: Allocates a new thread and stack, initializes all
    values but does **not** prepare the stack for task switching.
  * `usched_thread_fin`: Allocates and assigns the `result`, removes the thread 
    from the `threads` task queue.  Does **not** deallocate the stack or the
    thread itself.
* `__attribute__((noreturn)) void usched_bootstrap
  (usched_thread_t *thread, void *(*start_routine) (void *), void *p)`
  * Bootstraps a new thread, this function is called after first task switch.
    It unblocks `SIGALRM` which is normally done by `usched_yeild`, but not
    in this case because a new thread wasn't previously preempted.

### `usched_switch.s` / `usched_switch.h`
Low-level task switch code for 32-bit x86 on the SysV ABI.

##### Exported functions
* `void usched_switch (void **cur_stack, void *next_stack)`
  * Switch tasks, store the current tasks state in `*cur_stack` and load the
    next tasks state from `next_stack`. 
  * The stacks specified by `*cur_stack` and `next_stack` may be the same.
* `void *usched_switch_stack_init (void *stack)`
  * Prepare the specified stack and return a stack pointer which can be passed
    to `usched_switch` as `next_stack`.

### `test.c`
An example program using USched, demonstrating it's functionality and some of
its downsides.

<!-- vim: set ts=2 sw=2 et syn=markdown: -->

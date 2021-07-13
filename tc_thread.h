/*
 * tc_thread.h: Cross-platform threading & atomics.
 *
 * DEPENDS:
 * VERSION: 0.2.0 (2021-07-10)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.2.2    fixed some instances of malloc() not using TC_MALLOC
 * 0.2.1    fixed macros tcthread_atomic{32,sz}_{inc,dec}
 * 0.2.0    implemented semaphores & RW locks
 * 0.1.0    initial public release
 *
 * TODOs:
 * - document semaphores, RW locks, and atomics
 * - tcthread_atexit; maybe tcthread_cancel (+ destructors)?
 * - initializers (if possible)
 * - spinlocks
 * - high-level threading: threadpool + job system
 *
 *
 *
 * Portable threading primitives & atomics.
 *
 * A single file should contain the following `#define` before including the header:
 *
 *      #define TC_THREAD_IMPLEMENTATION
 *      #include "tc_thread.h"
 *
 * Currently supports Windows (MSVC, gcc, clang) and various POSIX platforms.
 * Compilation should use the following options:
 * - POSIX: `-pthread`
 * - MSVC on Windows: One of `/MD`, `/MDd`, `/MT`, `/MTd` (see https://docs.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library)
 * - gcc or clang (in gcc mode) on Windows: (no action needed)
 * Contributions for support of other platforms are welcome.
 */

/* ========== API ==========
 *
 * The API follows the POSIX API quite closely (albeit not exactly).
 *
 *
 * ========== UTIL ==========
 *
 * SYNOPSIS:
 *  uint32_t tcthread_get_cpu_count(void);
 * RETURN VALUE:
 *  Number of *logical* processor cores, or `0` if value cannot be obtained.
 * DESCRIPTION:
 *  Get the number of logical processor cores (# of hardware threads).
 *
 *  Note that this may be higher than the number of physical cores in systems
 *  that use technologies such as hyper-threading.
 *
 *  In multi-processor systems (SMP), this attempts to get the number of *all*
 *  such cores if it can; otherwise, it'll generally return only the number of
 *  the current processing group.
 *
 *
 * ========== THREADING ==========
 *
 * SYNOPSIS:
 *  typedef void* tcthread_runner_t(void* udata);
 *  tcthread_t tcthread_create(size_t stack_size, tcthread_runner_t* runner, void* udata);
 *  bool tcthread_is_valid(tcthread_t thread);
 * PARAMETERS:
 *  - stack_size: size of thread stack, or `0` to use a default
 *  - runner: function to run when the thread is created
 *  - udata: user data, passed to the function
 *  - thread: thread to check the validity of
 * RETURN VALUE:
 *  - tcthread_runner_t: return value of this function will be passed onto `tcthread_join`
 *  - tcthread_create: the newly-created thread; use `tcthread_is_valid` to verify if creation was successful
 *  - tcthread_is_valid: whether the thread creation was successful; note that this will *not* catch other forms of invalid data, such as an uninitialized or already-joined `thread` value
 * DESCRIPTION:
 *  Create a new thread, and run it immediately.
 *
 *  Threads are created with `tcthread_create`, but should be verified with
 *  `tcthread_is_valid`. They are executed immediately (in a "running" state),
 *  and (on platforms that recognize the distinction) in a joinable state.
 * SEE ALSO:
 *  - `tcthread_join` to join with a thread, and get the return value of `runner`
 *  - `tcthread_detach` to detach a thread, allowing it to run independently
 *
 *
 * SYNOPSIS:
 *  void tcthread_join(tcthread_t thread, void** retval);
 * PARAMETERS:
 *  - thread: thread to join with
 *  - retval: return value from user-supplied thread function; use `NULL` to discard
 * DESCRIPTION:
 *  Wait for a thread to terminate, and join with it.
 *
 *  If a thread has already terminated, this function returns immediately.
 *  Otherwise, it waits for the thread to terminate and returns once it has
 *  done so.
 *
 *  After joining, the thread handle becomes invalid, and should not be used
 *  again. Attempting to call any function other than `tcthread_create` with
 *  said handle is undefined.
 * SEE ALSO:
 *  - `tcthread_create`; `retval` is the return value of the `runner` function
 *  - `tcthread_detach` to detach a thread, allowing it to run independently
 *
 *
 * SYNOPSIS:
 *  void tcthread_detach(tcthread_t thread);
 * PARAMETERS:
 *  - thread: thread to detach
 *  - retval: return value from user-supplied thread function; use `NULL` to discard
 * DESCRIPTION:
 *  Detach a thread, allowing it to run independently.
 *
 *  When a detached thread terminates, its resources are released back to the
 *  system without the need for manual intervention.
 *
 *  After detaching, the thread handle becomes invalid, and should not be used
 *  again. Attempting to call any function other than `tcthread_create` with
 *  said handle is undefined.
 * TODO:
 *  Leaks memory on some platforms. This needs to be fixed.
 * SEE ALSO:
 *  - `tcthread_create` to create a new thread
 *  - `tcthread_join` to join with a thread, allowing to fetch its return value
 *
 *
 * SYNOPSIS:
 *  TC_HINT_NORETURN void tcthread_exit(void* retval);
 * PARAMETERS:
 *  - retval: value to return from thread
 * RETURN VALUE:
 *  This function never returns.
 * DESCRIPTION:
 *  Terminate the calling thread, returning `retval` as the result.
 *
 *  Warning: C++ destructors may or may not run, depending on platform. Do not
 *  use this function if destructors need to be ran; return from the thread
 *  normally in that case.
 *
 *
 * SYNOPSIS:
 *  void tcthread_sleep(uint32_t ms);
 * PARAMETERS:
 *  - ms: the amount of time to sleep, in milliseconds
 * DESCRIPTION:
 *  Suspend execution of the current thread for an interval of time.
 *
 *  Note that the amount of time may be longer than requested, due to rounding
 *  and system-scheduling reasons.
 *
 *
 * SYNOPSIS:
 *  tcthread_t tcthread_self(void);
 * RETURN VALUE:
 *  Returns a handle to own thread, or an invalid handle if this failed.
 * DESCRIPTION:
 *  Get the handle to the currently-running thread.
 *
 *  Value returned may be invalid if the operation failed or if the thread is
 *  one that was *not* created via `tcthread_create` (such as the main thread).
 * TODO:
 *  Make this work with all threads, including those that were not created
 *  via `tcthread_create`.
 * SEE ALSO:
 *  - `tcthread_is_valid` to verify is the returned handle is valid
 *
 *
 * ========== MUTEXES ==========
 *
 * SYNOPSIS:
 *  tcthread_mutex_t tcthread_mutex_create(bool recursive);
 *  bool tcthread_mutex_is_valid(tcthread_mutex_t mutex);
 *  void tcthread_mutex_destroy(tcthread_mutex_t mutex);
 * PARAMETERS:
 *  - recursive: whether to create a recursive mutex (which permits multiple locks from same thread) or not
 *  - mutex: mutex to check the validity of, or to destroy
 * RETURN VALUE:
 *  - tcthread_mutex_create: the newly-created mutex; use `tcthread_mutex_is_valid` to verify if creation was successful
 *  - tcthread_mutex_is_valid: whether the mutex creation was successful; note that this will *not* catch other forms of invalid data, such as an uninitialized `mutex` value
 * DESCRIPTION:
 *  Create a new optionally-recursive mutex.
 *
 *  Recursive mutexes keep an internal reference count, so that they may be
 *  locked multiple times by the same thread. Attempting to do so with
 *  non-recursive mutexes is undefined.
 *
 *  Attempting to destroy a mutex that is currently locked is undefined.
 *
 *  Created mutex should be verified with `tcthread_mutex_is_valid`.
 *
 *
 * SYNOPSIS:
 *  void tcthread_mutex_lock(tcthread_mutex_t mutex);
 *  bool tcthread_mutex_try_lock(tcthread_mutex_t mutex);
 *  void tcthread_mutex_unlock(tcthread_mutex_t mutex);
 * PARAMETERS:
 *  - mutex: mutex to lock or unlock
 * RETURN VALUE:
 *  - tcthread_mutex_try_lock: `true` if the mutex was successfully locked, `false` otherwise
 * DESCRIPTION:
 *  Lock or unlock a mutex.
 *
 *  A mutex is considered "owned" by a thread if said thread is currently
 *  holding a lock.
 *
 *  `tcthread_mutex_lock` will block until the mutex is successfully locked by
 *  the current thread.
 *  `tcthread_mutex_try_lock` will attempt to do the same, but will never block;
 *  instead, it will return whether it was successful.
 *  Once a lock is successful, the current thread owns the mutex. Use
 *  `tcthread_mutex_unlock` to unlock the mutex and allow other threads to
 *  progress.
 *
 *  Locking an owned recursive mutex with `tcthread_mutex_[try_]lock` will
 *  immediately return (as a success, in the case of `tcthread_mutex_try_lock`).
 *  The mutex will remaine owned (locked) until `tcthread_mutex_unlock` is
 *  called the same number of times. Attempting to lock a *non*-recursive mutex
 *  that is currently owned by the same thread is undefined.
 *
 *  Attempting to unlock a mutex that the thread does not currently own is
 *  undefined.
 *
 *
 * ========== CONDITION VARIABLES ==========
 *
 * SYNOPSIS:
 *  tcthread_cond_t tcthread_cond_create(void);
 *  bool tcthread_cond_is_valid(tcthread_cond_t cond);
 *  void tcthread_cond_destroy(tcthread_cond_t cond);
 * PARAMETERS:
 *  - cond: condition variable to check the validity of, or to destroy
 * RETURN VALUE:
 *  - tcthread_cond_create: the newly-created condition variable; use `tcthread_cond_is_valid` to verify if creation was successful
 *  - tcthread_cond_is_valid: whether the condition variable creation was successful; note that this will *not* catch other forms of invalid data, such as an uninitialized `cond` value
 * DESCRIPTION:
 *  Create a new condition variable.
 *
 *  A condition variable can be used to wait for a certain event; an event can
 *  be signaled (which wakes up a single waiting thread), or it can be broadcasted
 *  (which wakes up multiple waiting threads).
 *  Note that spurious wakeups are possible: wakeups which were not triggered
 *  by a signal or broadcast. See documentation of `tcthread_cond_wait` for an
 *  example of the correct use of a condition variable.
 *
 *  Attempting to destroy a condition variable that a thread is currently
 *  waiting on is undefined.
 *
 *  Created condition variable should be verified with `tcthread_cond_is_valid`.
 *
 *
 * SYNOPSIS:
 *  void tcthread_cond_wait(tcthread_cond_t cond, tcthread_mutex_t mutex);
 *  bool tcthread_cond_timed_wait(tcthread_cond_t cond, tcthread_mutex_t mutex, uint32_t timeout_ms);
 * PARAMETERS:
 *  - cond: condition variable to wait on
 *  - mutex: mutex protecting access to the condition-value we are trying to access
 *  - timeout_ms: amount of time to wait for, in milliseconds
 * RETURN VALUE:
 *  - tcthread_cond_timed_wait: `true` if the condition variable was triggered in time, `false` if timeout was reached
 * DESCRIPTION:
 *  Wait for a condition variable to be signaled.
 *
 *  `mutex` *must* be locked before waiting is attempted; calling these
 *  functions with an unlocked mutex is undefined.
 *
 *  The mutex will be unlocked before starting the wait for a signal. Once a
 *  thread is woken up, the mutex is automatically re-locked.
 *
 *  `tcthread_cond_wait` blocks indefinitely until the function is woken up.
 *  `tcthread_cond_timed_wait` will only block until `timeout_ms` is reached;
 *  it returns `true` if it returned as a result of a signal, or `false` if it
 *  is due to the timeout.
 *
 *  Note that spurious wakeups are possible: wakeups which were not triggered
 *  by a signal or broadcast. This also affects `tcthread_cond_timed_wait` in
 *  that it may return a false positive. The correct way of accounting for such
 *  a case is to re-check the condition(s) we are waiting for after
 *  `tcthread_cond_[timed_]wait` returns.
 *
 *  Following is a typical pattern that handles spurious wakeups correctly.
 *  The example is that of a simple task queue, such as one that might be used
 *  in a threadpool:
 *
 *      // mutex protects our condition, queue->num_tasks (we also reuse it for queue->tasks, as the two are updated together)
 *      tcthread_mutex_lock(queue->mutex);
 *      // handle spurious wakeups correctly: the condition we are waiting on is `queue->num_tasks != 0`
 *      while(queue->num_tasks == 0)
 *      {
 *          // unlocks queue->mutex before waiting
 *          // locks queue->mutex before returning
 *          tcthread_cond_wait(queue->cond_task_added, queue->mutex);
 *          // in case of spurious wakeup, `queue->num_tasks` may still be `0`; hence the loop
 *      }
 *      // queue->num_tasks is now guaranteed to be non-zero (i.e. condition is valid)
 *      // queue->mutex is still (or perhaps again, if we waited on a condition) locked
 *      queue_task* task = queue->tasks[queue->num_tasks - 1];
 *      --queue->num_tasks;
 *      tcthread_mutex_unlock(queue->mutex);
 *      // run the task (outside the mutex, to prevent the task from blocking other threads)
 *      task->user_function(task->user_data);
 *
 *  Simultaneous waits on a condition variable with different mutexes is
 *  undefined. Different mutexes *may* be used with the same condition variable,
 *  but not at the same time.
 *
 * SEE ALSO:
 *  - `tcthread_mutex_create` to create a mutex that is required for using a condition variable
 *  - `tcthread_cond_signal` & `tcthread_cond_broadcast` to wakeup threads waiting via these functions
 *
 *
 * SYNOPSIS:
 *  void tcthread_cond_signal(tcthread_cond_t cond);
 *  void tcthread_cond_broadcast(tcthread_cond_t cond);
 * PARAMETERS:
 *  - cond: condition variable to signal
 * DESCRIPTION:
 *  Signal a condition variable, waking up waiting threads.
 *
 *  `tcthread_cond_signal` wakes up a single waiting thread, whereas
 *  `tcthread_cond_broadcast` wakes up *all* waiting threads. If no threads
 *  are currently waiting, then this operation is a no-op.
 *
 *  It is recommended to unlock relevant wait-mutexes *before* signaling a
 *  condition variable (where possible), in order to reduce the amount of
 *  context-switching between threads.
 *
 * SEE ALSO:
 *  - `tcthread_cond_wait` & `tcthread_cond_timed_wait` to wait on a condition variable (the opposite operation)
 */

#ifndef TC_THREAD_H_
#define TC_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef TC_HINT_NORETURN
#if __STDC_VERSION__ >= 201112L
#define TC_HINT_NORETURN    _Noreturn
#elif defined(__GNUC__)
#define TC_HINT_NORETURN    __attribute__((__noreturn__))
#elif defined(_MSC_VER)
#define TC_HINT_NORETURN    __declspec(noreturn)
#else
#define TC_HINT_NORETURN
#endif
#endif

// this is the only one we need in the header (the rest are defined #ifdef TC_THREAD_IMPLEMENTATION)
#ifndef TC__REINTERPRET_CAST
#ifdef __cplusplus
#define TC__REINTERPRET_CAST(T,v) reinterpret_cast<T>(v)
#else
#define TC__REINTERPRET_CAST(T,v) ((T)(v))
#endif
#endif /* TC__REINTERPRET_CAST */

// wrapped in structs for type-safety, but otherwise raw handles
typedef struct tcthread { uintptr_t handle; } tcthread_t;
typedef struct tcthread_mutex { uintptr_t handle; } tcthread_mutex_t;
typedef struct tcthread_cond { uintptr_t handle; } tcthread_cond_t;
typedef struct tcthread_sem { uintptr_t handle; } tcthread_sem_t;
typedef struct tcthread_rwlock { uintptr_t handle; } tcthread_rwlock_t;

// Util
// Number of logical processor cores. May be higher than physical. Returns 0 if fetching failed.
uint32_t tcthread_get_cpu_count(void);

// Threads
typedef void* tcthread_runner_t(void* udata);
tcthread_t tcthread_create(size_t stack_size, tcthread_runner_t* runner, void* udata);
inline bool tcthread_is_valid(tcthread_t thread) { return thread.handle; }
void tcthread_join(tcthread_t thread, void** retval);
void tcthread_detach(tcthread_t thread);
// works with current thread
TC_HINT_NORETURN void tcthread_exit(void* retval);   // WARNING: May or may not run C++ destructors
void tcthread_sleep(uint32_t ms);
// only works with threads started with `tcthread_create`; TODO: make it work for all
tcthread_t tcthread_self(void);

// Mutex
tcthread_mutex_t tcthread_mutex_create(bool recursive);
inline bool tcthread_mutex_is_valid(tcthread_mutex_t mutex) { return mutex.handle; }
void tcthread_mutex_destroy(tcthread_mutex_t mutex);
void tcthread_mutex_lock(tcthread_mutex_t mutex);
bool tcthread_mutex_try_lock(tcthread_mutex_t mutex);
// timed locks might be a problem in Windows, so I might have to drop this (unless I switch from critical section to a [less performant] mutex)
//bool tcthread_mutex_timed_lock(tcthread_mutex_t mutex, uint32_t timeout_ms);
void tcthread_mutex_unlock(tcthread_mutex_t mutex);

// Condition Variable
tcthread_cond_t tcthread_cond_create(void);
inline bool tcthread_cond_is_valid(tcthread_cond_t cond) { return cond.handle; }
void tcthread_cond_destroy(tcthread_cond_t cond);
void tcthread_cond_wait(tcthread_cond_t cond, tcthread_mutex_t mutex);
bool tcthread_cond_timed_wait(tcthread_cond_t cond, tcthread_mutex_t mutex, uint32_t timeout_ms);
void tcthread_cond_signal(tcthread_cond_t cond);
void tcthread_cond_broadcast(tcthread_cond_t cond);

// Semaphore
tcthread_sem_t tcthread_sem_create(uint32_t initial_value);
inline bool tcthread_sem_is_valid(tcthread_sem_t sem) { return sem.handle; }
void tcthread_sem_destroy(tcthread_sem_t sem);
void tcthread_sem_post(tcthread_sem_t sem);
void tcthread_sem_wait(tcthread_sem_t sem);
bool tcthread_sem_try_wait(tcthread_sem_t sem);
bool tcthread_sem_timed_wait(tcthread_sem_t sem, uint32_t timeout_ms);

// RWLock
tcthread_rwlock_t tcthread_rwlock_create(void);
inline bool tcthread_rwlock_is_valid(tcthread_rwlock_t rwlock) { return rwlock.handle; }
void tcthread_rwlock_destroy(tcthread_rwlock_t rwlock);
void tcthread_rwlock_lock_rd(tcthread_rwlock_t rwlock);
bool tcthread_rwlock_try_lock_rd(tcthread_rwlock_t rwlock);
void tcthread_rwlock_unlock_rd(tcthread_rwlock_t rwlock);
void tcthread_rwlock_lock_wr(tcthread_rwlock_t rwlock);
bool tcthread_rwlock_try_lock_wr(tcthread_rwlock_t rwlock);
void tcthread_rwlock_unlock_wr(tcthread_rwlock_t rwlock);

// ********** ATOMICS **********
typedef uint8_t tcthread_atomicbool_t;  // uses the smallest available atomic type
typedef uint32_t tcthread_atomic32_t;
typedef uintptr_t tcthread_atomicsz_t;
typedef void* tcthread_atomicptr_t;

#if defined(__GNUC__)
#define TCTHREAD_MEMORDER_RELAXED   __ATOMIC_RELAXED
#define TCTHREAD_MEMORDER_CONSUME   __ATOMIC_CONSUME
#define TCTHREAD_MEMORDER_ACQUIRE   __ATOMIC_ACQUIRE
#define TCTHREAD_MEMORDER_RELEASE   __ATOMIC_RELEASE
#define TCTHREAD_MEMORDER_ACQ_REL   __ATOMIC_ACQ_REL
#define TCTHREAD_MEMORDER_SEQ_CST   __ATOMIC_SEQ_CST

// ***** boolean *****
// valid memorder: RELAXED, CONSUME, ACQUIRE, SEQ_CST
inline bool tcthread_atomicbool_load_explicit(volatile tcthread_atomicbool_t* ptr, int memorder) { return __atomic_load_n(ptr, memorder); }
// valid memorder: RELAXED, RELEASE, SEQ_CST
inline void tcthread_atomicbool_store_explicit(volatile tcthread_atomicbool_t* ptr, bool value, int memorder) { __atomic_store_n(ptr, value, memorder); }
// valid memorder: RELAXED, ACQUIRE, RELEASE, ACQ_REL, SEQ_CST
inline bool tcthread_atomicbool_exchange_explicit(volatile tcthread_atomicbool_t* ptr, tcthread_atomicbool_t desired, int memorder) { return __atomic_exchange_n(ptr, desired, memorder); }

// ***** load/store *****
// valid memorder: RELAXED, CONSUME, ACQUIRE, SEQ_CST
inline tcthread_atomic32_t tcthread_atomic32_load_explicit(volatile tcthread_atomic32_t* ptr, int memorder) { return __atomic_load_n(ptr, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_load_explicit(volatile tcthread_atomicsz_t* ptr, int memorder) { return __atomic_load_n(ptr, memorder); }
// valid memorder: RELAXED, RELEASE, SEQ_CST
inline void tcthread_atomic32_store_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder) { __atomic_store_n(ptr, value, memorder); }
inline void tcthread_atomicsz_store_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder) { __atomic_store_n(ptr, value, memorder); }

// ***** (compare-and-)swap *****
// valid memorder: RELAXED, ACQUIRE, RELEASE, ACQ_REL, SEQ_CST
inline tcthread_atomic32_t tcthread_atomic32_exchange_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t desired, int memorder) { return __atomic_exchange_n(ptr, desired, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_exchange_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t desired, int memorder) { return __atomic_exchange_n(ptr, desired, memorder); }
// valid memorder_success: (any)
// valid memorder_failure: RELAXED, CONSUME, ACQUIRE, SEQ_CST; must *not* be stronger than memorder_success
inline tcthread_atomic32_t tcthread_atomic32_compare_exchange_strong_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t expected, tcthread_atomic32_t desired, int memorder_success, int memorder_failure) { __atomic_compare_exchange_n(ptr, &expected, desired, false, memorder_success, memorder_failure); return expected; }
inline tcthread_atomicsz_t tcthread_atomicsz_compare_exchange_strong_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t expected, tcthread_atomicsz_t desired, int memorder_success, int memorder_failure) { __atomic_compare_exchange_n(ptr, &expected, desired, false, memorder_success, memorder_failure); return expected; }
inline tcthread_atomic32_t tcthread_atomic32_compare_exchange_weak_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t expected, tcthread_atomic32_t desired, int memorder_success, int memorder_failure) { __atomic_compare_exchange_n(ptr, &expected, desired, true, memorder_success, memorder_failure); return expected; }
inline tcthread_atomicsz_t tcthread_atomicsz_compare_exchange_weak_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t expected, tcthread_atomicsz_t desired, int memorder_success, int memorder_failure) { __atomic_compare_exchange_n(ptr, &expected, desired, true, memorder_success, memorder_failure); return expected; }

// ***** arithmetic & bitwise *****
// valid memorder: (any)
inline tcthread_atomic32_t tcthread_atomic32_fetch_add_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder) { return __atomic_fetch_add(ptr, value, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_add_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder) { return __atomic_fetch_add(ptr, value, memorder); }
inline tcthread_atomic32_t tcthread_atomic32_fetch_sub_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder) { return __atomic_fetch_sub(ptr, value, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_sub_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder) { return __atomic_fetch_sub(ptr, value, memorder); }
inline tcthread_atomic32_t tcthread_atomic32_fetch_and_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder) { return __atomic_fetch_and(ptr, value, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_and_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder) { return __atomic_fetch_and(ptr, value, memorder); }
inline tcthread_atomic32_t tcthread_atomic32_fetch_xor_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder) { return __atomic_fetch_xor(ptr, value, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_xor_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder) { return __atomic_fetch_xor(ptr, value, memorder); }
inline tcthread_atomic32_t tcthread_atomic32_fetch_or_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder) { return __atomic_fetch_or(ptr, value, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_or_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder) { return __atomic_fetch_or(ptr, value, memorder); }
// GCC additionally supports `nand` ... should we expose it?

// ***** increment & decrement; these return the *new* value *****
// valid memorder: (any)
inline tcthread_atomic32_t tcthread_atomic32_inc_explicit(volatile tcthread_atomic32_t* ptr, int memorder) { return __atomic_add_fetch(ptr, 1, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_inc_explicit(volatile tcthread_atomicsz_t* ptr, int memorder) { return __atomic_add_fetch(ptr, 1, memorder); }
inline tcthread_atomic32_t tcthread_atomic32_dec_explicit(volatile tcthread_atomic32_t* ptr, int memorder) { return __atomic_sub_fetch(ptr, 1, memorder); }
inline tcthread_atomicsz_t tcthread_atomicsz_dec_explicit(volatile tcthread_atomicsz_t* ptr, int memorder) { return __atomic_sub_fetch(ptr, 1, memorder); }

#elif defined(_MSC_VER)
/*
 * Values are assigned so that ORing provides the "stronger" combination.
 * DO NOT RELY ON THAT, it is merely used to simplify some internal logic and is
 * subject to change without notice.
 */
// RELAXED = 0
// CONSUME = 1 | RELAXED (== 1)
// ACQUIRE = 2 | CONSUME (== 3)
// RELEASE = 4 | RELAXED (== 4)
// ACQ_REL = ACQUIRE | RELEASE (== 7)
// SEQ_CST = 8 | ACQ_REL (== 15)
#define TCTHREAD_MEMORDER_RELAXED   0x0
#define TCTHREAD_MEMORDER_CONSUME   0x1
#define TCTHREAD_MEMORDER_ACQUIRE   0x3
#define TCTHREAD_MEMORDER_RELEASE   0x4
#define TCTHREAD_MEMORDER_ACQ_REL   0x7
#define TCTHREAD_MEMORDER_SEQ_CST   0xF

// ***** boolean *****
// valid memorder: RELAXED, CONSUME, ACQUIRE, SEQ_CST
inline bool tcthread_atomicbool_load_explicit(volatile tcthread_atomicbool_t* ptr, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedCompareExchange8_nf((volatile char*)ptr, 0, 0);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedCompareExchange8_acq((volatile char*)ptr, 0, 0);
#endif
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedCompareExchange8((volatile char*)ptr, 0, 0);
    }
    __assume(0);    // unreachable
    //return 0;
}
// valid memorder: RELAXED, RELEASE, SEQ_CST
inline void tcthread_atomicbool_store_explicit(volatile tcthread_atomicbool_t* ptr, bool value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        _InterlockedExchange8_nf((volatile char*)ptr, value);
        return;
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_ARM) || defined(_M_ARM64)
        _InterlockedExchange8_rel((volatile char*)ptr, value);
        return;
#endif
    case TCTHREAD_MEMORDER_SEQ_CST:
        _InterlockedExchange8((volatile char*)ptr, value);
        return;
    }
    __assume(0);    // unreachable
}
// valid memorder: RELAXED, ACQUIRE, RELEASE, ACQ_REL, SEQ_CST
inline bool tcthread_atomicbool_exchange_explicit(volatile tcthread_atomicbool_t* ptr, tcthread_atomicbool_t desired, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchange8_nf((volatile char*)ptr, desired);
#endif
    //case TCTHREAD_MEMORDER_CONSUME:   // seems to be unallowed
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchange8_acq((volatile char*)ptr, desired);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchange8_rel((volatile char*)ptr, desired);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedExchange8((volatile char*)ptr, desired);
    }
    __assume(0);    // unreachable
    //return 0;
}

// ***** load/store *****
// valid memorder: RELAXED, CONSUME, ACQUIRE, SEQ_CST
inline tcthread_atomic32_t tcthread_atomic32_load_explicit(volatile tcthread_atomic32_t* ptr, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
        // strangely, there is no _InterlockedCompareExchange_nf on ARM
#if defined(_M_ARM)
        // On 32-bit, we can simulate it with the same operation on pointers.
        return (tcthread_atomic32_t)_InterlockedCompareExchangePointer_nf((void* volatile*)ptr, NULL, NULL);
#elif defined(_M_ARM64)
        // On 64-bit, we're out of luck. Let's try an atomic `or`.
        // (TODO: Should we simply fallthrough to _acq here?)
        return _InterlockedOr_nf((volatile long*)ptr, 0, 0);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedCompareExchange_HLEAcquire((volatile long*)ptr, 0, 0);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedCompareExchange_acq((volatile long*)ptr, 0, 0);
#endif
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedCompareExchange((volatile long*)ptr, 0, 0);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_load_explicit(volatile tcthread_atomicsz_t* ptr, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_nf(ptr, NULL, NULL);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_HLEAcquire(ptr, NULL, NULL);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_acq(ptr, NULL, NULL);
#endif
    case TCTHREAD_MEMORDER_SEQ_CST:
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer(ptr, NULL, NULL);
    }
    __assume(0);    // unreachable
    //return 0;
}
// valid memorder: RELAXED, RELEASE, SEQ_CST
inline void tcthread_atomic32_store_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        _InterlockedExchange_nf((volatile long*)ptr, value);
        return;
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        _InterlockedExchange_HLERelease((volatile long*)ptr, value);
        return;
#elif defined(_M_ARM) || defined(_M_ARM64)
        _InterlockedExchange_rel((volatile long*)ptr, value);
        return;
#endif
    case TCTHREAD_MEMORDER_SEQ_CST:
        _InterlockedExchange((volatile long*)ptr, value);
        return;
    }
    __assume(0);    // unreachable
}
inline void tcthread_atomicsz_store_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        _InterlockedExchangePointer_nf(ptr, (void*)value);
        return;
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
        // x86 has no _InterlockedExchangePointer_HLERelease for some reason, but it has _InterlockedExchange_HLERelease
        _InterlockedExchange_HLERelease((volatile long*)ptr, value);
        return;
#elif defined(_M_AMD64)
        _InterlockedExchangePointer_HLERelease(ptr, (void*)value);
        return;
#elif defined(_M_ARM) || defined(_M_ARM64)
        _InterlockedExchangePointer_rel(ptr, (void*)value);
        return;
#endif
    case TCTHREAD_MEMORDER_SEQ_CST:
        _InterlockedExchangePointer(ptr, (void*)value);
        return;
    }
    __assume(0);    // unreachable
}

// ***** (compare-and-)swap *****
// valid memorder: RELAXED, ACQUIRE, RELEASE, ACQ_REL, SEQ_CST
inline tcthread_atomic32_t tcthread_atomic32_exchange_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t desired, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchange_nf((volatile long*)ptr, desired);
#endif
    //case TCTHREAD_MEMORDER_CONSUME:   // seems to be unallowed
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedExchange_HLEAcquire((volatile long*)ptr, desired);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchange_acq((volatile long*)ptr, desired);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedExchange_HLERelease((volatile long*)ptr, desired);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchange_rel((volatile long*)ptr, desired);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedExchange((volatile long*)ptr, desired);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_exchange_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t desired, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedExchangePointer_nf(ptr, (void*)desired);
#endif
    //case TCTHREAD_MEMORDER_CONSUME:   // seems to be unallowed
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return (tcthread_atomicsz_t)_InterlockedExchangePointer_HLEAcquire(ptr, (void*)desired);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedExchangePointer_acq(ptr, (void*)desired);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
        // x86 has no _InterlockedExchangePointer_HLERelease for some reason, but it has _InterlockedExchange_HLERelease
        return _InterlockedExchange_HLERelease((volatile long*)ptr, desired);
#elif defined(_M_AMD64)
        return (tcthread_atomicsz_t)_InterlockedExchangePointer_HLERelease(ptr, (void*)desired);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedExchangePointer_rel(ptr, (void*)desired);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return (tcthread_atomicsz_t)_InterlockedExchangePointer(ptr, (void*)desired);
    }
    __assume(0);    // unreachable
    //return 0;
}
// valid memorder_success: (any)
// valid memorder_failure: RELAXED, CONSUME, ACQUIRE, SEQ_CST; must *not* be stronger than memorder_success
inline tcthread_atomic32_t tcthread_atomic32_compare_exchange_strong_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t expected, tcthread_atomic32_t desired, int memorder_success, int memorder_failure)
{
    switch(memorder_success | memorder_failure)
    {
    case TCTHREAD_MEMORDER_RELAXED:
    // strangely, there is no _InterlockedCompareExchange_nf on ARM
#if defined(_M_ARM)
    // On 32-bit, we can simulate it with the same operation on pointers.
    return (tcthread_atomic32_t)_InterlockedCompareExchangePointer_nf((void* volatile*)ptr, (void*)desired, (void*)expected);
//#elif defined(_M_ARM64) // On 64-bit, we're out of luck. Fallthrough to ACQUIRE
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedCompareExchange_HLEAcquire((volatile long*)ptr, desired, expected);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedCompareExchange_acq((volatile long*)ptr, desired, expected);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedCompareExchange_HLERelease((volatile long*)ptr, desired, expected);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedCompareExchange_rel((volatile long*)ptr, desired, expected);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedCompareExchange((volatile long*)ptr, desired, expected);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_compare_exchange_strong_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t expected, tcthread_atomicsz_t desired, int memorder_success, int memorder_failure)
{
    switch(memorder_success | memorder_failure)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_nf(ptr, (void*)desired, (void*)expected);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_HLEAcquire(ptr, (void*)desired, (void*)expected);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_acq(ptr, (void*)desired, (void*)expected);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_HLERelease(ptr, (void*)desired, (void*)expected);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer_rel(ptr, (void*)desired, (void*)expected);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return (tcthread_atomicsz_t)_InterlockedCompareExchangePointer(ptr, (void*)desired, (void*)expected);
    }
    __assume(0);    // unreachable
    //return 0;
}
// MSVC has no `weak` vs `strong` semantics
inline tcthread_atomic32_t tcthread_atomic32_compare_exchange_weak_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t expected, tcthread_atomic32_t desired, int memorder_success, int memorder_failure)
{
    return tcthread_atomic32_compare_exchange_strong_explicit(ptr, expected, desired, memorder_success, memorder_failure);
}
inline tcthread_atomicsz_t tcthread_atomicsz_compare_exchange_weak_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t expected, tcthread_atomicsz_t desired, int memorder_success, int memorder_failure)
{
    return tcthread_atomicsz_compare_exchange_strong_explicit(ptr, expected, desired, memorder_success, memorder_failure);
}

// ***** arithmetic & bitwise *****
// valid memorder: (any)
inline tcthread_atomic32_t tcthread_atomic32_fetch_add_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchangeAdd_nf((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedExchangeAdd_HLEAcquire((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchangeAdd_acq((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedExchangeAdd_HLERelease((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedExchangeAdd_rel((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedExchangeAdd((volatile long*)ptr, value);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_add_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM)
        return _InterlockedExchangeAdd_nf(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedExchangeAdd64_nf(ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86)
        return _InterlockedExchangeAdd_HLEAcquire(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedExchangeAdd64_HLEAcquire(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedExchangeAdd_acq(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedExchangeAdd64_acq(ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
        return _InterlockedExchangeAdd_HLERelease(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedExchangeAdd64_HLERelease(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedExchangeAdd_rel(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedExchangeAdd64_rel(ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
#if defined(_M_IX86) || defined(_M_ARM)
        return _InterlockedExchangeAdd(ptr, value);
#elif defined(_M_AMD64) || defined(_M_ARM64)
        return _InterlockedExchangeAdd64(ptr, value);
#else
#error "Unknown CPU architecture"
#endif
    }
    __assume(0);    // unreachable
    //return 0;
}
#pragma warning( push )
// Disable "C4146: unary minus operator applied to unsigned type, result still unsigned"
#pragma warning( disable : 4146 )
inline tcthread_atomic32_t tcthread_atomic32_fetch_sub_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder)
{
    return tcthread_atomic32_fetch_sub_explicit(ptr, -value, memorder);
}
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_sub_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder)
{
    return tcthread_atomicsz_fetch_add_explicit(ptr, -value, memorder);
}
#pragma warning( pop )
inline tcthread_atomic32_t tcthread_atomic32_fetch_and_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedAnd_nf((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedAnd_HLEAcquire((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedAnd_acq((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedAnd_HLERelease((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedAnd_rel((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedAnd((volatile long*)ptr, value);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_and_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM)
        return _InterlockedAnd_nf(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedAnd64_nf(ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86)
        return _InterlockedAnd_HLEAcquire(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedAnd64_HLEAcquire(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedAnd_acq(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedAnd64_acq(ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
        return _InterlockedAnd_HLERelease(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedAnd64_HLERelease(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedAnd_rel(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedAnd64_rel(ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
#if defined(_M_IX86) || defined(_M_ARM)
        return _InterlockedAnd(ptr, value);
#elif defined(_M_AMD64) || defined(_M_ARM64)
        return _InterlockedAnd64(ptr, value);
#else
#error "Unknown CPU architecture"
#endif
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomic32_t tcthread_atomic32_fetch_xor_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedXor_nf((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedXor_HLEAcquire((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedXor_acq((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedXor_HLERelease((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedXor_rel((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedXor((volatile long*)ptr, value);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_xor_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM)
        return _InterlockedXor_nf(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedXor64_nf(ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86)
        return _InterlockedXor_HLEAcquire(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedXor64_HLEAcquire(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedXor_acq(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedXor64_acq(ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
        return _InterlockedXor_HLERelease(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedXor64_HLERelease(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedXor_rel(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedXor64_rel(ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
#if defined(_M_IX86) || defined(_M_ARM)
        return _InterlockedXor(ptr, value);
#elif defined(_M_AMD64) || defined(_M_ARM64)
        return _InterlockedXor64(ptr, value);
#else
#error "Unknown CPU architecture"
#endif
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomic32_t tcthread_atomic32_fetch_or_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedOr_nf((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedOr_HLEAcquire((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedOr_acq((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        return _InterlockedOr_HLERelease((volatile long*)ptr, value);
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedOr_rel((volatile long*)ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedOr((volatile long*)ptr, value);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_fetch_or_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM)
        return _InterlockedOr_nf(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedOr64_nf(ptr, value);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86)
        return _InterlockedOr_HLEAcquire(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedOr64_HLEAcquire(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedOr_acq(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedOr64_acq(ptr, value);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
        return _InterlockedOr_HLERelease(ptr, value);
#elif defined(_M_AMD64)
        return _InterlockedOr64_HLERelease(ptr, value);
#elif defined(_M_ARM)
        return _InterlockedOr_rel(ptr, value);
#elif defined(_M_ARM64)
        return _InterlockedOr64_rel(ptr, value);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
#if defined(_M_IX86) || defined(_M_ARM)
        return _InterlockedOr(ptr, value);
#elif defined(_M_AMD64) || defined(_M_ARM64)
        return _InterlockedOr64(ptr, value);
#else
#error "Unknown CPU architecture"
#endif
    }
    __assume(0);    // unreachable
    //return 0;
}

// ***** increment & decrement *****
// valid memorder: (any)
inline tcthread_atomic32_t tcthread_atomic32_inc_explicit(volatile tcthread_atomic32_t* ptr, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedIncrement_nf((volatile long*)ptr);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        // _InterlockedIncrement_HLEAcquire does not exist, simulate with ExchangeAdd
        return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLEAcquire((volatile long*)ptr, 1) + 1u;
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedIncrement_acq((volatile long*)ptr);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        // _InterlockedIncrement_HLERelease does not exist, simulate with ExchangeAdd
        return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLERelease((volatile long*)ptr, 1) + 1u;
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedIncrement_rel((volatile long*)ptr);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedIncrement((volatile long*)ptr);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_inc_explicit(volatile tcthread_atomicsz_t* ptr, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM)
        return _InterlockedIncrement_nf(ptr);
#elif defined(_M_ARM64)
        return _InterlockedIncrement64_nf(ptr);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86)
    // _InterlockedIncrement_HLEAcquire does not exist, simulate with ExchangeAdd
    return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLEAcquire((volatile long*)ptr, 1) + 1u;
#elif defined(_M_AMD64)
    // _InterlockedIncrement64_HLEAcquire does not exist, simulate with ExchangeAdd64
    return (tcthread_atomicsz_t)_InterlockedExchangeAdd64_HLEAcquire((volatile __int64*)ptr, 1) + 1u;
#elif defined(_M_ARM)
        return _InterlockedIncrement_acq(ptr);
#elif defined(_M_ARM64)
        return _InterlockedIncrement64_acq(ptr);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
    // _InterlockedIncrement_HLERelease does not exist, simulate with ExchangeAdd
    return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLERelease((volatile long*)ptr, 1) + 1u;
#elif defined(_M_AMD64)
    // _InterlockedIncrement64_HLERelease does not exist, simulate with ExchangeAdd64
    return (tcthread_atomicsz_t)_InterlockedExchangeAdd64_HLERelease((volatile __int64*)ptr, 1) + 1u;
#elif defined(_M_ARM)
        return _InterlockedIncrement_rel(ptr);
#elif defined(_M_ARM64)
        return _InterlockedIncrement64_rel(ptr);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
#if defined(_M_IX86) || defined(_M_ARM)
        return _InterlockedIncrement(ptr);
#elif defined(_M_AMD64) || defined(_M_ARM64)
        return _InterlockedIncrement64(ptr);
#else
#error "Unknown CPU architecture"
#endif
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomic32_t tcthread_atomic32_dec_explicit(volatile tcthread_atomic32_t* ptr, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedDecrement_nf((volatile long*)ptr);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86) || defined(_M_AMD64)
        // _InterlockedDecrement_HLEAcquire does not exist, simulate with ExchangeAdd
        return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLEAcquire((volatile long*)ptr, -1) - 1u;
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedDecrement_acq((volatile long*)ptr);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86) || defined(_M_AMD64)
        // _InterlockedDecrement_HLERelease does not exist, simulate with ExchangeAdd
        return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLERelease((volatile long*)ptr, -1) - 1u;
#elif defined(_M_ARM) || defined(_M_ARM64)
        return _InterlockedDecrement_rel((volatile long*)ptr);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
        return _InterlockedDecrement((volatile long*)ptr);
    }
    __assume(0);    // unreachable
    //return 0;
}
inline tcthread_atomicsz_t tcthread_atomicsz_dec_explicit(volatile tcthread_atomicsz_t* ptr, int memorder)
{
    switch(memorder)
    {
    case TCTHREAD_MEMORDER_RELAXED:
#if defined(_M_ARM)
        return _InterlockedDecrement_nf(ptr);
#elif defined(_M_ARM64)
        return _InterlockedDecrement64_nf(ptr);
#endif
    case TCTHREAD_MEMORDER_CONSUME:
    case TCTHREAD_MEMORDER_ACQUIRE:
#if defined(_M_IX86)
    // _InterlockedDecrement_HLEAcquire does not exist, simulate with ExchangeAdd
    return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLEAcquire((volatile long*)ptr, -1) - 1u;
#elif defined(_M_AMD64)
    // _InterlockedDecrement64_HLEAcquire does not exist, simulate with ExchangeAdd64
    return (tcthread_atomicsz_t)_InterlockedExchangeAdd64_HLEAcquire((volatile __int64*)ptr, -1) - 1u;
#elif defined(_M_ARM)
        return _InterlockedDecrement_acq(ptr);
#elif defined(_M_ARM64)
        return _InterlockedDecrement64_acq(ptr);
#endif
    case TCTHREAD_MEMORDER_RELEASE:
#if defined(_M_IX86)
    // _InterlockedDecrement_HLERelease does not exist, simulate with ExchangeAdd
    return (tcthread_atomic32_t)_InterlockedExchangeAdd_HLERelease((volatile long*)ptr, -1) - 1u;
#elif defined(_M_AMD64)
    // _InterlockedDecrement64_HLERelease does not exist, simulate with ExchangeAdd64
    return (tcthread_atomicsz_t)_InterlockedExchangeAdd64_HLERelease((volatile __int64*)ptr, -1) - 1u;
#elif defined(_M_ARM)
        return _InterlockedDecrement_rel(ptr);
#elif defined(_M_ARM64)
        return _InterlockedDecrement64_rel(ptr);
#endif
    case TCTHREAD_MEMORDER_ACQ_REL:
    case TCTHREAD_MEMORDER_SEQ_CST:
#if defined(_M_IX86) || defined(_M_ARM)
        return _InterlockedDecrement(ptr);
#elif defined(_M_AMD64) || defined(_M_ARM64)
        return _InterlockedDecrement64(ptr);
#else
#error "Unknown CPU architecture"
#endif
    }
    __assume(0);    // unreachable
    //return 0;
}
#else
#pragma message ("Warning: Atomics not supported")
#endif

// ********** ATOMICS: Wrappers **********

// ***** atomicptr (wrappers around `atomicsz`) *****
inline tcthread_atomicptr_t tcthread_atomicptr_load_explicit(volatile tcthread_atomicptr_t* ptr, int memorder)
{
    return TC__REINTERPRET_CAST(tcthread_atomicptr_t, tcthread_atomicsz_load_explicit(
            TC__REINTERPRET_CAST(volatile tcthread_atomicsz_t*, ptr),
            memorder
    ));
}
inline void tcthread_atomicptr_store_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t value, int memorder)
{
    tcthread_atomicsz_store_explicit(
        TC__REINTERPRET_CAST(volatile tcthread_atomicsz_t*, ptr),
        TC__REINTERPRET_CAST(tcthread_atomicsz_t, value),
        memorder
    );
}
inline tcthread_atomicptr_t tcthread_atomicptr_exchange_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t desired, int memorder)
{
    return TC__REINTERPRET_CAST(tcthread_atomicptr_t, tcthread_atomicsz_exchange_explicit(
        TC__REINTERPRET_CAST(volatile tcthread_atomicsz_t*, ptr),
        TC__REINTERPRET_CAST(tcthread_atomicsz_t, desired),
        memorder
    ));
}
inline tcthread_atomicptr_t tcthread_atomicptr_compare_exchange_strong_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t expected, tcthread_atomicptr_t desired, int memorder_success, int memorder_failure)
{
    return TC__REINTERPRET_CAST(tcthread_atomicptr_t, tcthread_atomicsz_compare_exchange_strong_explicit(
        TC__REINTERPRET_CAST(volatile tcthread_atomicsz_t*, ptr),
        TC__REINTERPRET_CAST(tcthread_atomicsz_t, expected),
        TC__REINTERPRET_CAST(tcthread_atomicsz_t, desired),
        memorder_success,
        memorder_failure
    ));
}
inline tcthread_atomicptr_t tcthread_atomicptr_compare_exchange_weak_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t expected, tcthread_atomicptr_t desired, int memorder_success, int memorder_failure)
{
    return TC__REINTERPRET_CAST(tcthread_atomicptr_t, tcthread_atomicsz_compare_exchange_weak_explicit(
        TC__REINTERPRET_CAST(volatile tcthread_atomicsz_t*, ptr),
        TC__REINTERPRET_CAST(tcthread_atomicsz_t, expected),
        TC__REINTERPRET_CAST(tcthread_atomicsz_t, desired),
        memorder_success,
        memorder_failure
    ));
}

#define tcthread_atomicptr_load(ptr)                                        tcthread_atomicptr_load_explicit(ptr, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicptr_store(ptr, value)                                tcthread_atomicptr_store_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicptr_exchange(ptr, desired)                           tcthread_atomicptr_exchange_explicit(ptr, desired, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicptr_compare_exchange_strong(ptr, expected, desired)  tcthread_atomicptr_compare_exchange_strong_explicit(ptr, expected, desired, TCTHREAD_MEMORDER_SEQ_CST, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicptr_compare_exchange_weak(ptr, expected, desired)    tcthread_atomicptr_compare_exchange_weak_explicit(ptr, expected, desired, TCTHREAD_MEMORDER_SEQ_CST, TCTHREAD_MEMORDER_SEQ_CST)

// ***** boolean *****
#define tcthread_atomicbool_load(ptr)                                       tcthread_atomicbool_load_explicit(pre, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicbool_store(ptr, value)                               tcthread_atomicbool_store_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicbool_exchange(ptr, desired)                          tcthread_atomicbool_exchange_explicit(ptr, desired, TCTHREAD_MEMORDER_SEQ_CST)

// ***** store/load *****
#define tcthread_atomic32_load(ptr)                                         tcthread_atomic32_load_explicit(ptr, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_load(ptr)                                         tcthread_atomicsz_load_explicit(ptr, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_store(ptr, value)                                 tcthread_atomic32_store_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_store(ptr, value)                                 tcthread_atomicsz_store_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)

// ***** (compare-and-)swap *****
#define tcthread_atomic32_exchange(ptr, desired)                            tcthread_atomic32_exchange_explicit(ptr, desired, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_exchange(ptr, desired)                            tcthread_atomicsz_exchange_explicit(ptr, desired, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_compare_exchange_strong(ptr, expected, desired)   tcthread_atomic32_compare_exchange_strong_explicit(ptr, expected, desired, TCTHREAD_MEMORDER_SEQ_CST, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_compare_exchange_strong(ptr, expected, desired)   tcthread_atomicsz_compare_exchange_strong_explicit(ptr, expected, desired, TCTHREAD_MEMORDER_SEQ_CST, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_compare_exchange_weak(ptr, expected, desired)     tcthread_atomic32_compare_exchange_weak_explicit(ptr, expected, desired, TCTHREAD_MEMORDER_SEQ_CST, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_compare_exchange_weak(ptr, expected, desired)     tcthread_atomicsz_compare_exchange_weak_explicit(ptr, expected, desired, TCTHREAD_MEMORDER_SEQ_CST, TCTHREAD_MEMORDER_SEQ_CST)

// ***** arithmetic & bitwise *****
#define tcthread_atomic32_fetch_add(ptr, value) tcthread_atomic32_fetch_add_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_fetch_add(ptr, value) tcthread_atomicsz_fetch_add_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_fetch_sub(ptr, value) tcthread_atomic32_fetch_sub_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_fetch_sub(ptr, value) tcthread_atomicsz_fetch_sub_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_fetch_and(ptr, value) tcthread_atomic32_fetch_and_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_fetch_and(ptr, value) tcthread_atomicsz_fetch_and_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_fetch_xor(ptr, value) tcthread_atomic32_fetch_xor_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_fetch_xor(ptr, value) tcthread_atomicsz_fetch_xor_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_fetch_or(ptr, value)  tcthread_atomic32_fetch_or_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_fetch_or(ptr, value)  tcthread_atomicsz_fetch_or_explicit(ptr, value, TCTHREAD_MEMORDER_SEQ_CST)

// ***** increment & decrement; these return the *new* value *****
#define tcthread_atomic32_inc(ptr)  tcthread_atomic32_inc_explicit(ptr, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_inc(ptr)  tcthread_atomicsz_inc_explicit(ptr, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomic32_dec(ptr)  tcthread_atomic32_dec_explicit(ptr, TCTHREAD_MEMORDER_SEQ_CST)
#define tcthread_atomicsz_dec(ptr)  tcthread_atomicsz_dec_explicit(ptr, TCTHREAD_MEMORDER_SEQ_CST)

// ***** reference-counting *****
/*
 * These helpers use memory order appropriate for the respective operations. The
 * intent is to have something that "just works" for this (very) common use-case
 * without having to think about what memory order is appropriate.
 *
 * - incref uses RELAXED memory order (changes might not be visible immediately)
 *   => also provides no return value, since it is mostly meaningless (due to memory order)
 * - decref uses ACQ_REL memory order (changes immediate)
 * - decref_lazy uses RELAXED memory order
 *   (it is intended to be used with instances where refcount must be decreased, but we don't yet want to free the data)
 *   => no return value for same reason as incref
 * - initref uses REL memory order (to ensure that initialization is propagated)
 * - loadref uses ACQ memory order (to ensure that changes are visible)
 */
#define tcthread_atomic32_incref(ptr)           ((void)tcthread_atomic32_inc_explicit(ptr, TCTHREAD_MEMORDER_RELAXED))
#define tcthread_atomic32_decref(ptr)           tcthread_atomic32_dec_explicit(ptr, TCTHREAD_MEMORDER_ACQ_REL)
#define tcthread_atomic32_decref_lazy(ptr)      ((void)tcthread_atomic32_dec_explicit(ptr, TCTHREAD_MEMORDER_RELAXED))
#define tcthread_atomic32_initref(ptr, value)   tcthread_atomic32_store_explicit(ptr, value, TCTHREAD_MEMORDER_RELEASE)
#define tcthread_atomic32_loadref(ptr)          tcthread_atomic32_load_explicit(ptr, TCTHREAD_MEMORDER_ACQUIRE)

#ifdef __cplusplus
}
#endif

#endif /* TC_THREAD_H_ */



#ifdef TC_THREAD_IMPLEMENTATION
#undef TC_THREAD_IMPLEMENTATION
#include <stdlib.h>

#ifndef TC__PARAM_UNUSED
#define TC__PARAM_UNUSED(variable)    (void)(variable)
#endif /* TC__PARAM_UNUSED */

#ifndef TC__STATIC_CAST
#ifdef __cplusplus
#define TC__STATIC_CAST(T,v) static_cast<T>(v)
#else
#define TC__STATIC_CAST(T,v) ((T)(v))
#endif
#endif /* TC__STATIC_CAST */

/* no cast done to preserve undefined function warnings in C */
#ifndef TC__VOID_CAST
#ifdef __cplusplus
#define TC__VOID_CAST(T,v)  TC__STATIC_CAST(T,v)
#else
#define TC__VOID_CAST(T,v)  (v)
#endif
#endif /* TC__VOID_CAST */

#ifndef TC_ASSERT
#include <assert.h>
#define TC_ASSERT(x, message)   assert((x) && (message))
#endif
#ifndef TC_MALLOC
#define TC_MALLOC(size)         malloc(size)
#endif /* TC_MALLOC */
#ifndef TC_FREE
#define TC_FREE(ptr)            free(ptr)
#endif /* TC_FREE */

#ifdef _WIN32
#define TCTHREAD__PLATFORM_WINDOWS
#elif (defined(__unix__) || defined(unix)) && !defined(USG)
#define TCTHREAD__PLATFORM_POSIX
#include <sys/param.h>
#if defined(BSD) && (BSD >= 199103)
#define TCTHREAD__PLATFORM_POSIX_BSD
#endif
#else
#error "Unknown platform"
#endif

#if defined(TCTHREAD__PLATFORM_WINDOWS)
#if _WIN32_WINNT < 0x600    // ensure _WIN32_WINNT is at least 0x600 (Vista)
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>
#include <processthreadsapi.h>

static INIT_ONCE tcthread_init_once_ = INIT_ONCE_STATIC_INIT;
static DWORD tcthread_tls_index_ = TLS_OUT_OF_INDEXES;
static BOOL CALLBACK tcthread_init_once_runner_(PINIT_ONCE init_once, PVOID param, PVOID* context)
{
    TC__PARAM_UNUSED(init_once);
    TC__PARAM_UNUSED(param);
    tcthread_tls_index_ = TlsAlloc();
    *context = NULL;    // must be assigned, though we don't use it
    return tcthread_tls_index_ != TLS_OUT_OF_INDEXES;
    // TODO: TlsFree() at exit?
}
struct tcthread_internal_
{
    HANDLE handle;
    void* data;
    tcthread_runner_t* runner;
    tcthread_atomicbool_t done_or_detached;
};
static void tcthread_before_exit_(struct tcthread_internal_* internal, void* retval)
{
    internal->data = retval;
    // test-and-set; if this was already set, then thread was detached, and we can free its data
    if(tcthread_atomicbool_exchange_explicit(&internal->done_or_detached, true, TCTHREAD_MEMORDER_ACQ_REL))
        TC_FREE(internal);
}
static unsigned __stdcall tcthread_create_runner_(LPVOID ptr)
{
    struct tcthread_internal_* internal = TC__VOID_CAST(struct tcthread_internal_*, ptr);
    if(!TlsSetValue(tcthread_tls_index_, internal))
    {
        // TODO: Should we handle this to report error in tcthread_create itself?
        // (but it needs locking in said function)
        //return 1;
    }
    tcthread_before_exit_(internal, internal->runner(internal->data));
    return 0;
}
typedef DWORD winapi_GetActiveProcessorCount_t(WORD);
union winapi_GetActiveProcessorCount_conv
{
    void* void_ptr;
    winapi_GetActiveProcessorCount_t* proc;
    FARPROC farproc;
};
#elif defined(TCTHREAD__PLATFORM_POSIX)

#if TCTHREAD__PLATFORM_POSIX_BSD
#include <sys/sysctl.h> // for sysctl
#else
#include <unistd.h>     // for sysconf
#endif

#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
static pthread_once_t tcthread_init_once_ = PTHREAD_ONCE_INIT;
static pthread_key_t tcthread_tls_key_;
static void tcthread_init_once_runner_(void)
{
    pthread_key_create(&tcthread_tls_key_, NULL);   // can fail, but what can we do ...
}
struct tcthread_internal_
{
    pthread_t handle;
    void* data;
    tcthread_runner_t* runner;
    pthread_mutex_t mutex;
    tcthread_atomicbool_t done_or_detached;
};
static void tcthread_before_exit_(struct tcthread_internal_* internal)
{
    // test-and-set; if this was already set, then thread was detached, and we can free its data
    if(tcthread_atomicbool_exchange_explicit(&internal->done_or_detached, true, TCTHREAD_MEMORDER_ACQ_REL))
        TC_FREE(internal);
}
static void* tcthread_create_runner_(void* ptr)
{
    struct tcthread_internal_* internal = TC__VOID_CAST(struct tcthread_internal_*, ptr);
    if(!pthread_setspecific(tcthread_tls_key_, internal))
    {
        // TODO: Should we handle this to report error in tcthread_create itself?
        // (but it needs locking in said function)
        //return NULL;
    }
    // mutex starts locked (by parent thread), so this acts as a simple initialization barrier
    // these can fail, but again, what do we do about it?
    pthread_mutex_lock(&internal->mutex);
    pthread_mutex_unlock(&internal->mutex);
    void* retval = internal->runner(internal->data);
    tcthread_before_exit_(internal);
    return retval;
}
static struct timespec tcthread_timespec_from_ms_(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000u;
    ts.tv_nsec = (ms % 1000u) * 1000000u;
    return ts;
}
#else
#error "Unknown platform"
#endif

#include <stdio.h>
// Util
// https://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
uint32_t tcthread_get_cpu_count(void)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    // The first option needs Windows 7 (but we want to support Vista & Server 2008).
    // It is an improvement over the latter in that it properly handles # of cores larger than 64.

    static volatile union winapi_GetActiveProcessorCount_conv winapi_GetActiveProcessorCount;
    // (used as a boolean, but InterlockedOr8 is not supported in MinGW --- InterlockedOr is)
    static volatile LONG winapi_GetActiveProcessorCount_load_attempted = 0;
    // load `GetActiveProcessorCount`; done at runtime in case we're running an older version of Vista
    if(!winapi_GetActiveProcessorCount_load_attempted)
    {
        // (yes, it's a race condition in that it may run more than once; that's perfectly fine)
        // get the `kernel32` module
        HMODULE lib_kernel32 = LoadLibraryW(L"kernel32.dll");
        if(lib_kernel32)
        {
            union winapi_GetActiveProcessorCount_conv conv;
            // GetActiveProcessorCount is what we want out of it
            conv.farproc = GetProcAddress(lib_kernel32, "GetActiveProcessorCount");
            // if InterlockedCompareExchangePointer fails (= procedure was already loaded), we free the lib_kernel32, to maintain the correct refcount
            if(conv.farproc && InterlockedCompareExchangePointer(&winapi_GetActiveProcessorCount.void_ptr, conv.void_ptr, NULL))
                FreeLibrary(lib_kernel32);
        }
        // mark that an attempt was completed, so that we don't keep trying if this fails (for performance reasons)
        InterlockedOr(&winapi_GetActiveProcessorCount_load_attempted, 1);
    }
    // finally, read the value
    winapi_GetActiveProcessorCount_t* proc = winapi_GetActiveProcessorCount.proc;
    if(proc)
        return proc(ALL_PROCESSOR_GROUPS);

    // failed to load the procedure; continue to fallback
    // GetSystemInfo only supports up to 32-64 cores (depending on architecture), which is why it's used only as a fallback
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return system_info.dwNumberOfProcessors;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    // modern BSDs support sysconf(), but older ones do not
    // the same holds for OS X
#if defined(TCTHREAD__PLATFORM_POSIX_BSD) && (defined(HW_AVAILCPU) || defined(HW_NCPU))
    int name[2];
    name[0] = CTL_HW;
    int ncpu = -1;
    size_t len = sizeof(ncpu);
    // some BSDs might provide both, so we'll check both if both are available
#ifdef HW_AVAILCPU
    if(ncpu < 1)
    {
        name[1] = HW_AVAILCPU;
        sysctl(name, sizeof(name) / sizeof(*name), &ncpu, &len, NULL, 0);
    }
#endif
#ifdef HW_NCPU
    if(ncpu < 1)
    {
        name[1] = HW_NCPU;
        sysctl(name, sizeof(name) / sizeof(*name), &ncpu, &len, NULL, 0);
    }
#endif
    return ncpu < 0 ? 0 : ncpu;
#elif defined(_SC_NPROCESSORS_ONLN)
    // non-BSD, try for sysconf
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    return ncpu < 0 ? 0 : ncpu;
#else
    // we have a POSIX system, but still don't know how to get # of CPU cores ...
    #pragma message ("Warning: tcthread_get_cpu_count uninplemented on platform")
    return 0;   // unlike the "no platform" option below, we do exit gracefully here
#endif
#else
    #pragma message ("Warning: tcthread_get_cpu_count uninplemented on platform")
#endif
}


// Threads
tcthread_t tcthread_create(size_t stack_size, tcthread_runner_t* runner, void* udata)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    tcthread_t thread = {0};
    // initialize thread-local storage
    PVOID init_once_context;    // unused, but necessary?
    if(!InitOnceExecuteOnce(&tcthread_init_once_, tcthread_init_once_runner_, NULL, &init_once_context))
        return thread;

    /*
     * _beginthreadex is the replacement for CreateThread (due to a memory leak
     * in the latter), though it only supports stack sizes up to 4GB; I doubt
     * this will be a problem.
     *
     * See: https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread
     * "A thread in an executable that calls the C run-time library (CRT) should
     * use the _beginthreadex and _endthreadex functions for thread management
     * rather than CreateThread and ExitThread; this requires the use of the
     * multithreaded version of the CRT. If a thread created using CreateThread
     * calls the CRT, the CRT may terminate the process in low-memory conditions."
     */
    // TODO: We can avoid this allocation via an init variable; would that be better?
    struct tcthread_internal_* internal = TC__VOID_CAST(struct tcthread_internal_*, TC_MALLOC(sizeof(struct tcthread_internal_)));
    internal->data = udata;
    internal->runner = runner;
    tcthread_atomicbool_store_explicit(&internal->done_or_detached, false, TCTHREAD_MEMORDER_RELEASE);

    // created as a suspended thread to avoid races
    uintptr_t handle = _beginthreadex(NULL, TC__STATIC_CAST(DWORD, stack_size), tcthread_create_runner_, internal, CREATE_SUSPENDED, NULL);
    if(!handle)
    {
        TC_FREE(internal);
        return thread;
    }
    // this assignment is the race we're avoiding
    internal->handle = TC__REINTERPRET_CAST(HANDLE, handle);
    ResumeThread(internal->handle);
    thread.handle = TC__REINTERPRET_CAST(uintptr_t, internal);
    return thread;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    tcthread_t thread = {0};
    // initialize thread-local storage
    pthread_once(&tcthread_init_once_, tcthread_init_once_runner_);

    struct tcthread_internal_* internal = TC__VOID_CAST(struct tcthread_internal_*, TC_MALLOC(sizeof(struct tcthread_internal_)));
    internal->data = udata;
    internal->runner = runner;
    tcthread_atomicbool_store_explicit(&internal->done_or_detached, false, TCTHREAD_MEMORDER_RELEASE);

    if(pthread_mutex_init(&internal->mutex, NULL))
    {
        TC_FREE(internal);
        return thread;
    }

    // thread is created within a locked mutex, to ensure we can assign internal->handle in time
    // (this emulates creating a thread in a suspended state)
    if(pthread_mutex_lock(&internal->mutex))
    {
        pthread_mutex_destroy(&internal->mutex);
        TC_FREE(internal);
        return thread;
    }

    int ret;
    if(stack_size)
    {
        pthread_attr_t attr;
        if(pthread_attr_init(&attr))
        {
            pthread_mutex_destroy(&internal->mutex);
            TC_FREE(internal);
            return thread;  // pthread_attr_init (somehow) failed
        }
        if(pthread_attr_setstacksize(&attr, stack_size))
        {
            pthread_attr_destroy(&attr);    // can also fail for some silly reason, but, what would we even do?
            pthread_mutex_destroy(&internal->mutex);
            TC_FREE(internal);
            return thread;
        }
        ret = pthread_create(&internal->handle, &attr, tcthread_create_runner_, internal);
        pthread_attr_destroy(&attr);    // can also fail for some silly reason, but, what would we even do?
    }
    else
        ret = pthread_create(&internal->handle, NULL, tcthread_create_runner_, internal);

    // can fail, but nothing we can do then ...
    pthread_mutex_unlock(&internal->mutex);

    if(ret) // error
    {
        pthread_mutex_destroy(&internal->mutex);
        TC_FREE(internal);
        return thread;
    }
    thread.handle = TC__REINTERPRET_CAST(uintptr_t, internal);
    return thread;
#else
    #pragma message ("Warning: tcthread_create uninplemented on platform")
#endif
}
bool tcthread_is_valid(tcthread_t thread);
void tcthread_join(tcthread_t thread, void** retval)
{
    TC_ASSERT(thread.handle, "tcthread_join passed null `thread` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    struct tcthread_internal_* internal = TC__REINTERPRET_CAST(struct tcthread_internal_*, thread.handle);
    if(WaitForSingleObject(internal->handle, INFINITE))
        TC_ASSERT(false, "WaitForSingleObject failed");
    if(retval) *retval = internal->data;
    if(!CloseHandle(internal->handle))
        TC_ASSERT(false, "CloseHandle failed");
    TC_FREE(internal);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    struct tcthread_internal_* internal = TC__REINTERPRET_CAST(struct tcthread_internal_*, thread.handle);
    if(pthread_join(internal->handle, retval))
        TC_ASSERT(false, "thread_join failed");
    TC_FREE(internal);
#else
    #pragma message ("Warning: tcthread_join uninplemented on platform")
#endif
}
void tcthread_detach(tcthread_t thread)
{
    TC_ASSERT(thread.handle, "tcthread_detach passed null `thread` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    struct tcthread_internal_* internal = TC__REINTERPRET_CAST(struct tcthread_internal_*, thread.handle);
    if(!CloseHandle(internal->handle))
        TC_ASSERT(false, "CloseHandle failed");
    // test-and-set; if this was already set, then thread has already exited, and we can free its data
    if(tcthread_atomicbool_exchange_explicit(&internal->done_or_detached, true, TCTHREAD_MEMORDER_ACQ_REL))
        TC_FREE(internal);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    struct tcthread_internal_* internal = TC__REINTERPRET_CAST(struct tcthread_internal_*, thread.handle);
    if(!pthread_detach(internal->handle))
        TC_ASSERT(false, "pthread_detach failed (thread is non-detachable or invalid parameter)");
    // test-and-set; if this was already set, then thread has already exited, and we can free its data
    if(tcthread_atomicbool_exchange_explicit(&internal->done_or_detached, true, TCTHREAD_MEMORDER_ACQ_REL))
        TC_FREE(internal);
#else
    #pragma message ("Warning: tcthread_detach uninplemented on platform")
#endif
}

void tcthread_exit(void* retval)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    tcthread_t thread = tcthread_self();
    if(tcthread_is_valid(thread))
    {
        struct tcthread_internal_* internal = TC__REINTERPRET_CAST(struct tcthread_internal_*, thread.handle);
        tcthread_before_exit_(internal, retval);
    }
    _endthreadex(0);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    tcthread_t thread = tcthread_self();
    if(tcthread_is_valid(thread))
    {
        struct tcthread_internal_* internal = TC__REINTERPRET_CAST(struct tcthread_internal_*, thread.handle);
        tcthread_before_exit_(internal);
    }
    pthread_exit(retval);
#else
    #pragma message ("Warning: tcthread_exit uninplemented on platform")
#endif
}
void tcthread_sleep(uint32_t ms)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    Sleep(ms);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    struct timespec ts = tcthread_timespec_from_ms_(ms);
    // we loop `nanosleep` as long as it's not yet done (thus ignoring signal wakeups)
    while(nanosleep(&ts, &ts) && errno == EINTR) {}
#else
    #pragma message ("Warning: tcthread_sleep uninplemented on platform")
#endif
}
tcthread_t tcthread_self(void)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    tcthread_t thread = {0};
    if(tcthread_tls_index_ == TLS_OUT_OF_INDEXES)   // index was not initialized ...
        return thread;
    thread.handle = TC__REINTERPRET_CAST(uintptr_t, TlsGetValue(tcthread_tls_index_));
    return thread;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    tcthread_t thread = {0};
    thread.handle = TC__REINTERPRET_CAST(uintptr_t, pthread_getspecific(tcthread_tls_key_));
    return thread;
#else
    #pragma message ("Warning: tcthread_self uninplemented on platform")
#endif
}


// Mutex
tcthread_mutex_t tcthread_mutex_create(bool recursive)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    tcthread_mutex_t mutex = {0};
    TC__PARAM_UNUSED(recursive);    // CRITICAL_SECTION is always recursive
    // it's 40 bytes, so it will not fit in a uintptr_t directly
    CRITICAL_SECTION* cs = TC__VOID_CAST(CRITICAL_SECTION*, TC_MALLOC(sizeof(CRITICAL_SECTION)));
    // can technically fail in Windows Server 2003 & Windows XP, but we don't really care about that
    InitializeCriticalSection(cs);
    mutex.handle = TC__REINTERPRET_CAST(uintptr_t, cs);
    return mutex;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    tcthread_mutex_t mutex = {0};

    pthread_mutex_t* pmutex = TC__VOID_CAST(pthread_mutex_t*, TC_MALLOC(sizeof(pthread_mutex_t)));
    int ret;
    if(recursive)
    {
        pthread_mutexattr_t attr;
        if(pthread_mutexattr_init(&attr))
        {
            TC_FREE(pmutex);
            return mutex;   // pthread_mutexattr_init (somehow) failed
        }
        if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE))
        {
            pthread_mutexattr_destroy(&attr);   // can also fail for some silly reason, but, what would we even do?
            TC_FREE(pmutex);
            return mutex;
        }
        ret = pthread_mutex_init(pmutex, &attr);
        pthread_mutexattr_destroy(&attr);   // can also fail for some silly reason, but, what would we even do?
    }
    else
        ret = pthread_mutex_init(pmutex, NULL);

    if(ret) // error
    {
        TC_FREE(pmutex);
        return mutex;
    }
    mutex.handle = TC__REINTERPRET_CAST(uintptr_t, pmutex);
    return mutex;
#else
    #pragma message ("Warning: tcthread_mutex_create uninplemented on platform")
#endif
}
bool tcthread_mutex_is_valid(tcthread_mutex_t mutex);
void tcthread_mutex_destroy(tcthread_mutex_t mutex)
{
    if(!mutex.handle) return;   // _destroy silently allows null handles
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CRITICAL_SECTION* cs = TC__REINTERPRET_CAST(CRITICAL_SECTION*, mutex.handle);
    DeleteCriticalSection(cs);
    TC_FREE(cs);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_mutex_t* pmutex = TC__REINTERPRET_CAST(pthread_mutex_t*, mutex.handle);
    if(pthread_mutex_destroy(pmutex))
        TC_ASSERT(false, "pthread_mutex_destroy failed (probably in use or invalid parameter)");
    TC_FREE(pmutex);
#else
    #pragma message ("Warning: tcthread_mutex_destroy uninplemented on platform")
#endif
}
void tcthread_mutex_lock(tcthread_mutex_t mutex)
{
    TC_ASSERT(mutex.handle, "tcthread_mutex_lock passed null `mutex` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CRITICAL_SECTION* cs = TC__REINTERPRET_CAST(CRITICAL_SECTION*, mutex.handle);
    // can throw deadlock exception
    EnterCriticalSection(cs);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_mutex_t* pmutex = TC__REINTERPRET_CAST(pthread_mutex_t*, mutex.handle);
    if(pthread_mutex_lock(pmutex))
        TC_ASSERT(false, "pthread_mutex_lock failed");
#else
    #pragma message ("Warning: tcthread_mutex_lock uninplemented on platform")
#endif
}
bool tcthread_mutex_try_lock(tcthread_mutex_t mutex)
{
    TC_ASSERT(mutex.handle, "tcthread_mutex_try_lock passed null `mutex` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CRITICAL_SECTION* cs = TC__REINTERPRET_CAST(CRITICAL_SECTION*, mutex.handle);
    return TryEnterCriticalSection(cs);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_mutex_t* pmutex = TC__REINTERPRET_CAST(pthread_mutex_t*, mutex.handle);
    int ret = pthread_mutex_trylock(pmutex);
    if(ret)
    {
        TC_ASSERT(ret == EBUSY, "pthread_mutex_trylock failed");
        return false;   // locking failed
    }
    return true;
#else
    #pragma message ("Warning: tcthread_mutex_try_lock uninplemented on platform")
#endif
}
//bool tcthread_mutex_timed_lock(tcthread_mutex_t mutex, uint32_t timeout_ms);
void tcthread_mutex_unlock(tcthread_mutex_t mutex)
{
    TC_ASSERT(mutex.handle, "tcthread_mutex_unlock passed null `mutex` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CRITICAL_SECTION* cs = TC__REINTERPRET_CAST(CRITICAL_SECTION*, mutex.handle);
    LeaveCriticalSection(cs);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_mutex_t* pmutex = TC__REINTERPRET_CAST(pthread_mutex_t*, mutex.handle);
    if(pthread_mutex_unlock(pmutex))
        TC_ASSERT(false, "pthread_mutex_unlock failed (was mutex locked by this thread?)");
#else
    #pragma message ("Warning: tcthread_mutex_unlock uninplemented on platform")
#endif
}


// Condition Variable
tcthread_cond_t tcthread_cond_create(void)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    tcthread_cond_t cond = {0};
    // it's 8 bytes, so it does fit in uintptr_t ... but we're not allowed to move it, so we need to wrap it in something that won't move
    CONDITION_VARIABLE* cv = TC__VOID_CAST(CONDITION_VARIABLE*, TC_MALLOC(sizeof(CONDITION_VARIABLE)));
    InitializeConditionVariable(cv);
    cond.handle = TC__REINTERPRET_CAST(uintptr_t, cv);
    return cond;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    tcthread_cond_t cond = {0};
    pthread_cond_t* pcond = TC__VOID_CAST(pthread_cond_t*, TC_MALLOC(sizeof(pthread_cond_t)));
    if(pthread_cond_init(pcond, NULL)) // error
    {
        TC_FREE(pcond);
        return cond;
    }
    cond.handle = TC__REINTERPRET_CAST(uintptr_t, pcond);
    return cond;
#else
    #pragma message ("Warning: tcthread_cond_create uninplemented on platform")
#endif
}
bool tcthread_cond_is_valid(tcthread_cond_t cond);
void tcthread_cond_destroy(tcthread_cond_t cond)
{
    if(!cond.handle) return;    // _destroy silently allows null handles
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CONDITION_VARIABLE* cv = TC__REINTERPRET_CAST(CONDITION_VARIABLE*, cond.handle);
    // WinAPI itself needs no freeing
    TC_FREE(cv);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_cond_t* pcond = TC__REINTERPRET_CAST(pthread_cond_t*, cond.handle);
    if(pthread_cond_destroy(pcond))
        TC_ASSERT(false, "pthread_cond_destroy failed (probably in use or invalid parameter)");
    TC_FREE(pcond);
#else
    #pragma message ("Warning: tcthread_cond_destroy uninplemented on platform")
#endif
}
void tcthread_cond_wait(tcthread_cond_t cond, tcthread_mutex_t mutex)
{
    TC_ASSERT(cond.handle, "tcthread_cond_wait passed null `cond` parameter");
    TC_ASSERT(mutex.handle, "tcthread_cond_wait passed null `mutex` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CONDITION_VARIABLE* cv = TC__REINTERPRET_CAST(CONDITION_VARIABLE*, cond.handle);
    CRITICAL_SECTION* cs = TC__REINTERPRET_CAST(CRITICAL_SECTION*, mutex.handle);
    if(!SleepConditionVariableCS(cv, cs, INFINITE))
        TC_ASSERT(false, "SleepConditionVariableCS failed");
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_cond_t* pcond = TC__REINTERPRET_CAST(pthread_cond_t*, cond.handle);
    pthread_mutex_t* pmutex = TC__REINTERPRET_CAST(pthread_mutex_t*, mutex.handle);
    if(pthread_cond_wait(pcond, pmutex))
        TC_ASSERT(false, "pthread_cond_wait failed");
#else
    #pragma message ("Warning: tcthread_cond_wait uninplemented on platform")
#endif
}
bool tcthread_cond_timed_wait(tcthread_cond_t cond, tcthread_mutex_t mutex, uint32_t timeout_ms)
{
    TC_ASSERT(cond.handle, "tcthread_cond_timed_wait passed null `cond` parameter");
    TC_ASSERT(mutex.handle, "tcthread_cond_timed_wait passed null `mutex` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CONDITION_VARIABLE* cv = TC__REINTERPRET_CAST(CONDITION_VARIABLE*, cond.handle);
    CRITICAL_SECTION* cs = TC__REINTERPRET_CAST(CRITICAL_SECTION*, mutex.handle);
    if(!SleepConditionVariableCS(cv, cs, timeout_ms))
    {
        TC_ASSERT(GetLastError() == ERROR_TIMEOUT, "SleepConditionVariableCS failed (and error was not a timeout)");
        return false;   // timed out
    }
    return true;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_cond_t* pcond = TC__REINTERPRET_CAST(pthread_cond_t*, cond.handle);
    pthread_mutex_t* pmutex = TC__REINTERPRET_CAST(pthread_mutex_t*, mutex.handle);
    struct timespec ts = tcthread_timespec_from_ms_(timeout_ms);
    int ret = pthread_cond_timedwait(pcond, pmutex, &ts);
    if(ret) // timeout or failure
    {
        TC_ASSERT(ret == ETIMEDOUT, "pthread_cond_timedwait failed (and error was not a timeout)");
        return false;   // timed out
    }
    return true;
#else
    #pragma message ("Warning: tcthread_cond_timed_wait uninplemented on platform")
#endif
}
void tcthread_cond_signal(tcthread_cond_t cond)
{
    TC_ASSERT(cond.handle, "tcthread_cond_signal passed null `cond` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CONDITION_VARIABLE* cv = TC__REINTERPRET_CAST(CONDITION_VARIABLE*, cond.handle);
    WakeConditionVariable(cv);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_cond_t* pcond = TC__REINTERPRET_CAST(pthread_cond_t*, cond.handle);
    if(pthread_cond_signal(pcond))
        TC_ASSERT(false, "pthread_cond_signal failed (probably invalid parameter)");
#else
    #pragma message ("Warning: tcthread_cond_signal uninplemented on platform")
#endif
}
void tcthread_cond_broadcast(tcthread_cond_t cond)
{
    TC_ASSERT(cond.handle, "tcthread_cond_broadcast passed null `cond` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    CONDITION_VARIABLE* cv = TC__REINTERPRET_CAST(CONDITION_VARIABLE*, cond.handle);
    WakeAllConditionVariable(cv);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_cond_t* pcond = TC__REINTERPRET_CAST(pthread_cond_t*, cond.handle);
    if(pthread_cond_broadcast(pcond))
        TC_ASSERT(false, "pthread_cond_broadcast failed (probably invalid parameter)");
#else
    #pragma message ("Warning: tcthread_cond_broadcast uninplemented on platform")
#endif
}


// Semaphore
tcthread_sem_t tcthread_sem_create(uint32_t initial_value)
{
    TC_ASSERT(initial_value <= INT32_MAX, "initial_value for tcthread_sem_create must be <= INT32_MAX");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    tcthread_sem_t sem = {0};
    sem.handle = TC__REINTERPRET_CAST(uintptr_t, CreateSemaphoreW(NULL, initial_value, LONG_MAX, NULL));
    return sem;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    tcthread_sem_t sem = {0};
    sem_t* psem = TC__VOID_CAST(sem_t*, TC_MALLOC(sizeof(sem_t)));
    if(sem_init(psem, 0, initial_value))    // error
    {
        TC_FREE(psem);
        return sem;
    }
    sem.handle = TC__REINTERPRET_CAST(uintptr_t, psem);
    return sem;
#else
    #pragma message ("Warning: tcthread_sem_create uninplemented on platform")
#endif
}
bool tcthread_sem_is_valid(tcthread_sem_t sem);
void tcthread_sem_destroy(tcthread_sem_t sem)
{
    if(!sem.handle) return; // _destroy silently allows null handles
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    HANDLE handle = TC__REINTERPRET_CAST(HANDLE, sem.handle);
    if(!CloseHandle(handle))
        TC_ASSERT(false, "CloseHandle failed");
#elif defined(TCTHREAD__PLATFORM_POSIX)
    sem_t* psem = TC__REINTERPRET_CAST(sem_t*, sem.handle);
    if(sem_destroy(psem))
        TC_ASSERT(false, "sem_destroy failed (probably invalid parameter)");
    TC_FREE(psem);
#else
    #pragma message ("Warning: tcthread_sem_destroy uninplemented on platform")
#endif
}
void tcthread_sem_post(tcthread_sem_t sem)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    HANDLE handle = TC__REINTERPRET_CAST(HANDLE, sem.handle);
    if(!ReleaseSemaphore(handle, 1, NULL))
        TC_ASSERT(false, "ReleaseSemaphore failed");
#elif defined(TCTHREAD__PLATFORM_POSIX)
    sem_t* psem = TC__REINTERPRET_CAST(sem_t*, sem.handle);
    if(sem_post(psem))
        TC_ASSERT(false, "sem_post failed (probably invalid parameter)");
#else
    #pragma message ("Warning: tcthread_sem_post uninplemented on platform")
#endif
}
void tcthread_sem_wait(tcthread_sem_t sem)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    HANDLE handle = TC__REINTERPRET_CAST(HANDLE, sem.handle);
    if(WaitForSingleObject(handle, INFINITE))
        TC_ASSERT(false, "WaitForSingleObject failed");
#elif defined(TCTHREAD__PLATFORM_POSIX)
    sem_t* psem = TC__REINTERPRET_CAST(sem_t*, sem.handle);
    if(sem_wait(psem))
        TC_ASSERT(false, "sem_wait failed");
#else
    #pragma message ("Warning: tcthread_sem_wait uninplemented on platform")
#endif
}
bool tcthread_sem_try_wait(tcthread_sem_t sem)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    HANDLE handle = TC__REINTERPRET_CAST(HANDLE, sem.handle);
    DWORD ret = WaitForSingleObject(handle, 0);
    if(ret)
    {
        TC_ASSERT(ret == WAIT_TIMEOUT, "WaitForSingleObject failed");
        return false;   // waiting failed
    }
    return true;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    sem_t* psem = TC__REINTERPRET_CAST(sem_t*, sem.handle);
    int ret = sem_trywait(psem);
    if(ret)
    {
        TC_ASSERT(errno == EAGAIN, "sem_trywait failed");
        return false;   // waiting failed
    }
    return true;
#else
    #pragma message ("Warning: tcthread_sem_try_wait uninplemented on platform")
#endif
}
bool tcthread_sem_timed_wait(tcthread_sem_t sem, uint32_t timeout_ms)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    HANDLE handle = TC__REINTERPRET_CAST(HANDLE, sem.handle);
    DWORD ret = WaitForSingleObject(handle, timeout_ms);
    if(ret)
    {
        TC_ASSERT(ret == WAIT_TIMEOUT, "WaitForSingleObject failed");
        return false;   // timed out
    }
    return true;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    sem_t* psem = TC__REINTERPRET_CAST(sem_t*, sem.handle);
    struct timespec ts = tcthread_timespec_from_ms_(timeout_ms);
    int ret = sem_timedwait(psem, &ts);
    if(ret)
    {
        TC_ASSERT(errno == ETIMEDOUT, "sem_timedwait failed");
        return false;   // timed out
    }
    return true;
#else
    #pragma message ("Warning: tcthread_sem_timed_wait uninplemented on platform")
#endif
}


// RWLock
tcthread_rwlock_t tcthread_rwlock_create(void)
{
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    tcthread_rwlock_t rwlock = {0};
    SRWLOCK* srw = TC__VOID_CAST(SRWLOCK*, TC_MALLOC(sizeof(SRWLOCK)));
    InitializeSRWLock(srw);
    rwlock.handle = TC__REINTERPRET_CAST(uintptr_t, srw);
    return rwlock;
#elif defined(TCTHREAD__PLATFORM_POSIX)
    tcthread_rwlock_t rwlock = {0};
    pthread_rwlock_t* prwlock = TC__VOID_CAST(pthread_rwlock_t*, TC_MALLOC(sizeof(pthread_rwlock_t)));
    if(pthread_rwlock_init(prwlock, NULL))  // error
    {
        TC_FREE(prwlock);
        return rwlock;
    }
    rwlock.handle = TC__REINTERPRET_CAST(uintptr_t, prwlock);
    return rwlock;
#else
    #pragma message ("Warning: tcthread_rwlock_create uninplemented on platform")
#endif
}
bool tcthread_rwlock_is_valid(tcthread_rwlock_t rwlock);
void tcthread_rwlock_destroy(tcthread_rwlock_t rwlock)
{
    if(!rwlock.handle) return;  // _destroy silently allows null handles
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    SRWLOCK* srw = TC__REINTERPRET_CAST(SRWLOCK*, rwlock.handle);
    // WinAPI itself needs no freeing
    TC_FREE(srw);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_rwlock_t* prwlock = TC__REINTERPRET_CAST(pthread_rwlock_t*, rwlock.handle);
    if(pthread_rwlock_destroy(prwlock))
        TC_ASSERT(false, "pthread_rwlock_destroy failed");
    TC_FREE(prwlock);
#else
    #pragma message ("Warning: tcthread_rwlock_destroy uninplemented on platform")
#endif
}
void tcthread_rwlock_lock_rd(tcthread_rwlock_t rwlock)
{
    TC_ASSERT(rwlock.handle, "tcthread_rwlock_lock_rd passed null `rwlock` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    SRWLOCK* srw = TC__REINTERPRET_CAST(SRWLOCK*, rwlock.handle);
    AcquireSRWLockShared(srw);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_rwlock_t* prwlock = TC__REINTERPRET_CAST(pthread_rwlock_t*, rwlock.handle);
    if(pthread_rwlock_rdlock(prwlock))  // TODO: handle EAGAIN?
        TC_ASSERT(false, "pthread_rwlock_rdlock failed");
#else
    #pragma message ("Warning: tcthread_rwlock_lock_rd uninplemented on platform")
#endif
}
bool tcthread_rwlock_try_lock_rd(tcthread_rwlock_t rwlock)
{
    TC_ASSERT(rwlock.handle, "tcthread_rwlock_try_lock_rd passed null `rwlock` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    SRWLOCK* srw = TC__REINTERPRET_CAST(SRWLOCK*, rwlock.handle);
    return TryAcquireSRWLockShared(srw);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_rwlock_t* prwlock = TC__REINTERPRET_CAST(pthread_rwlock_t*, rwlock.handle);
    int ret = pthread_rwlock_tryrdlock(prwlock);
    if(ret)
    {
        TC_ASSERT(ret == EBUSY, "pthread_rwlock_tryrdlock failed");
        return false;   // locking failed
    }
    return true;
#else
    #pragma message ("Warning: tcthread_rwlock_try_lock_rd uninplemented on platform")
#endif
}
void tcthread_rwlock_unlock_rd(tcthread_rwlock_t rwlock)
{
    TC_ASSERT(rwlock.handle, "tcthread_rwlock_unlock_rd passed null `rwlock` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    SRWLOCK* srw = TC__REINTERPRET_CAST(SRWLOCK*, rwlock.handle);
    ReleaseSRWLockShared(srw);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_rwlock_t* prwlock = TC__REINTERPRET_CAST(pthread_rwlock_t*, rwlock.handle);
    if(pthread_rwlock_unlock(prwlock))
        TC_ASSERT(false, "pthread_rwlock_unlock failed");
#else
    #pragma message ("Warning: tcthread_rwlock_unlock_rd uninplemented on platform")
#endif
}
void tcthread_rwlock_lock_wr(tcthread_rwlock_t rwlock)
{
    TC_ASSERT(rwlock.handle, "tcthread_rwlock_lock_wr passed null `rwlock` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    SRWLOCK* srw = TC__REINTERPRET_CAST(SRWLOCK*, rwlock.handle);
    AcquireSRWLockExclusive(srw);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_rwlock_t* prwlock = TC__REINTERPRET_CAST(pthread_rwlock_t*, rwlock.handle);
    if(pthread_rwlock_wrlock(prwlock))  // TODO: handle EAGAIN?
        TC_ASSERT(false, "pthread_rwlock_wrlock failed");
#else
    #pragma message ("Warning: tcthread_rwlock_lock_rd uninplemented on platform")
#endif
}
bool tcthread_rwlock_try_lock_wr(tcthread_rwlock_t rwlock)
{
    TC_ASSERT(rwlock.handle, "tcthread_rwlock_try_lock_wr passed null `rwlock` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    SRWLOCK* srw = TC__REINTERPRET_CAST(SRWLOCK*, rwlock.handle);
    return TryAcquireSRWLockExclusive(srw);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_rwlock_t* prwlock = TC__REINTERPRET_CAST(pthread_rwlock_t*, rwlock.handle);
    int ret = pthread_rwlock_trywrlock(prwlock);
    if(ret)
    {
        TC_ASSERT(ret == EBUSY, "pthread_rwlock_trywrlock failed");
        return false;   // locking failed
    }
    return true;
#else
    #pragma message ("Warning: tcthread_rwlock_try_lock_rd uninplemented on platform")
#endif
}
void tcthread_rwlock_unlock_wr(tcthread_rwlock_t rwlock)
{
    TC_ASSERT(rwlock.handle, "tcthread_rwlock_unlock_wr passed null `rwlock` parameter");
#if defined(TCTHREAD__PLATFORM_WINDOWS)
    SRWLOCK* srw = TC__REINTERPRET_CAST(SRWLOCK*, rwlock.handle);
    ReleaseSRWLockExclusive(srw);
#elif defined(TCTHREAD__PLATFORM_POSIX)
    pthread_rwlock_t* prwlock = TC__REINTERPRET_CAST(pthread_rwlock_t*, rwlock.handle);
    if(pthread_rwlock_unlock(prwlock))
        TC_ASSERT(false, "pthread_rwlock_unlock failed");
#else
    #pragma message ("Warning: tcthread_rwlock_unlock_rd uninplemented on platform")
#endif
}


// ********** ATOMICS **********

// ***** atomicptr (wrappers around `atomicsz`) *****
tcthread_atomicptr_t tcthread_atomicptr_load_explicit(volatile tcthread_atomicptr_t* ptr, int memorder);
void tcthread_atomicptr_store_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t value, int memorder);
tcthread_atomicptr_t tcthread_atomicptr_exchange_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t desired, int memorder);
tcthread_atomicptr_t tcthread_atomicptr_compare_exchange_strong_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t expected, tcthread_atomicptr_t desired, int memorder_success, int memorder_failure);
tcthread_atomicptr_t tcthread_atomicptr_compare_exchange_weak_explicit(volatile tcthread_atomicptr_t* ptr, tcthread_atomicptr_t expected, tcthread_atomicptr_t desired, int memorder_success, int memorder_failure);

// ***** boolean *****
bool tcthread_atomicbool_load_explicit(volatile tcthread_atomicbool_t* ptr, int memorder);
void tcthread_atomicbool_store_explicit(volatile tcthread_atomicbool_t* ptr, bool value, int memorder);
bool tcthread_atomicbool_exchange_explicit(volatile tcthread_atomicbool_t* ptr, tcthread_atomicbool_t desired, int memorder);

// ***** load/store *****
tcthread_atomic32_t tcthread_atomic32_load_explicit(volatile tcthread_atomic32_t* ptr, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_load_explicit(volatile tcthread_atomicsz_t* ptr, int memorder);
void tcthread_atomic32_store_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder);
void tcthread_atomicsz_store_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder);

// ***** (compare-and-)swap *****
tcthread_atomic32_t tcthread_atomic32_exchange_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t desired, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_exchange_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t desired, int memorder);
tcthread_atomic32_t tcthread_atomic32_compare_exchange_strong_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t expected, tcthread_atomic32_t desired, int memorder_success, int memorder_failure);
tcthread_atomicsz_t tcthread_atomicsz_compare_exchange_strong_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t expected, tcthread_atomicsz_t desired, int memorder_success, int memorder_failure);
tcthread_atomic32_t tcthread_atomic32_compare_exchange_weak_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t expected, tcthread_atomic32_t desired, int memorder_success, int memorder_failure);
tcthread_atomicsz_t tcthread_atomicsz_compare_exchange_weak_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t expected, tcthread_atomicsz_t desired, int memorder_success, int memorder_failure);

// ***** arithmetic & bitwise *****
tcthread_atomic32_t tcthread_atomic32_fetch_add_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_fetch_add_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder);
tcthread_atomic32_t tcthread_atomic32_fetch_sub_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_fetch_sub_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder);
tcthread_atomic32_t tcthread_atomic32_fetch_and_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_fetch_and_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder);
tcthread_atomic32_t tcthread_atomic32_fetch_xor_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_fetch_xor_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder);
tcthread_atomic32_t tcthread_atomic32_fetch_or_explicit(volatile tcthread_atomic32_t* ptr, tcthread_atomic32_t value, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_fetch_or_explicit(volatile tcthread_atomicsz_t* ptr, tcthread_atomicsz_t value, int memorder);
// GCC additionally supports `nand` ... should we expose it?

// ***** increment & decrement; these return the *new* value *****
tcthread_atomic32_t tcthread_atomic32_inc_explicit(volatile tcthread_atomic32_t* ptr, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_inc_explicit(volatile tcthread_atomicsz_t* ptr, int memorder);
tcthread_atomic32_t tcthread_atomic32_dec_explicit(volatile tcthread_atomic32_t* ptr, int memorder);
tcthread_atomicsz_t tcthread_atomicsz_dec_explicit(volatile tcthread_atomicsz_t* ptr, int memorder);

#endif /* TC_THREAD_IMPLEMENTATION */

/*----------------------------------------------------------------------------*\
 |  library.cpp - cross-platform library implementation                       |
 |                                                                            |
 |  Copyright � 2005-2008, The Gray Matter Team, original authors.            |
\*----------------------------------------------------------------------------*/

/*
 | This program is free software: you can redistribute it and/or modify it under
 | the terms of the GNU General Public License as published by the Free Software
 | Foundation, either version 3 of the License, or (at your option) any later
 | version.
 |
 | This program is distributed in the hope that it will be useful, but WITHOUT
 | ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 | FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 | details.
 |
 | You should have received a copy of the GNU General Public License along with
 | this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "library.h"
#include <iostream>

//fast bit index lookup assistance
//static const int MultiplyDeBruijnBitPosition[32] = 
//{
// 0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
//  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
//};
static const int MultiplyDeBruijnBitPosition[32] = 
{
  1,2,29,3,30,15,25,4,31,23,21,16,26,18,5,9,
  32,28,14,24,22,20,17,8,27,13,19,7,12,6,11,10
};


//Windows needs a variable to hold the time per move
//since it seems to get lost/corrupted in the current
//threading model
unsigned int timePerMove = 0;

/*----------------------------------------------------------------------------*\
 |                              thread_create()                               |
\*----------------------------------------------------------------------------*/
int thread_create(thread_t *thread, entry_t entry, void *arg)
{

// Create a thread.

#if defined(LINUX) || defined(OS_X)
    return pthread_create(thread, NULL, entry, arg) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    return (*thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) entry, arg, 0, NULL)) == NULL ? CRITICAL : SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                               thread_wait()                                |
\*----------------------------------------------------------------------------*/
int thread_wait(thread_t *thread)
{

// Wait for a thread to exit.

#if defined(LINUX) || defined(OS_X)
    return pthread_join(*thread, NULL) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    return WaitForSingleObject(*thread, INFINITE) == WAIT_FAILED ? CRITICAL : SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                              thread_destroy()                              |
\*----------------------------------------------------------------------------*/
int thread_destroy(thread_t *thread)
{

// Destroy a thread.  If thread is NULL, destroy the calling thread.  Otherwise,
// destroy the specified thread.

#if defined(LINUX) || defined(OS_X)
    if (thread == NULL)
    {
        pthread_exit(NULL);
        return CRITICAL; // This should never be reached.
    }
    else
        return pthread_kill(*thread, SIGTERM) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    if (thread == NULL)
    {
        ExitThread(0);
        return CRITICAL; // This should never be reached.
    }
    else
    {
      if (!TerminateThread(thread, 0))
      {
        *thread = INVALID_HANDLE_VALUE;
        return CRITICAL;
      }
      *thread = INVALID_HANDLE_VALUE;
      return SUCCESSFUL;
    }
#endif
    return CRITICAL; // This should never be reached.
}

/*----------------------------------------------------------------------------*\
 |                               mutex_create()                               |
\*----------------------------------------------------------------------------*/
int mutex_create(mutex_t *mutex)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_mutex_init(mutex, NULL) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    *mutex = CreateMutex(NULL, FALSE, NULL);
    return SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                              mutex_try_lock()                              |
\*----------------------------------------------------------------------------*/
int mutex_try_lock(mutex_t *mutex)
{
#if defined(LINUX) || defined(OS_X)
    switch (pthread_mutex_trylock(mutex))
    {
        default    : return CRITICAL;
        case EBUSY : return NON_CRITICAL;
        case 0     : return SUCCESSFUL;
    }
#elif defined(_MINGW_WINDOWS)
    return WaitForSingleObject(*mutex, 0) == WAIT_TIMEOUT ? NON_CRITICAL : SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                                mutex_lock()                                |
\*----------------------------------------------------------------------------*/
int mutex_lock(mutex_t *mutex)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_mutex_lock(mutex) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    return WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED ? CRITICAL : SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                               mutex_unlock()                               |
\*----------------------------------------------------------------------------*/
int mutex_unlock(mutex_t *mutex)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_mutex_unlock(mutex) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    return !ReleaseMutex(*mutex) ? CRITICAL : SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                              mutex_destroy()                               |
\*----------------------------------------------------------------------------*/
int mutex_destroy(mutex_t *mutex)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_mutex_destroy(mutex) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    return !CloseHandle(*mutex) ? CRITICAL : SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                               cond_create()                                |
\*----------------------------------------------------------------------------*/
int cond_create(cond_t *cond, void *attr)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_cond_init(cond, (pthread_condattr_t *) attr) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    InitializeCriticalSection(&cond->lock);
    cond->count = 0;
    if ((cond->sema = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL)) == NULL)
        return CRITICAL;
    if ((cond->done = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        return CRITICAL;
    cond->bcast = FALSE;
    return SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                                cond_wait()                                 |
\*----------------------------------------------------------------------------*/
int cond_wait(cond_t *cond, mutex_t *mutex)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_cond_wait(cond, mutex) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    EnterCriticalSection(&cond->lock);
    cond->count++;
    LeaveCriticalSection(&cond->lock);

    if (SignalObjectAndWait(*mutex, cond->sema, INFINITE, FALSE) == WAIT_FAILED)
        return CRITICAL;

    EnterCriticalSection(&cond->lock);
    BOOL waiter = --cond->count || !cond->bcast;
    LeaveCriticalSection(&cond->lock);

    if (!waiter && SignalObjectAndWait(cond->done, *mutex, INFINITE, FALSE) == WAIT_FAILED)
        return CRITICAL;
    if (waiter && WaitForSingleObject(*mutex, INFINITE) == WAIT_FAILED)
        return CRITICAL;
    return SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                               cond_signal()                                |
\*----------------------------------------------------------------------------*/
int cond_signal(cond_t *cond)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_cond_signal(cond) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    EnterCriticalSection(&cond->lock);
    BOOL waiter = cond->count;
    LeaveCriticalSection(&cond->lock);

    if (waiter && !ReleaseSemaphore(cond->sema, 1, 0))
        return CRITICAL;
    return SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                              cond_broadcast()                              |
\*----------------------------------------------------------------------------*/
int cond_broadcast(cond_t *cond)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_cond_broadcast(cond) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    BOOL waiter = FALSE;

    EnterCriticalSection(&cond->lock);
    if (cond->count)
    {
        if (!ReleaseSemaphore(cond->sema, cond->count, 0))
            return CRITICAL;
        cond->bcast = TRUE;
        waiter = TRUE;
    }
    LeaveCriticalSection(&cond->lock);

    if (waiter)
    {
        if (WaitForSingleObject(cond->done, INFINITE) == WAIT_FAILED)
            return CRITICAL;
        cond->bcast = FALSE;
    }
    return SUCCESSFUL;
#endif
}

/*----------------------------------------------------------------------------*\
 |                               cond_destroy()                               |
\*----------------------------------------------------------------------------*/
int cond_destroy(cond_t *cond)
{
#if defined(LINUX) || defined(OS_X)
    return pthread_cond_destroy(cond) ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    if (!CloseHandle(cond->done))
        return CRITICAL;
    if (!CloseHandle(cond->sema))
        return CRITICAL;
    DeleteCriticalSection(&cond->lock);
    return SUCCESSFUL;
#endif
}

// Global variables:
void (*callback)(void*);
void *callback_data;
#if defined(_MINGW_WINDOWS)
thread_t timer_thread = INVALID_HANDLE_VALUE;
#endif

/*----------------------------------------------------------------------------*\
 |                              timer_handler()                               |
\*----------------------------------------------------------------------------*/
#if defined(LINUX) || defined(OS_X)
void timer_handler(int num)
{

// On Linux and OS X, the alarm has sounded.  Call the previously specified
// function.

    (*callback)(callback_data);
}
#elif defined(_MINGW_WINDOWS)
DWORD timer_handler(LPVOID arg)
{

// On Windows, Steve Ballmer is too busy throwing chairs to implement SIGALRM.
// So, in order to replicate its functionality, we have to jump through these
// hoops:
//
//  1. Create a timer thread.  Then, within the timer thread:
//  2. Create an alarm.
//  3. Set the alarm.
//  4. Wait for the alarm to sound.
//  5. Somehow notify the other thread.
//  6. Exit the timer thread.
//
// This way, while the timer thread waits for the alarm to sound, the other
// thread continues with its work.  At this point, the timer thread has already
// been created, and this function is its entry point.  Think of this function
// as the timer thread's main().

    //unsigned long long msec = *((unsigned int *) arg); // Number of milli-
                                                       // seconds to wait before
                                                       // notifying the other
                                                       // thread.

    double dTimeInterval = (double)timePerMove;

    HANDLE timer_id = INVALID_HANDLE_VALUE;

    // Create an alarm.
    if ((timer_id = CreateWaitableTimer(NULL, TRUE, NULL)) == NULL)
        goto end;

    // Set the alarm.
    LARGE_INTEGER rel_time;
    rel_time.QuadPart = -(((long long)(dTimeInterval))*10000000/1000);
    //rel_time.QuadPart = -100000000L; // about one second
    //rel_time.QuadPart = -(msec * 10000L); // We negate this value to denote
                                              // time in 100 nanosecond
                                              // increments.
    if (!SetWaitableTimer(timer_id, &rel_time, 0, NULL, NULL, FALSE))
        goto end;

    // Wait for the alarm to sound.
    if (WaitForSingleObject(timer_id, INFINITE))
        goto end;

    // Notify the other thread - call the previously specified function.
    (*callback)(callback_data);

    // Exit the timer thread.
end:
    if (timer_id != INVALID_HANDLE_VALUE)
        CloseHandle(timer_id);
    thread_destroy(&timer_thread);
    return 0;
}
#endif

/*----------------------------------------------------------------------------*\
 |                              timer_function()                              |
\*----------------------------------------------------------------------------*/
int timer_function(void (*function)(void *), void *data)
{

// Specify the function to be called once the alarm has sounded.

#if defined(LINUX) || defined(OS_X)
    signal(SIGALRM, timer_handler);
#endif
    callback = function;
    callback_data = data;
    return SUCCESSFUL;
}

/*----------------------------------------------------------------------------*\
 |                                timer_set()                                 |
\*----------------------------------------------------------------------------*/
int timer_set(int csec)
{

// Set the alarm to sound after the specified number of seconds.

#if defined(LINUX) || defined(OS_X)
struct itimerval itimerval;
    itimerval.it_interval.tv_sec = 0;
    itimerval.it_interval.tv_usec = 0;
    itimerval.it_value.tv_sec = csec / 100;
    itimerval.it_value.tv_usec = csec % 100 * 10000;
    return setitimer(ITIMER_REAL, &itimerval, NULL) == -1 ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    // Is an alarm already pending?
    if (timer_thread != INVALID_HANDLE_VALUE)
        // Yes.  We can only set one alarm at a time.  :-(
        return CRITICAL;
    timePerMove = csec * 10;
    return thread_create(&timer_thread, (entry_t) timer_handler, &timePerMove);
#endif
}

/*----------------------------------------------------------------------------*\
 |                               timer_cancel()                               |
\*----------------------------------------------------------------------------*/
int timer_cancel()
{

// Cancel any pending alarm.

#if defined(LINUX) || defined(OS_X)
    struct itimerval itimerval;
    itimerval.it_interval.tv_sec = 0;
    itimerval.it_interval.tv_usec = 0;
    itimerval.it_value.tv_sec = 0;
    itimerval.it_value.tv_usec = 0;
    return setitimer(ITIMER_REAL, &itimerval, NULL) == -1 ? CRITICAL : SUCCESSFUL;
#elif defined(_MINGW_WINDOWS)
    // Is an alarm pending?
    if (timer_thread == INVALID_HANDLE_VALUE)
        // No.  There's nothing to cancel.
        return NON_CRITICAL;
    return thread_destroy(&timer_thread);
#endif
}

/*----------------------------------------------------------------------------*\
 |                                 rand_64()                                  |
\*----------------------------------------------------------------------------*/
uint64_t rand_64()
{

// Generate a 64-bit pseudo-random number.

#if defined(LINUX)
    return (uint64_t) rand() << 32 | rand();
#elif defined(OS_X)
    return (uint64_t) arc4random() << 32 | arc4random();
#elif defined(_MINGW_WINDOWS)
    return (uint64_t) rand() << 32 | rand();
#endif
}

/*----------------------------------------------------------------------------*\
 |                                 count_64()                                 |
 |   Count the number of set bits in a 64-bit integer.
\*----------------------------------------------------------------------------*/
int count_64(uint64_t n)
{
    n = n - ((n >> 1) & (uint64_t)~(uint64_t)0/3);
    n = (n & (uint64_t)~(uint64_t)0/15*3) + 
                     ((n >> 2) & (uint64_t)~(uint64_t)0/15*3);
    n = (n + (n >> 4)) & (uint64_t)~(uint64_t)0/255*15;   
    return (uint64_t)(n * ((uint64_t)~(uint64_t)0/255)) >> (sizeof(n) - 1) * 8;
}

// These next two functions, we shamelessly yoinked from the GNU C Library,
// version 2.5, copyright � 1991-1998, the Free Software Foundation, originally
// written by Torbjorn Granlund <tege@sics.se>.

/*----------------------------------------------------------------------------*\
 |                                 find_64()                                  |
\*----------------------------------------------------------------------------*/
int find_64(uint64_t n)
{
  // Find the first (least significant) set bit in a 64-bit integer.  The return
  // value ranges from 0 (for no bit set), to 1 (for the least significant bit
  // set), to 64 (for only the most significant bit set).
  // Optionally, use ffs, ffsl, ffsll, but this code is more portable
  // and just as fast
  if (n == 0) return 0;

  int c = find_32(n); //the lower 32 bits
  if (c) return c;

  return find_32(n >> 32) + 32; //the higher 32 bits
}

/*----------------------------------------------------------------------------*\
 |                                 find_32()                                  |
\*----------------------------------------------------------------------------*/
int find_32(uint32_t n)
{
  // Find the first (least significant) set bit in a 32-bit integer.  The return
  // value ranges from 0 (for no bit set), to 1 (for the least significant bit
  // set), to 32 (for only the most significant bit set).
  if (n == 0) 
    return 0;
  else 
    return MultiplyDeBruijnBitPosition[((uint32_t)((n & -n) * 0x077CB531U)) >> 27];
}

/*----------------------------------------------------------------------------*\
 |                            get_home_directory()                            |
\*----------------------------------------------------------------------------*/
/** Return the current user's home directory. */
char* get_home_directory()
{
#if defined(LINUX)
    return getenv("HOME");
#else
    // Dunno, just use current directory.
    return "./";
#endif
}

// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

void initlock(struct spinlock *lk, char *name)
{
    lk->name = name;
    lk->locked = 0;
    lk->cpu = 0;
}

// Note that this implementation is using a traditional spinlock,
// while Linux has moved to using qspinlocks on ARM64 for performance.

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.

void acquire(struct spinlock *lk)
{
    pushcli();		// disable interrupts to avoid deadlock.

    // CAS with acquire semantics.
    // On Armv8.1-A onwards, this GCC intrinsic will become:
    //    swpab
    //    cbnz
    // On legacy (armv8), this will become:
    //    ldaxrb
    //    stxrb
    //    cmp
    //    ccmp
    //    b.ne 

    while(__atomic_test_and_set(&lk->locked, __ATOMIC_ACQUIRE) != 0)
        asm ("yield" :::);

    // Now we have the lock. Explicit dsb synchronization not needed.

    // Record holder information
    lk->cpu = cpu;
}

// Release the lock.
void release(struct spinlock *lk)
{
    if (!holding(lk))
        panic("release");

    lk->cpu = 0;
    
    // Will become:
    //    stlrb

    __atomic_clear(&lk->locked, __ATOMIC_RELEASE);
    popcli();
}


// Check whether this cpu is holding the lock.
int holding(struct spinlock *lk)
{
    return (lk->locked && lk->cpu == cpu);
}


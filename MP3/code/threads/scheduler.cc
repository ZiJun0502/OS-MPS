// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------
int cmp1(Thread* a, Thread* b){
    if(a->CPUBurstTime > b->CPUBurstTime) return 1;
    if(a->CPUBurstTime < b->CPUBurstTime) return -1;
    return 0;
}
int cmp2(Thread* a, Thread* b){
    if(a->priority > b->priority) return 1;
    if(a->priority < b->priority) return -1;
    return 0;
}
Scheduler::Scheduler()
{ 
    //readyList = new List<Thread *>; 
    L1 = new SortedList<Thread *>(cmp1);
    L2 = new SortedList<Thread *>(cmp2);
    L3 = new List<Thread *>;
    preempting = 0;
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    //delete readyList;
    delete L1;
    delete L2;
    delete L3; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------
void 
Scheduler::age_util(ListIterator<Thread *>* it, List<Thread *> *temp){
    int now = kernel->stats->totalTicks;
    Thread* t;
    for(;!it->IsDone();it->Next()){
        t = it->Item();
        t->waitingTime += now - t->enterWaitTime;
        t->enterWaitTime = now;
        if(t->waitingTime >= 1500){
            t->waitingTime -= 1500;
            t->priority += 10;
        }
        temp->Append(t);
    }
}

void 
Scheduler::age(){
    ListIterator<Thread *>* it = new ListIterator<Thread *>(L1);
    List<Thread *> *temp = new List<Thread *>;
    age_util(it, temp);
    it = new ListIterator<Thread *>(L2);
    age_util(it, temp);
    it = new ListIterator<Thread *>(L3);
    age_util(it, temp);
    it = new ListIterator<Thread *>(temp);
    Thread* t;
    double BurstTime_min = 1e18;
    int now = kernel->stats->totalTicks;
    bool higherQueue = 0;
    for(;!it->IsDone();it->Next()){
        t = it->Item();
        //age
        t->waitingTime += now - t->lastWait;
        t->lastWait = now;
        if(t->waitingTime > 1500){
            t->priority += 10;
            if(t->priority >= 149) t->priority = 149;
        }
        //put into ready queue
        if(t->priority >= 0 && t->priority <= 49){
            L3->Append(t);
            t->listBelong = 3;
        }else if(t->priority <= 99){
            L2->Insert(t);
            t->listBelong = 2;
        }else if(t->priority <= 149){
            L1->Insert(t);
            t->listBelong = 1;
            double remain = t->apprBurstTime - (double)t->CPUBurstTime;
            BurstTime_min = min(BurstTime_min, remain);
        }

        higherQueue |= t->listBelong < kernel->currentThread->listBelong;
    }
    //3 cases for preempting
    //1. there exist a thread from higher queue
    //2. L1 thread with lower approximate CPU burst time
    //3. round robin for L3 queue
    double cur_remainTime = kernel->currentThread->apprBurstTime - (double)t->CPUBurstTime;
    preempting = (BurstTime_min < cur_remainTime) || higherQueue;
}
void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    // readyList->Append(thread);
    if(thread->priority >= 0 && thread->priority <= 49){
        L3->Append(thread);
        thread->listBelong = 3;
    }else if(thread->priority <= 99){
        L2->Insert(thread);
        thread->listBelong = 2;
    }else if(thread->priority <= 149){
        L1->Insert(thread);
        thread->listBelong = 1;
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    Thread* nextToRun = NULL;
    if(!L1->IsEmpty()){
        nextToRun = L1->RemoveFront();
    } else if(!L2->IsEmpty()){
        nextToRun = L2->RemoveFront();
    } else if(!L3->IsEmpty()){
        nextToRun = L3->RemoveFront();
    }
    return nextToRun;
    // if (readyList->IsEmpty()) {
	// 	return NULL;
    // } else {
    // 	return readyList->RemoveFront();
    // }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    //readyList->Apply(ThreadPrint);
}

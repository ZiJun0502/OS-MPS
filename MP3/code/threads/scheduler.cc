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


//wanyin
int compPriority(Thread* a, Thread* b) { // compare Priority
    if (a->getPriority() == b->getPriority()) {
        return 0;
    }
    if (a->getPriority() > b->getPriority()) {
        return 1;
    }
    if (a->getPriority() < b->getPriority()) {
        return -1;
    }
}
int compSJFS(Thread* a, Thread* b) { // Preepmptive short job first
    if (a->getBurstTime() == b->getBurstTime()) {
        return 0;
    }
    if (a->getBurstTime() > b->getBurstTime()) {
        return 1;
    }
    if (a->getBurstTime() < b->getBurstTime()) {
        return -1;
    }
}


Scheduler::Scheduler()
{ 
    readyList = new List<Thread *>; 
    L1 = new SortedList<Thread *>(cmp1);
    L2 = new SortedList<Thread *>(cmp2);
    L3 = new List<Thread *>;

    // wanyin
    L1_readyList = new SortedList<Thread *>(compSJFS);
    L2_readyList = new SortedList<Thread *>(compPriority);
    L3_readyList = new List<Thread *>;
    toBeDestroyed = NULL;
} 




//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList;
    delete L1;
    delete L2;
    delete L3;

    // wanyin 
    delete L1_readyList;
    delete L2_readyList;
    delete L3_readyList;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    // readyList->Append(thread);
    //TODO
    //if the thread has priority that is smaller than the
    // current thread => ask for a context switch.
    /*
    
    */
    //else append it
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

    

    // wanyin
    // thread->setWaitingTime();
    int p = thread->getPriority();
    if (p > 99) { // L1 
        L1_readyList->Insert(thread);
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << thread->getID() << " ] is inserted into queue L1_readyList");
        // if the current thread has the lowest burst time, not be preempted
        int currentBurstTime = kernel->currentThread->getBurstTime()*0.5 + (kernel->stats->userTicks-kernel->currentThread->stick)*0.5;
        if (currentBurstTime > thread->getBurstTime()) {
            kernel->currentThread->Yield(); 
        }
    } else if (p > 49) {
        L2_readyList->Insert(thread);
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << thread->getID() << " ] is inserted into queue L2_readyList");
    } else if (p >= 0) {
        L3_readyList->Append(thread);
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << thread->getID() << " ] is inserted into queue L3_readyList");
    }
}

// aging mechanism
void aging_mechanism (Thread* a) {
    int old_priority = a->getPriority();
    if (a->getWaitingTime() >= 1500) {
        a->setPriority(old_priority+10);
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << a->getID() << " change its priority from [ "<< old_priority << " to " << old_priority+10);
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
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << kernel->currentThread->getID() << "] is remove from queue L1");
    } else if(!L2->IsEmpty()){
        nextToRun = L2->RemoveFront();
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << kernel->currentThread->getID() << "] is remove from queue L2");
    } else if(!L3->IsEmpty()){
        nextToRun = L3->RemoveFront();
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << kernel->currentThread->getID() << "] is remove from queue L3");
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
    
    // wanyin
    // int currentBurstTime = oldThread->getBurstTime()*0.5 + (kernel->stats->userTicks-oldThread->stick)*0.5;
    // update burst time
    int diff = oldThread->getBurstTime() - (kernel->stats->userTicks-oldThread->stick);
    if (diff > 0) {
        oldThread->setBurstTime(diff); // ??
        DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << oldThread->getID()<< " ] update approximate burst time, from " << "?" <<" , add " << "?" << " , to " << "?" );
    }
    

    nextThread->stick = kernel->stats->userTicks;
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    
    
    DEBUG(dbgMFQ, "Tick [ " << kernel->stats->totalTicks << " ] Thread : [ " << nextThread->getID()<< " ] is now selected for execution, thread [ " << oldThread->getID() <<" ] is replaced, and it has executed [ " << kernel->stats->totalTicks-oldThread->stick << " ]");
    oldThread->stick = kernel->stats->totalTicks;
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
    readyList->Apply(ThreadPrint);
}

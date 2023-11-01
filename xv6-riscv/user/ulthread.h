#ifndef __UTHREAD_H__
#define __UTHREAD_H__

#include <stdbool.h>
#include <stddef.h>
#define MAXULTHREADS 100

struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
  
  uint64 a0;
  uint64 a1;
  uint64 a2;
  uint64 a3;
  uint64 a4;
  uint64 a5;
};
//struct cpu {
//  struct thread *thread;          // The process running on this cpu, or null.
//  struct context context;     // swtch() here to enter scheduler().
//  int noff;                   // Depth of push_off() nesting.
//  int intena;                 // Were interrupts enabled before push_off()?
//};
//extern struct cpu cpus[NCPU];

enum ulthread_state {
  SCHEDULE,
  FREE,
  RUNNABLE,
  YIELD,
};

enum ulthread_scheduling_algorithm {
  ROUNDROBIN,   
  PRIORITY,     
  FCFS,         // first-come-first serve
};
struct thread {
  enum ulthread_state state;
  enum ulthread_scheduling_algorithm algorithm;
  struct context context;
  int tid;
  int priority;

};
#endif

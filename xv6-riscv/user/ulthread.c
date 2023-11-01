    /* CSE 536: User-Level Threading Library */
    #include "kernel/types.h"
    #include "kernel/stat.h"
    #include "kernel/fcntl.h"
    #include "user/user.h"
    #include "user/ulthread.h"
    /* Standard definitions */
    #include <stdbool.h>
    #include <stddef.h> 

    struct thread threads[MAXULTHREADS];
    struct thread *scheduler;
    struct thread *next;
    struct thread *last;
    /* Get thread ID*/
    int get_current_tid() {
        return next->tid;
    }

    /* Thread initialization */
    void ulthread_init(int schedalgo) {
        struct thread *t;
        int i;
        for(t=threads , i=0;t<&threads[MAXULTHREADS];t++,i++){
            t->tid=i;
            t->state=FREE;
            t->context.ra=0;
            t->context.sp=0;
            t->priority=-1;
            t->context.a0=0;
            t->context.a1=0;
            t->context.a2=0;
            t->context.a3=0;
            t->context.a4=0;
            t->context.a5=0;
            if(i==0){
                scheduler=t;
                scheduler->state=SCHEDULE;
                scheduler->algorithm=schedalgo;
            }
        }
        
    }

    /* Thread creation */
    bool ulthread_create(uint64 start, uint64 stack, uint64 args[], int priority) {
        struct thread *t;
        for(t=threads;t<&threads[MAXULTHREADS];t++){
            if(t->state==FREE){
                goto create; 
            }
        }
        return false;
        create:
            
            t->state=RUNNABLE;//Once the thread is created, ULTLib should also track that the thread is now RUNNABLE for scheduling decisions.
            t->priority=priority;
            t->context.ra=start;
            t->context.sp=stack;
            t->context.a0=args[0];
            t->context.a1=args[1];
            t->context.a2=args[2];
            t->context.a3=args[3];
            t->context.a4=args[4];
            t->context.a5=args[5];
            printf("[*] ultcreate(tid: %d, ra: %p, sp: %p)\n", t->tid, t->context.ra, t->context.sp);

        return true;
    }
    bool RoundRobin(void){
    restart:
        next=0;
        int count=0;
        struct thread *t;
        for(t=threads;t<&threads[MAXULTHREADS];t++){
            if(t->state==RUNNABLE){
                next=t;
                break;
            }
        }
        for(t=threads;t<&threads[MAXULTHREADS];t++){
            if(t->state==YIELD){
                count++;
                //t->state=RUNNABLE;
            }
        }
        if(next==0){
            if(count>0){
                for(t=threads;t<&threads[MAXULTHREADS];t++){
                    if(t->state==YIELD){ 
                        t->state=RUNNABLE;
                    }
                }
                goto restart;
            }
            else{
                return false;
            }
        }
        if(scheduler!=next){
            printf("[*] ultschedule (next tid: %d)\n", next->tid);
            ulthread_context_switch(&scheduler->context, &next->context);
        }
    }

    bool Priority(void){
    restart:
        struct thread *t;
        next=0;
        int max=-20;
        int count=0;
        for(t=threads;t<&threads[MAXULTHREADS];t++){////just find the runnable one from 0->100
            if(t->state==RUNNABLE){
                if(t->priority>max){
                    max=t->priority;
                    next=t;
                    last=t;
                }
            }
        }
        for(t=threads;t<&threads[MAXULTHREADS];t++){
            if(t->state==YIELD){
                count++;
                t->state=RUNNABLE;
            }
        }
        
        if(next==0){
            if(count==0){
                return false;
            }
            goto restart;
        }
        if(scheduler!=next){
         
            printf("[*] ultschedule (next tid: %d)\n", next->tid);
            ulthread_context_switch(&scheduler->context, &next->context);
            
        }else{
            next=0;
        }
	

    }
    bool FirstComeFisrtServe(void){
        struct thread *t;
        next=0;
        for(t=threads;t<&threads[MAXULTHREADS];t++){////just find the runnable one from 0->100
            if(t->state==RUNNABLE){
                next=t;
                break;
            }
        }
        if(next==0){
            return false;
        }

        if(scheduler!=next&&next!=0){

            printf("[*] ultschedule (next tid: %d)\n", next->tid);
            // Switch betwee thread contexts
            ulthread_context_switch(&scheduler->context, &next->context);
            
        }else{
            next=0;
        }
        
    }
    /* Thread scheduler */
    void ulthread_schedule(void) {
        uint64 time;
        while (true)
        {   
            
            if(threads[0].algorithm==PRIORITY){
                
                if(Priority()==false){
                    return;
                }
            }
            else if(threads[0].algorithm==FCFS){
                if(FirstComeFisrtServe()==false){
                    return;
                }
            }
            else if(threads[0].algorithm==ROUNDROBIN){
                if(RoundRobin()==false){
                    return;
                }
            }
            else{
                printf("Not Build yet\n");
            }     
   
        

            
            
        }
    }



    /* Yield CPU time to some other thread. */
    void ulthread_yield(void) {
        ///if that specific thread pass cpu time then set it as yield
        printf("[*] ultyield(tid: %d)\n",next->tid );
        next->state=YIELD;
        ulthread_context_switch(&next->context,&scheduler->context);
    }

    /* Destroy thread */
    void ulthread_destroy(void) {
        printf("[*] ultdestroy(tid: %d)\n",next->tid);
        next->state=FREE;
        ulthread_context_switch(&next->context, &scheduler->context);
    }

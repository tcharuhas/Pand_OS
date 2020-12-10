/************************INTERRUPTS.C**********************************************
 *
 * The purpose of this file is to handle interrupts raised.  The highest 
 * priority interrupt is determined handles said interrupt.
 * The interrupt is acknowledged, and the device’s status code is
 * passed along to the pcb (i.e. process) that got unblocked as a 
 * result of the interrupt; this is the pcb that was waiting for the I/O 
 * to complete. The newly unblocked pcb is enqueued back on the
 * Ready Queue and control is returned to the Current Process.  This is
 * repeated until all interrupts are handled.
 * 
 *  Written by: Alex D & Tyler C
 * 
 */


#include "../h/const.h"
#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/initial.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "/usr/local/include/umps3/umps/libumps.h"

int zStop;
/*
    interruptHandler is what is called when there is an interrupt pending. 
    This fuction runs though all the possible interrupts to see the highest one
    pending and handles said interrupt by branching off to the appropriate function, based
    off the value of INTERRUPT_CHECK.
*/

void interruptHandler(){
    
    /*grab cause to see interrupt type*/
    unsigned int CoI = (((state_t*) BIOSDATAPAGE)->s_cause);
    
    /*used in testing to see what the interrupt type is */
    int hasMatch = 0;
    /* defaulted to the hex representation of the first possible interrupt type (PLT). */
    int IntrCheck = INTERRUPT_CHECK; 
    
    /* while no match has been found iterate down the cause reg */
    while(hasMatch == 0){
        /* by logically ANDing those together, we can determine the interrupt. */
        if ((CoI & IntrCheck) != 0){
            switch(IntrCheck){ /* NOTE each CONSTANT in this switch is the numarical value of the interrupt from the cause register */
                case INTERRUPT_LOCALT:
                    Handle_PLT();
                    break;
                case INTERRUPT_TIMER:
                    Handle_Timer();
                    break;
                case INTERRUPT_DISK:
                    Handle_IO(DISKINT);
                    break;
                case INTERRUPT_FLASH:
                    Handle_IO(FLASHINT);
                    break;
                case INTERRUPT_PRINT:
                    Handle_IO(PRNTINT);
                    break;
                case INTERRUPT_TERM:
                    Handle_IO(TERMINT);
                    break;
            }
        }
        /*if we iterate through all the interrupt types and did not find the one in our cause register we panic */
        if (IntrCheck > INTERRUPT_CHECKEND){ 
            PANIC();
        }
        /*shift and move on to the next bit*/
        IntrCheck = IntrCheck << SHIFT_INTER;
        
    }
    
}

 
/*
    Handle_PLT handles the pseudo timer interrupt by acknowledging the interrupt,
    copying the state at the time of the interrupt, and transitioning this process 
    from "running" to "ready" (placing this process back on the ready queue). 
    The scheduler is called to load the next job in line.

*/
void Handle_PLT (){   
    /*set when we've stopped*/
    cpu_t TOD_stop;
    STCK(TOD_stop);
    
    if (currentproc != NULL){
        /*change the time*/
        currentproc->p_time += (TOD_stop - TOD_start);
        
        /*store state*/
        setState((state_t*) BIOSDATAPAGE, &(currentproc->p_s));
        
        /*shove it back on the queue and get it outta here*/
        insertProcQ (&ready_h, currentproc);
        scheduler();
    }
    else{
        PANIC();
    }
    
}

/*

  Handle_Timer handles the pseudo timer by reseting the pseudo
  clock, unblocking all the process on the pseudo clock, and then
  resetting the semaphore for it. It returns control to the current process
  after doing so.

*/

void Handle_Timer(){
    /*acknowledge the interrupt*/
    LDIT(INTERVALTIME);
    
    /*Unblock ALL pcbs on the Pseudo-clock semaphore*/
    pcb_t* removed = removeBlocked(&(devList[CLOCK]));
    while (removed != NULL){
        insertProcQ (&ready_h, removed);
        softblockcnt -= 1;
        removed = removeBlocked(&(devList[CLOCK]));
    }
    
    devList[CLOCK] = 0;
    
    /*if the current process is NULL go back to scheduler to either fix that or keep waiting*/
    if(currentproc == NULL){
        scheduler();
    } else { 
        switchContext(currentproc);
    }
    
}

/*
   Handle_IO handles all non-timer interrupts. The type of
   device that caused the interrupt is passed to it by interruptHandler.
   The interrupt is acknowledged, has a V operation performed on it,
   and puts the newly unblocked process back on the ready queue.  Control
   is returned to the current process.

*/
void Handle_IO(int lineNum){

    /* devRegs and device mapping are all initialized.  Status is declared for later. */
    volatile devregarea_t* devRegs = (devregarea_t*) RAMBASEADDR;
    unsigned int devMap = (devRegs->interrupt_dev[(lineNum - DISKINT)]);
    unsigned int status;
    

    /* while loop vars */
    int devNum = -1;
    int count = 0;
    
    /* defaulted to the hex representation of the first possible interrupt type (PLT). */
    int devCheck = DEVICE_CHECK;  

    /* loop though all the device nums till you find the one you're looking for*/
    while (devNum == -1){
        /* by logically ANDing those together, we can determine the device type. */
        if ((devMap & devCheck) != 0){
            devNum = count;
        }
        else{
            /*shift down to the next possible device number and then increase the count*/
            devCheck = devCheck << SHIFT_INTER;
            count ++;
        }
    }

    /*get the device semaphore address*/
    int devSem = ((lineNum - DISKINT) * DEVPERINT) + devNum;
    
    /*if the we have a TERMINT*/
    if(lineNum == TERMINT){
        /* Acknowledge the outstanding interrupt. */
        /* if the terminal is ready set the status and acknowledge */
        if(((devRegs->devreg[devSem]).t_transm_status & READYBIT) != READY){
            status = (devRegs->devreg[devSem]).t_transm_status;
            (devRegs->devreg[devSem]).t_transm_command = ACK;
        }
        else{
            /* if not ready set status to ready to transmit, acknowledge and increment the device semaphore */
            status = (devRegs->devreg[devSem]).t_recv_status;
            (devRegs->devreg[devSem]).t_recv_command = ACK;
            (devSem) += DEVPERINT;
        }
    }
    else{
        /*acknowledge the interrupt*/
        status = (devRegs->devreg[devSem]).d_status;
        (devRegs->devreg[devSem]).d_command = ACK;
    }
    /*perform v*/
    devList[devSem] ++;
    zStop = 1;
    /* if something is blocked on the devList then unblock it and stick it back on the ready queue */
    if (devList[devSem] <= 0){
        pcb_t* removed = removeBlocked (&(devList[devSem]));
        if(removed != NULL) {
            removed->p_s.s_v0 = status;
            insertProcQ (&ready_h, removed);
            softblockcnt --;
        }
        
    }
    /*if the current process is NULL go back to scheduler to either fix that or keep waiting*/
    if(currentproc == NULL) {
        scheduler();
    } else {
        /* interrupt handled, switchcontext back to current process */
        cpu_t TOD_stop;
        STCK(TOD_stop);

        /* reassigns the time spent on the process before the interrupt */
        currentproc->p_time = (TOD_stop - TOD_start);
        
        /*send it back off to work*/
        /*fix the clock*/
        STCK(TOD_start);
        setTIMER(getTIMER());            
        switchContext(currentproc);
        }
    
}



        

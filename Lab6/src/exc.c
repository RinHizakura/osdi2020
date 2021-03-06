#include "include/uart.h"
#include "include/string.h"
#include "include/utils.h"
#include "include/timer.h"
#include "include/mm.h"
#include "include/scheduler.h"
#include "include/fork.h"
#include "include/irq.h"
#include "include/peripherals/uart.h"
#include "include/signal.h"
#include "include/queue.h"
#include "include/reboot.h"
#include "include/exc.h"
#include "include/kernel.h"
#include "include/arm/sysreg.h"
#include "include/pool.h"
void exception_handler(unsigned long type,unsigned long esr, \
		unsigned long elr){
        
	switch(type){
		case 0:uart_send_string("\r\nSynchronous");break;
		case 1:uart_send_string("\r\nIRQ");break;
		case 2:uart_send_string("\r\nFIQ");break;	
		case 3:uart_send_string("\r\nSError");break;	
		case 4:uart_send_string("\r\nSynchronous at 0x400");break;	
		case 5:uart_send_string("\r\nIRQ at 0x480");break;
	}
	uart_send_string(":");


	// decode some of exception type 
    	switch(esr>>26) {
        	case 0b000000: uart_send_string("Unknown"); break;
		case 0b010101: uart_send_string("System call"); break;
       		case 0b100100: uart_send_string("Data abort, lower EL"); break;
                case 0b100101: uart_send_string("Data abort, same EL"); break;
		case 0b011000: uart_send_string("Exception from MSR, MRS, or System instruction execution in AArch64 state");break;
		case 0b111100: uart_send_string("BRK instruction execution in AArch64 state");break;			       
		case 0b100000: uart_send_string("Instruction Abort from a lower Exception level");break;
		case 0b100001: uart_send_string("Instruction Abort taken without a change in Exception level");break;
		
		default: uart_send_string("Unknown...?"); break;
        }
        
        if(esr>>26==0b100100 || esr>>26==0b100101 || esr>>26==0b100000  ||esr>>26==0b100001 ) {
        	uart_send_string(", ");
        	switch((esr>>2)&0x3) {
            		case 0: uart_send_string("Address size fault"); break;
            		case 1: uart_send_string("Translation fault"); break;
            		case 2: uart_send_string("Access flag fault"); break;
            		case 3: uart_send_string("Permission fault"); break;
        	}
        	switch(esr&0x3) {
            		case 0: uart_send_string(" at level 0"); break;
            		case 1: uart_send_string(" at level 1"); break;
            		case 2: uart_send_string(" at level 2"); break;
            		case 3: uart_send_string(" at level 3"); break;
        		}
    	}


	// elr: return address
        uart_send_string("\r\nException return address: 0x");
	uart_hex(elr>>32);
	uart_hex(elr);

	// EC[31:26]: Exception class
	uart_send_string("\r\nException class(EC): 0x");
	uart_hex(esr>>26);

        // ISS[24:0]: Instruction Specific Syndrome
        uart_send_string("\r\nInstruction specific syndrome (ISS): 0x");
	uart_hex(( ((unsigned int)esr)<<7)>>7);

	uart_send_string("\r\n");
}

// Now I just use no more than 6 argument
unsigned long el0_svc_handler(size_t arg0,size_t arg1,size_t arg2,size_t arg3,\
		size_t arg4,size_t arg5, size_t sys_call_num){
	enable_irq();

	switch(sys_call_num){
		// Core timer
		case CORE_TIMER:{
			core_timer_enable();
			return 0;
		}
		// DAIF information
		case DAIF:{
			unsigned int daif;
          		asm volatile ("mrs %0, daif" : "=r" (daif));
          		printf("DAIF is %x\r\n",daif);
			
			return 0;
		}
		// kill
		case SYS_KILL:{
			struct task_struct *p = task[arg0];
			if(p && p->signal.pending!=SIGKILL)
				p -> signal.pending = SIGKILL;

			else
				printf("@@@ Signal failed\r\n");
			return 0;
		}
		// fork
		case SYS_FORK:{
			return user_task_create();
		}
		// exec
		case SYS_EXEC:{
			return 0;
		 	//return do_exec((void *)arg0);      
		}
		// exit
		case SYS_EXIT:{
			exit_process();
			return 0;
		}
		// get task pid
		case SYS_GET_TASKID:{
			return current->pid;
		}
		//  uart write
		case SYS_UART_WRITE:{
			// Using blocking write for safety
			preempt_disable();	
			
			int success = 0;
			int ret = 0;
			
			for(unsigned int i=0; i<arg1;i++){
				ret = uart_send(((char*)arg0)[i]);
				if(ret==0)
					++success;
			}

			preempt_enable();
		 	return success;	
		}
		// uart read
		case SYS_UART_READ:{	 
		      char recv_char;
		      unsigned int i = 0;
		      int flag = 0;
		   
		      for(;i<arg1;i++){
		      		// put task in waitQ and wait
			        current->state = TASK_WAIT;
		      		priorityQ_push(&waitqueue,1,current->pid); 
				
				//recv and send
				recv_char = uart_recv();		
				uart_send(recv_char);

				if(recv_char =='\n' || recv_char == '\r'){
					flag = 1;
					break;
				}
				else{ 
					((char*)arg0)[i] = recv_char;
				}
			}
			
			while(flag==0){
		      		// put task in waitQ and wait
			        current->state = TASK_WAIT;
				priorityQ_push(&waitqueue,1,current->pid); 
				
				//recv and send
				recv_char = uart_recv();
				uart_send(recv_char);

				if(recv_char =='\n' || recv_char == '\r')
					break;
			}	
			
			// send "\r\n"
			uart_send('\r');
			uart_send('\n');
			return i;	
		}
		// user_printf: allow only one argument now
		case SYS_UART_PRINT:{
			printf((char *)arg0,arg1,arg2,arg3,arg4);
			return 0;
		}
		// reboot
		case SYS_REBOOT:{
			reset(10000);
			return 0;	
		}
		// delay
		case SYS_DELAY:{
			delay(arg0);
			return 0;
		}
		// remain page num
		case SYS_REMAIN_PAGE:{
			return remain_page;
		}
		case SYS_MMAP: {
			return (unsigned long)mmap((void *)arg0,arg1,arg2,arg3,(void *)arg4,arg5);	       
		}
		case SYS_WAIT: {
			return current->state = TASK_WAIT;
		}	
		case SYS_MALLOC: {
			if(arg0 > (MIN_DEFAULT_ALLOCATOR_SIZE * DEFAULT_ALLOCATOR_NUM)){
				printf("$$$ allocate by mmap\r\n");
				return (unsigned long)mmap(NULL, arg0, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, NULL, 0);
			}
			else{
				int allocator_num;
		                
				for(int i=0;i<DEFAULT_ALLOCATOR_NUM;i++){
	                        	if(arg0 <= (unsigned long)MIN_DEFAULT_ALLOCATOR_SIZE*(i+1)){
                                 		allocator_num = i;
                                 		break;
                         		}
	                 	}
                 		printf("$$$ allocate from allocator number %d\r\n",allocator_num);
				return pool_alloc_user(&(default_allocator[current->pid][allocator_num]));

			}
		}
		case SYS_FREE:{
			 // if your memory was allocated from pool, put it back
	        	for(int i=0;i<DEFAULT_ALLOCATOR_NUM;i++){
                 		pool tmp_pool = default_allocator[current->pid][i];
                 		for(int page=0; page<=tmp_pool.page; page++){
                         		if ( ((arg0>>12)<<12) == (((tmp_pool.pages_addr[page].vir_addr)>>12)<<12)){
                                 		pool_free(&default_allocator[current->pid][i],arg0);
                                 		printf("*** free to allocator number %d\r\n",i);
                                 		return 0;
                         		}
                 		}
         		}
			return free_user_page(arg0);	      
		}
		case SYS_OBJ_ALLOCATOR_INIT:{
			return allocator_register(arg0);			    
		}
		case SYS_OBJ_ALLOC:{
			return allocator_user_alloc(arg0);  
		}
		case SYS_OBJ_FREE:{
			allocator_free(arg0,arg1);		  
			return 0;
		}
		case SYS_OBJ_ALLOCATOR_FREE:{
			allocator_user_unregister(arg0);
			return 0;	
		} 
	}
	// Not here if no bug happened!
	return -1;
}



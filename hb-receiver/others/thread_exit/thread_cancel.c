#include <pthread.h>                                                            
#include <stdio.h>                                                              
#include <stdlib.h>                                                             
#include <unistd.h>                                                             
                                                                                
int footprint=0;                                                                

void cleanup(void *arg) {
        printf("cleanup: %s\n", (char *)arg);
}

void *thread(void *arg) {                                                       
                                                                                
  pthread_cleanup_push(cleanup, "!!");                                          
                                                                                
  footprint++;                                                                  
  while (1)                                                                     
    sleep(1);                                                               

  puts("before pop in thread.\n");
  pthread_cleanup_pop(1);                                                       
  puts("after pop in thread.\n");
}                                                                               
                                                                                
main() {                                                                        
  pthread_t thid;                                                               
                                                                                
  if (pthread_create(&thid, NULL, thread, NULL) != 0) {                         
    perror("pthread_create() error");                                           
    exit(1);                                                                    
  }                                                                             
                                                                                
  while (footprint == 0)                                                        
    sleep(1);                                                                   
                                                                                
  puts("IPT is cancelling thread");                                             
                                                                                
  if (pthread_cancel(thid) != 0) {                                              
    perror("pthread_cancel() error");                                           
    exit(3);                                                                    
  }                                                                             
             
  puts("before join\n");

  if (pthread_join(thid, NULL) != 0) {                                          
    perror("pthread_join() error");                                             
    exit(4);                                                                    
  }                                                                             
  puts("after join\n");

}         

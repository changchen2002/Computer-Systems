#include <pthread.h>
// to create threads:
pthread_t T1, T2, t[10];

//you can create your own struct to store info about a thread
typedef struct thread_info { 
    int x; 
    int y;
} thread_info_t;

void * thread_function(void *vinfo) {
    thread_info_t* info = (thread_info_t *)vinfo;
    ...
    pthread_exit(NULL);
}
int main(){
    thread_info_t *info= (thread_info_t *)malloc(sizeof(thread_info_t));
    info->x = …
    info->y = …
    pthread_create(&T1, NULL, thread_function, info);
    ...
    pthread_join(T1, NULL);
}
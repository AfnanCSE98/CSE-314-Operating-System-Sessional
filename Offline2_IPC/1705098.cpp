#include<cstdio>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include <unistd.h>
#include <bits/stdc++.h>
using namespace std;

const int m = 5, n = 2 , p = 3;
int w = 6, x = 6 , y = 6 , z = 2;
int tm_tmp=0;
pthread_mutex_t console;

sem_t mtx;
sem_t kiosk_empty_sem;
bool is_kiosk_empty[m];

int belt_ara[n];
sem_t belt_empty_sem[n];
bool is_belt_empty[n];

struct arg_struct {
    int thread_id;
    int is_vip;
};

void * time_thread(void * arguments){
    while(1){
        sleep(1);
        tm_tmp++;

    }
}

void security_chk(int thread_id){
    int tmp = rand() % n;
    pthread_mutex_lock(&console);
    cout<<"Passenger "<<thread_id<<" has started waiting for security check in belt "<<tmp+1<<" at "<<tm_tmp<<endl;
    pthread_mutex_unlock(&console);
    
    sem_wait(&belt_empty_sem[tmp]);
    sleep(y);
    pthread_mutex_lock(&console);
    cout<<"Passenger "<<thread_id<<" has crossed the security check"<<" at "<<tm_tmp<<endl;
    pthread_mutex_unlock(&console);

    sem_post(&belt_empty_sem[tmp]);

}

void pass_kiosk(int thread_id){
    
    int idx;
    sem_wait(&kiosk_empty_sem);
    sem_wait(&mtx);
    
    for(int i = 0; i < m ; i++ ){
        if(is_kiosk_empty[i]){
            is_kiosk_empty[i] = false;      
            printf("Passenger %d has started waiting in kiosk %d at %d \n" , thread_id , i+1 , tm_tmp);
            idx = i;
            break;
        }
    } 
    sem_post(&mtx);
    sleep(w);

    sem_wait(&mtx);
    is_kiosk_empty[idx] = true;
    printf("Passenger %d has got his boarding pass at %d \n" , thread_id , tm_tmp);
    sem_post(&mtx);

    sem_post(&kiosk_empty_sem);
      
}

void boarding_gate(int thread_id , bool has_boarding_pass){

}

void * passengerActivity(void * arguments){
    //struct arg_struct *args = (struct arg_struct *)arguments;
    pair<int , bool>*p = (pair<int , bool> *)arguments;
    pass_kiosk(p->first);
    if(p->second){
        //boarding_gate(thread_id , has_boarding_pass);
    }
    else{
        security_chk(p->first);
        //boarding_gate(thread_id , has_boarding_pass);
    }
    pthread_exit(NULL);
}

void init(){
    sem_init(&mtx , 0 , 1);
    sem_init(&kiosk_empty_sem , 0 , m);
    for(int i=0 ; i<m; i++){
        is_kiosk_empty[i] = true;
    }
    for(int i=0 ; i<n; i++){
        sem_init(&belt_empty_sem[i] , 0 , p);
        is_belt_empty[i] = true;
    }
    pthread_mutex_init(&console,0);
}

int main(){
    init();
    pthread_t t;
    pthread_create(&t , NULL , time_thread , NULL);
    int no_of_passengers = 10;
    pthread_t passengers[no_of_passengers];
    
    for(int i=0 ; i<no_of_passengers ; i++){
        pair<int , bool>*p = new pair<int , bool>(i+1 , false);
        //struct * arg_struct args = new ;
        //args->thread_id = i+1;
        //args->is_vip = false;
        //printf("debugging %d\n" , args->thread_id);
        pthread_create(&passengers[i],NULL,passengerActivity,(void *)p);
    }
    while(1);
    return 0;
}
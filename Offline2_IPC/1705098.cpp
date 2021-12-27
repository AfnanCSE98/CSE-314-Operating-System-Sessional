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
int forward_cnt = 0;
int backward_cnt = 0;

pthread_mutex_t board_mtx;
pthread_mutex_t forward_cnt_mtx;
pthread_mutex_t backward_cnt_mtx;
pthread_mutex_t vip_moving_mtx;
pthread_mutex_t channel_blocked_mtx;
pthread_mutex_t esp_kiosk_mtx;


sem_t mtx;
sem_t kiosk_empty_sem;
bool is_kiosk_empty[m];

int belt_ara[n];
sem_t belt_empty_sem[n];
bool is_belt_empty[n];

void * time_thread(void * arguments){
    while(1){
        sleep(1);
        tm_tmp++;
    }
}

void get_pass_from_special_kiosk(int thread_id , bool is_vip){
    if(is_vip)printf("Passenger %d (VIP) has started waiting in special kiosk at %d\n" , thread_id , tm_tmp);
    else printf("Passenger %d has started waiting in special kiosk at %d\n" , thread_id , tm_tmp);
    
    pthread_mutex_lock(&esp_kiosk_mtx);
    sleep(w);
    if(is_vip)printf("Passenger %d (VIP) has got his boarding pass from special kiosk at %d \n" , thread_id , tm_tmp);
    printf("Passenger %d (VIP) has got his boarding pass from special kiosk at %d \n" , thread_id , tm_tmp);
    pthread_mutex_unlock(&esp_kiosk_mtx);

}

void go_forward(int thread_id , bool is_vip){
    if(is_vip)printf("Passenger %d (VIP) has arrived at VIP channel at %d\n" , thread_id , tm_tmp);
    else printf("Passenger %d has arrived at VIP channel at %d\n" , thread_id , tm_tmp);
    pthread_mutex_lock(&forward_cnt_mtx);
    forward_cnt++;
    if(forward_cnt == 1){
        pthread_mutex_lock(&vip_moving_mtx);
        pthread_mutex_lock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&forward_cnt_mtx);

    sleep(z);

    if(is_vip)printf("Passenger %d (VIP) has crossed the VIP channel at %d\n" , thread_id , tm_tmp);
    else printf("Passenger %d has crossed the VIP channel at %d\n" , thread_id , tm_tmp);
    
    pthread_mutex_lock(&forward_cnt_mtx);
    forward_cnt--;
    if(forward_cnt == 0){
        pthread_mutex_unlock(&vip_moving_mtx);
        pthread_mutex_unlock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&forward_cnt_mtx);
}


void go_backward(int thread_id , bool is_vip){
    if(is_vip)printf("Passenger %d (VIP) has arrived at backward VIP channel at %d\n" , thread_id , tm_tmp);
    else printf("Passenger %d has arrived at backward VIP channel at %d\n" , thread_id , tm_tmp);
    
    pthread_mutex_lock(&backward_cnt_mtx);
    backward_cnt++;
    if(backward_cnt == 1){
        pthread_mutex_lock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&backward_cnt_mtx);

    sleep(z);

    if(is_vip)printf("Passenger %d (VIP) has passed the VIP channel to reach kiosk at %d\n" , thread_id , tm_tmp);
    else printf("Passenger %d has passed the VIP channel to reach kiosk at %d\n" , thread_id , tm_tmp);
    
    pthread_mutex_lock(&backward_cnt_mtx);
    backward_cnt--;
    if(backward_cnt == 0){
        pthread_mutex_unlock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&backward_cnt_mtx);

}

void security_chk(int thread_id){
    int tmp = rand() % n;
    printf("Passenger %d has started waiting for security check in belt %d at %d\n" , thread_id , tmp+1 , tm_tmp);
    
    sem_wait(&belt_empty_sem[tmp]);
    sleep(x);
    printf("Passenger %d has crossed the security check at %d\n" , thread_id , tm_tmp);
    sem_post(&belt_empty_sem[tmp]);

}

void pass_kiosk(int thread_id , bool is_vip){ 
    int idx;
    sem_wait(&kiosk_empty_sem);
    sem_wait(&mtx);
    
    for(int i = 0; i < m ; i++ ){
        if(is_kiosk_empty[i]){
            is_kiosk_empty[i] = false; 
            if(is_vip){
                printf("Passenger %d (VIP) has started waiting in kiosk %d at %d \n" , thread_id , i+1 , tm_tmp);    
            }     
            else printf("Passenger %d has started waiting in kiosk %d at %d \n" , thread_id , i+1 , tm_tmp);
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

bool boarding_gate(int thread_id , bool is_vip){
    if(rand()%2 == 0){
        if(is_vip)printf("Passenger %d (VIP) has lost his boaring pass!\n" , thread_id);
        else printf("Passenger %d has lost his boaring pass!\n" , thread_id);
        return false;
    }
    if(is_vip)printf("Passenger %d (VIP) has started waiting for boarding at %d\n" , thread_id , tm_tmp);
    else printf("Passenger %d has started waiting for boarding at %d\n" , thread_id , tm_tmp);
    
    pthread_mutex_lock(&board_mtx);
    sleep(y);
    if(is_vip)printf("Passenger %d (VIP) has boarded the plane at %d\n" , thread_id , tm_tmp);
    else printf("Passenger %d has boarded the plane at %d\n" , thread_id , tm_tmp);
    pthread_mutex_unlock(&board_mtx);

    return true;
}



void * passengerActivity(void * arguments){
    pair<int , bool>*p = (pair<int , bool> *)arguments;
    int thread_id = p->first;
    bool is_vip = p->second;
    pass_kiosk(thread_id , is_vip);
    if(is_vip){
        go_forward(thread_id , is_vip);        
    }
    else{
        security_chk(thread_id);
    }
    while(!boarding_gate(thread_id , is_vip)){
        go_backward(thread_id , is_vip);
        get_pass_from_special_kiosk(thread_id , is_vip);
        go_forward(thread_id , is_vip);
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
    pthread_mutex_init(&board_mtx,NULL);
}

int main(){
    init();
    pthread_t t;
    pthread_create(&t , NULL , time_thread , NULL);
    int no_of_passengers = 10;
    pthread_t passengers[no_of_passengers];
    bool is_vip;
    for(int i=0 ; i<no_of_passengers ; i++){
        is_vip = (rand()%2)==0?true : false;
        pair<int , bool>*p = new pair<int , bool>(i+1 , is_vip);
        pthread_create(&passengers[i],NULL,passengerActivity,(void *)p);
    }
    while(1);
    return 0;
}
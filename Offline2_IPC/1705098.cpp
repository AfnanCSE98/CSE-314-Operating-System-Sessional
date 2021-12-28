#include<cstdio>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include <unistd.h>
#include <random>
#include <bits/stdc++.h>
using namespace std;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

ofstream ofs("logfile", ios_base::out );
pthread_mutex_t ofs_mtx;

int m,n,p,w,x,y,z;
int curr_time=0;
int forward_cnt = 0;
int backward_cnt = 0;

pthread_mutex_t board_mtx;
pthread_mutex_t forward_cnt_mtx;
pthread_mutex_t backward_cnt_mtx;
pthread_mutex_t vip_moving_mtx;
pthread_mutex_t channel_blocked_mtx;
pthread_mutex_t esp_kiosk_mtx;

default_random_engine bernouli_generator;
bernoulli_distribution bernouli_dist(0.2);

sem_t mtx;
sem_t kiosk_empty_sem;
bool is_kiosk_empty[10000];

//int belt_ara[n];
sem_t belt_empty_sem[10000];
//bool is_belt_empty[n];

void read_file(){
    fstream myfile("input", std::ios_base::in);
    myfile >> m >> n >> p;
    myfile >> w >> x >> y >> z;
}

void * time_thread(void * arguments){
    while(1){
        sleep(1);
        curr_time++;
    }
}

void get_pass_from_special_kiosk(int thread_id , bool is_vip){
    if(is_vip)printf("Passenger %d (VIP) has started waiting in special kiosk at %d\n" , thread_id , curr_time);
    else printf("Passenger %d has started waiting in special kiosk at %d\n" , thread_id , curr_time);
    
    pthread_mutex_lock(&esp_kiosk_mtx);
    sleep(w);

    if(is_vip)printf("Passenger %d (VIP) has got his boarding pass from special kiosk at %d \n" , thread_id , curr_time);
    else printf("Passenger %d has got his boarding pass from special kiosk at %d \n" , thread_id , curr_time);
    
    pthread_mutex_unlock(&esp_kiosk_mtx);

}

void go_forward(int thread_id , bool is_vip){
    if(is_vip)printf("Passenger %d (VIP) has arrived at VIP channel at %d\n" , thread_id , curr_time);
    else printf("Passenger %d has arrived at VIP channel at %d\n" , thread_id , curr_time);

    pthread_mutex_lock(&forward_cnt_mtx);
    forward_cnt++;
    if(forward_cnt == 1){
        pthread_mutex_lock(&vip_moving_mtx);
        pthread_mutex_lock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&forward_cnt_mtx);

    sleep(z);

    if(is_vip)printf("Passenger %d (VIP) has crossed the VIP channel at %d\n" , thread_id , curr_time);
    else printf("Passenger %d has crossed the VIP channel at %d\n" , thread_id , curr_time);
    
    pthread_mutex_lock(&forward_cnt_mtx);
    forward_cnt--;
    if(forward_cnt == 0){
        pthread_mutex_unlock(&vip_moving_mtx);
        pthread_mutex_unlock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&forward_cnt_mtx);
}


void go_backward(int thread_id , bool is_vip){
    if(is_vip)printf("Passenger %d (VIP) has arrived at backward VIP channel at %d\n" , thread_id , curr_time);
    else printf("Passenger %d has arrived at backward VIP channel at %d\n" , thread_id , curr_time);
    
    pthread_mutex_lock(&backward_cnt_mtx);
    backward_cnt++;
    if(backward_cnt == 1){
        pthread_mutex_lock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&backward_cnt_mtx);

    sleep(z);

    if(is_vip)printf("Passenger %d (VIP) has passed the VIP channel to reach special kiosk at %d\n" , thread_id , curr_time);
    else printf("Passenger %d has passed the VIP channel to reach special kiosk at %d\n" , thread_id , curr_time);
    
    pthread_mutex_lock(&backward_cnt_mtx);
    backward_cnt--;
    if(backward_cnt == 0){
        pthread_mutex_unlock(&channel_blocked_mtx);
    }
    pthread_mutex_unlock(&backward_cnt_mtx);

}

bool get_bernouli_status(){
    return bernouli_dist(bernouli_generator);

}

void security_chk(int thread_id){
    int tmp = rand() % n;
    printf("Passenger %d has started waiting for security check in belt %d at %d\n" , thread_id , tmp+1 , curr_time);
    
    sem_wait(&belt_empty_sem[tmp]);
    sleep(x);
    printf("Passenger %d has crossed the security check at %d\n" , thread_id , curr_time);
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
                printf("Passenger %d (VIP) has started waiting in kiosk %d at %d \n" , thread_id , i+1 , curr_time);    
            }     
            else printf("Passenger %d has started waiting in kiosk %d at %d \n" , thread_id , i+1 , curr_time);
            idx = i;
            break;
        }
    } 
    sem_post(&mtx);
    sleep(w);

    sem_wait(&mtx);
    is_kiosk_empty[idx] = true;
    if(is_vip)printf("Passenger %d (VIP)has got his boarding pass at %d \n" , thread_id , curr_time);
    else printf("Passenger %d has got his boarding pass at %d \n" , thread_id , curr_time);
    sem_post(&mtx);

    sem_post(&kiosk_empty_sem);
      
}

bool boarding_gate(int thread_id , bool is_vip){
    if(get_bernouli_status()){
        if(is_vip)printf(ANSI_COLOR_RED "Passenger %d (VIP) has lost his boaring pass at %d"  ANSI_COLOR_RESET "\n", thread_id,curr_time);
        else printf(ANSI_COLOR_RED "Passenger %d has lost his boaring pass at %d" ANSI_COLOR_RESET "\n", thread_id,curr_time);
        return false;
    }
    if(is_vip)printf("Passenger %d (VIP) has started waiting for boarding at %d\n" , thread_id , curr_time);
    else printf("Passenger %d has started waiting for boarding at %d\n" , thread_id , curr_time);
    
    pthread_mutex_lock(&board_mtx);
    sleep(y);
    if(is_vip){
        printf(ANSI_COLOR_GREEN "Passenger %d (VIP) has boarded the plane at %d" ANSI_COLOR_RESET "\n", thread_id , curr_time);
        pthread_mutex_lock(&ofs_mtx);
        ofs<<"Passenger "<<thread_id<<" (VIP) has boarded the plane at "<<curr_time<<endl;
        pthread_mutex_unlock(&ofs_mtx);
    }
    else {
        printf(ANSI_COLOR_GREEN "Passenger %d has boarded the plane at %d" ANSI_COLOR_RESET "\n", thread_id , curr_time);
        pthread_mutex_lock(&ofs_mtx);
        ofs<<"Passenger "<<thread_id<<" has boarded the plane at "<<curr_time<<endl;
        pthread_mutex_unlock(&ofs_mtx);
    }
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
        //is_belt_empty[i] = true;
    }
    pthread_mutex_init(&board_mtx,NULL);
    pthread_mutex_init(&ofs_mtx , NULL);
    pthread_mutex_init(&forward_cnt_mtx , NULL);
    pthread_mutex_init(&backward_cnt_mtx , NULL);
    pthread_mutex_init(&channel_blocked_mtx , NULL);
    pthread_mutex_init(&vip_moving_mtx , NULL);
    pthread_mutex_init(&esp_kiosk_mtx , NULL);
}

void * create_passengers(void * args){
    default_random_engine generator;
    double *tmp = (double *)args;
    double lambda = *tmp;
    poisson_distribution<int> distribution(lambda);
    bool is_vip;
    int i = 0;
    int total_passengers = 0;
    int no_of_passengers = 0; 
    int curr = 0;
    while (1)
    {
        no_of_passengers = distribution(generator);
        total_passengers += no_of_passengers;
        //cout<<no_of_passengers<<"----"<<endl;
        pthread_t passengers[no_of_passengers];
        for(i = 0; i<no_of_passengers ; i++){
            is_vip = get_bernouli_status();
            pair<int , bool>*p = new pair<int , bool>(i+curr+1 , is_vip);
            pthread_create(&passengers[i],NULL,passengerActivity,(void *)p);
        }
        curr+=no_of_passengers;
        sleep(3);
    }
    
}
int main(){
    read_file();
    init();
    pthread_t t;
    pthread_create(&t , NULL , time_thread , NULL);
    
    pthread_t poisson;
    double poisson_mean = 10.7;
    double *lambda;
    lambda = &poisson_mean;
    pthread_create(&poisson , NULL , create_passengers , (void *)lambda);
    while(1);
    return 0;
}
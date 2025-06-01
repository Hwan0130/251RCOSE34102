#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define name_size 80  //이름 길이
#define time_q 5      //타임퀀텀
#define total_alg 8   //총 알고리즘 개수


int process_num = 0;  //프로세스 개수

//프로세스 구조체
struct proc {
    int pid;
    int arrive;        //도착시간
    int cpu_burst;     //cpu 버스트
    int io_burst;      //io 버스트  
    int prior;         //우선순위
    int io_start;      //io 시작점
    int cpu_left;      //남은 cpu시간
    int io_left;       //남은 io시간
    int wait;          //대기시간
    int turnaround;    //반환시간
};
struct proc** proc_list = NULL;   //프로세스 리스트

//결과 저장용 구조체
struct result {
	double wait_avg;
	double turn_avg;
    char alg_name[name_size];
};
struct result* results[total_alg] = {NULL};

int current_alg = 0;  //현재 알고리즘 번호

//도착시간으로 정렬하는 함수
void sort_by_arrive_time(struct proc** procs, int n){
    //도착시간 빠른순으로, 같으면 pid 작은순
    int i, j;
    for(i = 0; i < n-1; i++) {
        int small = i;
        for(j = i+1; j < n; j++) {
            if(procs[j]->arrive < procs[small]->arrive) {
                small = j;
            }
            else if(procs[j]->arrive == procs[small]->arrive && procs[j]->pid < procs[small]->pid) {
                small = j;
            }
        }
        //바꾸기
        if(small != i) {
            struct proc* tmp = procs[i];
            procs[i] = procs[small];
            procs[small] = tmp;
        }
    }
}

void sort_by_cpu_time(struct proc** procs, int n){
    //cpu 시간 적은순으로 정렬
    int i, j;
    for(i = 0; i < n-1; i++) {
        int small = i;
        for(j = i+1; j < n; j++) {
            if(procs[j]->cpu_left < procs[small]->cpu_left) {
                small = j;
            } else if(procs[j]->cpu_left == procs[small]->cpu_left) {
                if(procs[j]->arrive < procs[small]->arrive) {
                    small = j;
                } else if(procs[j]->arrive == procs[small]->arrive && procs[j]->pid < procs[small]->pid) {
                    small = j;
                }
            }
        }
        //교환
        if(small != i) {
            struct proc* temp = procs[i];
            procs[i] = procs[small];
            procs[small] = temp;
        }
    }
}

void sort_by_priority(struct proc** procs, int n){
    // 우선순위로 정렬(숫자 작을수록 높은 우선순위)
    int i, j;
    for(i = 0; i < n-1; i++) {
        int min_idx = i;
        for(j = i+1; j < n; j++) {
            if(procs[j]->prior < procs[min_idx]->prior) {
                min_idx = j;
            }
            else if(procs[j]->prior == procs[min_idx]->prior) {
                if(procs[j]->arrive < procs[min_idx]->arrive) {
                    min_idx = j;
                }
                else if(procs[j]->arrive == procs[min_idx]->arrive && procs[j]->pid < procs[min_idx]->pid) {
                    min_idx = j;
                }
            }
        }
        // swap
        if(min_idx != i) {
            struct proc* temp = procs[i];
            procs[i] = procs[min_idx];
            procs[min_idx] = temp;
        }
    }
}

void sort_by_io(struct proc** procs, int n){
    //io시간 큰 순서로 정렬, 같으면 cpu시간 작은순, 그것도 같으면 도착시간, pid 순
    int i, j;
    for(i = 0; i < n-1; i++) {
        int big = i; //큰거 찾는거니까 big으로
        for(j = i+1; j < n; j++) {
            if(procs[j]->io_left > procs[big]->io_left) {
                big = j;
            }
            else if(procs[j]->io_left == procs[big]->io_left) {
                //io 같으면 cpu 작은순
                if(procs[j]->cpu_left < procs[big]->cpu_left) {
                    big = j;
                }
                else if(procs[j]->cpu_left == procs[big]->cpu_left) {
                    //cpu도 같으면 도착시간
                    if(procs[j]->arrive < procs[big]->arrive) {
                        big = j;
                    }
                    else if(procs[j]->arrive == procs[big]->arrive && procs[j]->pid < procs[big]->pid) {
                        big = j;
                    }
                }
            }
        }
        //바꾸기
        if(big != i) {
            struct proc* temp = procs[i];
            procs[i] = procs[big];
            procs[big] = temp;
        }
    }
}

//프로세스들 만드는 함수
void make_processes(){
    proc_list = malloc(process_num * sizeof(struct proc*));
    
    srand(time(NULL)); //랜덤 시드
    int i;
    for(i = 0; i < process_num; i++) {
        proc_list[i] = malloc(sizeof(struct proc));

        proc_list[i]->pid = i+1; //프로세스 아이디는 1번부터. 보기 편하게
        proc_list[i]->arrive = rand() % 11; //도착시간 0~10
        proc_list[i]->cpu_burst = (rand() % 16) + 5; //cpu버스트 5~20
        proc_list[i]->io_burst = rand() % 6; //io버스트 0~5
        
        //io가 0이면 io작업 없음
        if(proc_list[i]->io_burst == 0){
            proc_list[i]->io_start = -1;
        } else {
            proc_list[i]->io_start = (rand() % (proc_list[i]->cpu_burst - 1)) + 1; // cpu burst가 10이면 1~9 중 랜덤
        }
        
        proc_list[i]->prior = (rand() % process_num) + 1; //우선순위 1~n, 작을수록 우선순위 높음
        proc_list[i]->cpu_left = proc_list[i]->cpu_burst; //처음엔 전체 시간이 남은시간
        proc_list[i]->io_left = proc_list[i]->io_burst;
        proc_list[i]->wait = 0; //처음엔 대기시간 0
        proc_list[i]->turnaround = 0; //반환시간도 0
    }
    
    sort_by_arrive_time(proc_list, process_num); //도착시간으로 정렬

    //만든 프로세스들 출력해보기
    printf("Process List:\n");
    for(i = 0; i < process_num; i++) {
        printf("Pid%d: arrive=%d, cpu_burst=%d, io_burst=%d, priority=%d, io_start=%d\n",
               proc_list[i]->pid, proc_list[i]->arrive, proc_list[i]->cpu_burst, 
               proc_list[i]->io_burst, proc_list[i]->prior, proc_list[i]->io_start);
    }
    printf("\n");
    
    //결과 저장할 배열들 초기화
    for(i = 0; i < total_alg; i++) {
        results[i] = malloc(sizeof(struct result));
    }
}

//결과 계산하고 출력하는 함수
void calculate_result(struct proc** p, char* alg_name){
    printf("\n");

    //각 프로세스별 결과 출력
    int i;
    for(i = 0; i < process_num; i++) {
        printf("[pid: %d, wait_time: %d, turnaround_time: %d]\n",
               p[i]->pid, p[i]->wait, p[i]->turnaround);
    }
    printf("\n");

    double wait_sum = 0;
	double turn_sum = 0;

    //평균 계산하기
    for(i = 0; i < process_num; i++){
        wait_sum += (double)p[i]->wait;
        turn_sum += (double)p[i]->turnaround;
    }

    wait_sum = wait_sum / (double)process_num;
    turn_sum = turn_sum / (double)process_num;

    //결과 저장
    results[current_alg]->wait_avg = wait_sum;
    results[current_alg]->turn_avg = turn_sum;
    strcpy(results[current_alg]->alg_name, alg_name); //알고리즘 이름 복사

    current_alg++; //다음 알고리즘을 위해 증가

    printf("평균 대기 시간: %.2f\n", wait_sum);
	printf("평균 반환 시간: %.2f\n", turn_sum);
    printf("===============================================================\n");
}

void compare_all_algorithms() {
    printf("<평균 대기 시간 순위>\n");

    // 대기 시간 기준
    int i, j;
    for (i = 0; i < total_alg - 1; i++) {
        int min_idx = i;
        for (j = i + 1; j < total_alg; j++) {
            if (results[j]->wait_avg < results[min_idx]->wait_avg) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            struct result* temp = results[i];
            results[i] = results[min_idx];
            results[min_idx] = temp;
        }
    }

    for (i = 0; i < total_alg; i++) {
        printf("%d등. %s: %.2f\n", i + 1, results[i]->alg_name, results[i]->wait_avg);
    }

    printf("\n<평균 반환 시간 순위>\n");

    // 반환 시간 기준
    for (i = 0; i < total_alg - 1; i++) {
        int min_idx = i;
        for (j = i + 1; j < total_alg; j++) {
            if (results[j]->turn_avg < results[min_idx]->turn_avg) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            struct result* temp = results[i];
            results[i] = results[min_idx];
            results[min_idx] = temp;
        }
    }

    for (i = 0; i < total_alg; i++) {
        printf("%d등. %s: %.2f\n", i + 1, results[i]->alg_name, results[i]->turn_avg);
    }
}


//프로세스 리스트 복사하는 함수. 알고리즘 별로 동일한 프로세스로 실험하기 위해.
struct proc** copy_proc_list(){ 
    struct proc** new_list = malloc(sizeof(struct proc*) * process_num);
    
    int i;
    for(i = 0; i < process_num; i++) {
        new_list[i] = malloc(sizeof(struct proc));

        //원본에서 복사해오기
        new_list[i]->pid = proc_list[i]->pid;
        new_list[i]->arrive = proc_list[i]->arrive;
        new_list[i]->cpu_burst = proc_list[i]->cpu_burst;
        new_list[i]->io_burst = proc_list[i]->io_burst;
        new_list[i]->prior = proc_list[i]->prior;
        new_list[i]->cpu_left = proc_list[i]->cpu_left;
        new_list[i]->io_left = proc_list[i]->io_left;
        new_list[i]->wait = proc_list[i]->wait;
        new_list[i]->turnaround = proc_list[i]->turnaround;
        new_list[i]->io_start = proc_list[i]->io_start;
    }

    return new_list;
}


void fcfs_algorithm(){
    char alg_name[name_size] = "First Come First Served Algorithm";
    printf("<%s>\n", alg_name);
    
    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); //프로세스 리스트 복사

    //변수들 초기화
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    struct proc* running = NULL;

    while(done < process_num) {

        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        sort_by_arrive_time(ready_q, ready_cnt); //도착시간으로 정렬
        
        if(running != NULL) { //실행중인 프로세스가 있으면

            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) { //레디큐에서 새 프로세스 가져오기

            running = ready_q[0]; 
            //레디큐에서 맨 앞 제거
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;

            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else { //아무것도 없으면 idle
            printf("[CPU idle], ");
        }
        
        //실행 끝났는지 확인
        if(running != NULL){
            if(running->cpu_left <= 0) { 
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에 있는 프로세스들은 대기시간 증가
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //웨이팅큐 업데이트 (IO 처리)
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) { //IO 끝남
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    //레디큐로 이동
                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    //웨이팅큐에서 제거
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--;
                }
            }
        }
        
        //IO 작업 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left; //지금까지 한 cpu 작업
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }
    
    calculate_result(p, alg_name); //결과 계산
    
    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}


//비선점 SJF 알고리즘
void non_preemptive_sjf(){
    char alg_name[name_size] = "Non-preemptive Shortest Job First Algorithm";
    printf("<%s>\n", alg_name);

    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); //프로세스 복사

    //변수들
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    struct proc* running = NULL;

    while(done < process_num) {

        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        sort_by_cpu_time(ready_q, ready_cnt); //cpu시간으로 정렬
        
        if(running != NULL) { //실행중인게 있으면
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) { //없으면 레디큐에서 가져오기
            running = ready_q[0]; 
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else { 
            printf("[CPU idle], ");
        }
        
        if(running != NULL){
            if(running->cpu_left <= 0) { //끝났으면
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에서 기다리는 프로세스들
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //IO 큐 처리
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) { //IO 끝남
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--; 
                }
            }
        }

        //IO 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left;
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }

    calculate_result(p, alg_name);

    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}


//선점 SJF 알고리즘
void preemptive_sjf(){
    char alg_name[name_size] = "Preemptive Shortest Job First Algorithm";
    printf("<%s>\n", alg_name);
    
    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); 

    //변수들
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    struct proc* running = NULL;

    while(done < process_num) {

        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        sort_by_cpu_time(ready_q, ready_cnt); //cpu시간으로 정렬
        
        // preemption
        if(running != NULL) { //실행중인게 있으면
            if(ready_cnt > 0){ //선점 확인
                if(running->cpu_left > ready_q[0]->cpu_left){ //더 짧은게 왔으면 바꾸기
                    ready_q[ready_cnt] = running; //현재거를 레디큐에 넣고
                    ready_cnt++;
                    running = ready_q[0]; //새거 가져오기
                    int j;
                    for(j = 1; j < ready_cnt; j++) { 
                        ready_q[j - 1] = ready_q[j];
                    }
                    ready_cnt--;
                }
            }
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) { //레디큐에서 가져오기
            running = ready_q[0]; 
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;

            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else { 
            printf("[CPU idle], ");
        }
        
        if(running != NULL){
            if(running->cpu_left <= 0) { //끝났으면
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에서 기다리는 프로세스들
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //IO 큐 처리
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) { //IO 끝남
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--; 
                }
            }
        }
        
        //IO 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left;
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }

    calculate_result(p, alg_name);

    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}


//비선점 우선순위 알고리즘
void non_preemptive_priority(){
    char alg_name[name_size] = "Non-preemptive Priority Algorithm";
    printf("<%s>\n", alg_name);
  
    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); 

    //변수들
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    struct proc* running = NULL;

    while(done < process_num) {

        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        sort_by_priority(ready_q, ready_cnt); //우선순위로 정렬
        
        if(running != NULL) { //실행중인게 있으면

            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) { //레디큐에서 가져오기

            running = ready_q[0]; 
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else { 
            printf("[CPU idle], ");
        }
        
        if(running != NULL){
            if(running->cpu_left <= 0) { //끝났으면
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에서 기다리는 프로세스들
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //IO 큐 처리
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) { //IO 끝남
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--; 
                }
            }
        }
        
        //IO 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left;
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }

    calculate_result(p, alg_name);

    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}


//선점 우선순위 알고리즘
void preemptive_priority(){
    char alg_name[name_size] = "Preemptive Priority Algorithm";
    printf("<%s>\n", alg_name);
    
    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); 

    //변수들
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    struct proc* running = NULL;
    
    while(done < process_num) {

        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        sort_by_priority(ready_q, ready_cnt); //우선순위로 정렬
        
        if(running != NULL) { //실행중인게 있으면
            if(ready_cnt > 0){ //선점 확인
                if(running->prior > ready_q[0]->prior){ //더 높은 우선순위가 왔으면 바꾸기
                    ready_q[ready_cnt] = running; //현재거를 레디큐에 넣고
                    ready_cnt++;
                    running = ready_q[0]; //새거 가져오기
                    int j;
                    for(j = 1; j < ready_cnt; j++) { 
                        ready_q[j - 1] = ready_q[j];
                    }
                    ready_cnt--;
                }
            }
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) { //레디큐에서 가져오기

            running = ready_q[0]; 
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else { 
            printf("[CPU idle], ");
        }
        
        if(running != NULL){
            if(running->cpu_left <= 0) { //끝났으면
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에서 기다리는 프로세스들
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //IO 큐 처리
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) { //IO 끝남
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--; 
                }
            }
        }

        //IO 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left;
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }

    calculate_result(p, alg_name);

    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}


//라운드 로빈 알고리즘
void round_robin(){
    char alg_name[name_size] = "Round Robin Algorithm";
    printf("<%s (time quantum: %d)>\n", alg_name, time_q);
    
    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); 

    //변수들
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    int time_slice = 0; //현재 프로세스가 사용한 시간
    struct proc* running = NULL;

    while(done < process_num) {
        
        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        
        if(running != NULL) { //실행중인게 있으면           
            if(ready_cnt > 0){ 
                if(time_slice >= time_q){ //타임 퀀텀 다 썼으면 바꾸기
                    ready_q[ready_cnt] = running; //현재거를 레디큐 뒤에 넣고
                    ready_cnt++;
                    running = ready_q[0]; //맨 앞거 가져오기
                    int j;
                    for(j = 1; j < ready_cnt; j++) { 
                        ready_q[j - 1] = ready_q[j];
                    }
                    ready_cnt--;
                    time_slice = 0; //시간 다시 0으로
                }
            }

            running->cpu_left--;
            running->turnaround++;
            time_slice++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) { //레디큐에서 가져오기
            running = ready_q[0]; 
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;
            time_slice = 0;

            running->cpu_left--;
            running->turnaround++;
            time_slice++;
            printf("P%d [running], ", running->pid);

        } else { 
            printf("[CPU idle], ");
        }
        
        if(running != NULL){
            if(running->cpu_left <= 0) { //끝났으면
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에서 기다리는 프로세스들
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //IO 큐 처리
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) { //IO 끝남
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--; 
                }
            }
        }

        //IO 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left;
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }

    calculate_result(p, alg_name);

    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}

//비선점 긴 IO 짧은 작업 우선 알고리즘
void non_preemptive_liosjf(){
    char alg_name[name_size] = "Non-preemptive Longest I/O Shortest Job First Algorithm";
    printf("<%s>\n", alg_name);
  
    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); 

    //변수들
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    struct proc* running = NULL;

    while(done < process_num) {

        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        sort_by_io(ready_q, ready_cnt); //IO시간으로 정렬
        
        if(running != NULL) { //실행중인게 있으면
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) { //레디큐에서 가져오기
            running = ready_q[0]; 
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else { 
            printf("[CPU idle], ");
        }
        
        if(running != NULL){
            if(running->cpu_left <= 0) { //끝났으면
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에서 기다리는 프로세스들
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //IO 큐 처리
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) { //IO 끝남
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--; 
                }
            }
        }

        //IO 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left;
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }

    calculate_result(p, alg_name);

    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}


//선점 긴 IO 짧은 작업 우선 알고리즘
void preemptive_liosjf(){
    char alg_name[name_size] = "Preemptive Longest I/O Shortest Job First Algorithm";
    printf("<%s>\n", alg_name);

    //큐들 만들기
    struct proc** ready_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** waiting_q = malloc(sizeof(struct proc*) * process_num);
    struct proc** p = copy_proc_list(); 

    //변수들
    int ready_cnt = 0;
    int waiting_cnt = 0;
    int time = 0;
    int done = 0;
    int check_idx = 0;
    struct proc* running = NULL;

    while(done < process_num) {

        printf("T %d: ", time);
        
        //도착한 프로세스들 레디큐에 넣기
        while(check_idx < process_num && p[check_idx]->arrive <= time) { 
            ready_q[ready_cnt] = p[check_idx];
            ready_cnt++;
            check_idx++;
        }
        sort_by_io(ready_q, ready_cnt); //IO시간으로 정렬
        
        if(running != NULL) { //실행중인게 있으면
            if(ready_cnt > 0){ //선점 확인
                //IO시간 더 길거나, IO시간 같은데 CPU시간 더 짧은게 있으면 바꾸기
                if((running->io_left < ready_q[0]->io_left) || 
                   (running->io_left == ready_q[0]->io_left && running->cpu_left > ready_q[0]->cpu_left)){ 
                    ready_q[ready_cnt] = running;
                    ready_cnt++;
                    running = ready_q[0];
                    int j;
                    for(j = 1; j < ready_cnt; j++) { 
                        ready_q[j - 1] = ready_q[j];
                    }
                    ready_cnt--;
                }
            }
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else if(ready_cnt > 0) {
            running = ready_q[0]; 
            int j;
            for(j = 1; j < ready_cnt; j++) { 
                ready_q[j - 1] = ready_q[j];
            }
            ready_cnt--;
            running->cpu_left--;
            running->turnaround++;
            printf("P%d [running], ", running->pid);

        } else {
            printf("[CPU idle], ");
        }
        
        if(running != NULL){
            if(running->cpu_left <= 0) {
                printf("P%d [terminated], ", running->pid);
                running = NULL;
                done++;
            }
        }
        
        //레디큐에서 기다리는 프로세스들
        int i;
        for(i = 0; i < ready_cnt; i++) { 
            ready_q[i]->wait++;
            ready_q[i]->turnaround++;
        }
        
        //IO 큐 처리
        if(waiting_cnt > 0) {
            for(i = 0; i < waiting_cnt; i++) {
                waiting_q[i]->turnaround++;
                waiting_q[i]->io_left--;
                if(waiting_q[i]->io_left <= 0) {
                    printf("P%d [I/O complete], ", waiting_q[i]->pid);

                    ready_q[ready_cnt] = waiting_q[i];
                    ready_cnt++;
                    int j;
                    for(j = i + 1; j < waiting_cnt; j++) { 
                        waiting_q[j - 1] = waiting_q[j];
                    }
                    waiting_cnt--;
                    i--;
                }
            }
        }

        //IO 시작하는지 확인
        if(running != NULL){
            if(running->io_left > 0){
                int cpu_done = running->cpu_burst - running->cpu_left;
                if(cpu_done == running->io_start){ 
                    printf("P%d [I/O request], ", running->pid);
                    waiting_q[waiting_cnt] = running;
                    waiting_cnt++;
                    running = NULL;
                }
            } 
        }
        
        time++;
        printf("\n");
    }

    calculate_result(p, alg_name);

    //메모리 정리
    free(waiting_q);
    int i;
    for(i = 0; i < process_num; i++) {
        free(p[i]);
    }
    free(p);
    free(ready_q);
}


//메인 함수
int main(int argc, char **argv){
    process_num = atoi(argv[1]); //프로세스 개수 입력받기
    make_processes(); //프로세스들 생성

    //알고리즘들 실행하기
    fcfs_algorithm();
    non_preemptive_sjf();
    preemptive_sjf();
    non_preemptive_priority();
    preemptive_priority();
    round_robin();

    //추가 알고리즘들
    non_preemptive_liosjf(); 
    preemptive_liosjf();

    //전체 결과 비교
    compare_all_algorithms(); 

    return 0;
}
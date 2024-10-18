#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#define NUM_PROCESSES 4

typedef struct Process {
    int id;
    int arrive_time;
    int burst_time;
    int remaining_time;
    struct Process* link;
} Process;

typedef struct Queue {
    Process* front;
    Process* rear;
} Queue;

void initQueue(Queue* q) {
    q->front = q->rear = NULL;
}

Queue* createQueue(void) {
    Queue* Q = (Queue*)malloc(sizeof(Queue));
    Q->rear = NULL;
    Q->front = NULL;
    return Q;
}

int isEmpty(Queue* Q) {
    return Q->front == NULL;
}

void insert(Queue* q, int id) {
    Process* temp = (Process*)malloc(sizeof(Process));
    temp->id = id;
    temp->arrive_time = id;
    temp->burst_time = rand() % 10 + 1;
    temp->remaining_time = temp->burst_time;
    temp->link = NULL;

    if (isEmpty(q)) {
        q->front = q->rear = temp;
    }
    else {
        q->rear->link = temp;
        q->rear = temp;
    }
}

void insertQ(Queue* q, Process* p) {
    if (isEmpty(q)) {
        q->front = q->rear = p;
    }
    else {
        q->rear->link = p;
        q->rear = p;
    }
    p->link = NULL;
}

void printQueue(Queue* q) {
    printf("Queue state: [ ");
    Process* curr = q->front;  // 구의 첫 번째 노드부터 시작
    // 구의 모든 노드를 순차적으로 탐색하여 출력
    while (curr != NULL) {
        printf("{ID: %d, arrive: %d, burst: %d} ", curr->id, curr->arrive_time, curr->burst_time);  // 프로세스 정보 출력
        curr = curr->link;  // 다음 노드로 이동
    }
    printf("]\n");
}

void SJF(Queue* processQ) {
    int time = 0;
    int cnt = 0;
    int total_waiting_time = 0;
    Queue ReadyQ;
    Process* current = processQ->front;

    initQueue(&ReadyQ);

    while (cnt < NUM_PROCESSES) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        while (current != NULL && current->arrive_time <= time) {
            Process* next = current->link;
            insertQ(&ReadyQ, current);
            current = next;
        }

        Process* cur = NULL;
        int min_time = 10000;

        // Ready Queue에서 남은 시간이 가장 짝은 프로세스를 선택
        Process* temp = ReadyQ.front;
        while (temp != NULL) {
            if (temp->remaining_time < min_time && temp->remaining_time > 0) {
                min_time = temp->remaining_time;
                cur = temp;
            }
            temp = temp->link;
        }

        if (cur == NULL) {
            time++;
            continue;
        }

        // 선택한 프로세스를 실행 (선제면)
        printf("time: %d, process_id: %d, remaining burst time: %d\n", time, cur->id, cur->remaining_time);
        cur->remaining_time--;
        time++;

        // 프로세스가 완료된 경우 처리
        if (cur->remaining_time == 0) {
            int waiting_time = time - cur->arrive_time - cur->burst_time;
            total_waiting_time += waiting_time;
            cnt++;
            printf("Process_id : %d completed time: %d\n", cur->id, time);

            // Ready Queue에서 간단히 완료된 프로세스 제거
            if (ReadyQ.front == cur) {
                ReadyQ.front = cur->link;
                if (ReadyQ.front == NULL) {
                    ReadyQ.rear = NULL;
                }
            }
            else {
                Process* prev = ReadyQ.front;
                while (prev->link != cur) {
                    prev = prev->link;
                }
                prev->link = cur->link;
                if (cur == ReadyQ.rear) {
                    ReadyQ.rear = prev;
                }
            }
        }
    }
    printf("Average time: %.2f\n", (float)total_waiting_time / NUM_PROCESSES);
}

void RoundRobin(Queue* processQ, int time_quantum) {
    int time = 0;
    int cnt = 0;
    int total_waiting_time = 0;
    Queue ReadyQ;
    Process* current = processQ->front;

    initQueue(&ReadyQ);

    while (cnt < NUM_PROCESSES) {
        // 현재 시간에 도착한 프로세스를 Ready Queue에 추가
        while (current != NULL && current->arrive_time <= time) {
            Process* next = current->link;
            insertQ(&ReadyQ, current);
            current = next;
        }

        if (isEmpty(&ReadyQ)) {
            time++;
            continue;
        }

        Process* cur = ReadyQ.front;

        // 선택한 프로세스를 실행 (시간 할당량만큼)
        int execution_time = (cur->remaining_time < time_quantum) ? cur->remaining_time : time_quantum;
        printf("Time: %d, Process_id : %d, :  execution_time: %d\n", time, cur->id, execution_time);
        cur->remaining_time -= execution_time;
        time += execution_time;

        // 프로세스가 완료된 경우 처리
        if (cur->remaining_time == 0) {
            int waiting_time = time - cur->arrive_time - cur->burst_time;
            total_waiting_time += waiting_time;
            cnt++;
            printf("Process_id : %d completed time: %d\n", cur->id, time);

            // Ready Queue에서 완료된 프로세스 제거
            ReadyQ.front = cur->link;
            if (ReadyQ.front == NULL) {
                ReadyQ.rear = NULL;
            }
            free(cur);
        }
        else {
            // 프로세스를 Ready Queue의 끝으로 이동
            ReadyQ.front = cur->link;
            if (ReadyQ.front == NULL) {
                ReadyQ.rear = NULL;
            }
            insertQ(&ReadyQ, cur);
        }
    }
    printf("Average time : %.2f\n", time_quantum, (float)total_waiting_time / NUM_PROCESSES);
}

int main() {
    Queue Q;
    initQueue(&Q);
    srand(time(NULL));

    for (int i = 0; i < NUM_PROCESSES; i++) {
        insert(&Q, i);
    }

    printQueue(&Q);
    SJF(&Q);
    printf("\n-----\n");

    // Round Robin = 4
    Queue Q1;
    initQueue(&Q1);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        insert(&Q1, i);
    }
    RoundRobin(&Q1, 4);

    printf("\n-----\n");

    // Round Robin= 8
    Queue Q2;
    initQueue(&Q2);
    for (int i = 0; i < NUM_PROCESSES; i++) {
        insert(&Q2, i);
    }
    RoundRobin(&Q2, 8);

    getchar();
    return 0;
}

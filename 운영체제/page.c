#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define MAIN_MEMORY_SIZE 1024  // 메인 메모리 크기 (1KB)
#define PAGE_SIZE 16           // 페이지(프레임) 크기 (16바이트)
#define MAX_PAGES (MAIN_MEMORY_SIZE / PAGE_SIZE) // 최대 페이지 수
#define MAX_PROCESSES 21       // 총 처리할 프로세스 수

// 프로세스 구조체 정의
typedef struct {
    int process_id;          // 프로세스 ID
    int size;                // 프로세스 크기
    int num_pages;           // 필요한 페이지 수
    int* page_table;         // 페이지 테이블 (프레임 번호)
} Process;

// 메모리 상태를 나타내는 배열
bool memory[MAX_PAGES] = { false };

// 프로세스 리스트
Process* process_list[MAX_PROCESSES] = { NULL };
int process_count = 0; // 현재 실행 중인 프로세스 수

// 새로운 프로세스를 생성
void create_process(int process_id) {
    int size = (rand() % (100 - 20 + 1)) + 20; // 20 <= size <= 100
    int num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; // 필요한 페이지 수 계산

    // 프로세스를 메모리에 배치
    int* page_table = (int*)malloc(num_pages * sizeof(int));
    int allocated_pages = 0;

    for (int i = 0; i < MAX_PAGES && allocated_pages < num_pages; i++) {
        if (!memory[i]) {
            memory[i] = true;
            page_table[allocated_pages++] = i; // 페이지 테이블에 프레임 번호 기록
        }
    }

    if (allocated_pages < num_pages) {
        printf("프로세스 %d: 메모리가 부족하여 생성 실패\n", process_id);
        free(page_table);
        return;
    }

    // 프로세스 생성 및 리스트에 추가
    Process* new_process = (Process*)malloc(sizeof(Process));
    new_process->process_id = process_id;
    new_process->size = size;
    new_process->num_pages = num_pages;
    new_process->page_table = page_table;

    process_list[process_count++] = new_process;

    // 프로세스 생성 정보 출력
    printf("프로세스 %d 생성 (크기: %d, 페이지 수: %d)\n", process_id, size, num_pages);
    printf("페이지 테이블: ");
    for (int i = 0; i < num_pages; i++) {
        printf("%d ", page_table[i]);
    }
    printf("\n");
}

// 프로세스를 종료
void terminate_process(int process_id) {
    for (int i = 0; i < process_count; i++) {
        if (process_list[i]->process_id == process_id) {
            Process* p = process_list[i];

            // 메모리 해제
            for (int j = 0; j < p->num_pages; j++) {
                memory[p->page_table[j]] = false;
            }

            // 프로세스 리스트에서 제거
            free(p->page_table);
            free(p);

            // 리스트에서 프로세스 제거 및 정리
            for (int k = i; k < process_count - 1; k++) {
                process_list[k] = process_list[k + 1];
            }
            process_list[--process_count] = NULL;

            printf("프로세스 %d 종료\n", process_id);
            return;
        }
    }
    printf("프로세스 %d: 존재하지 않음\n", process_id);
}

// 메모리 상태 출력
// 메모리 상태 출력
void print_memory_state() {
    printf("메모리 상태:\n");
    for (int i = 0; i < MAX_PAGES; i++) {
        printf("[%2d: %s] ", i, memory[i] ? "사용 중" : "비어 있음");
        if ((i + 1) % 8 == 0) { // 한 줄에 8개씩 출력
            printf("\n");
        }
    }
    printf("\n");
}

int main() {
    srand(time(NULL)); // 난수 초기화

    int current_process_id = 1;

    // 프로세스 생성 및 종료 순서
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (i % 5 == 0 && process_count > 0) {
            // 5번째마다 프로세스 종료
            terminate_process(process_list[0]->process_id);
        }

        // 새 프로세스 생성
        create_process(current_process_id++);
        print_memory_state();
        printf("\n");
    }

    // 남은 모든 프로세스 종료
    while (process_count > 0) {
        terminate_process(process_list[0]->process_id);
        print_memory_state();
    }

    printf("모든 프로세스 종료 완료.\n");

    getchar();
    return 0;
}

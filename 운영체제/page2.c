#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define MAX_PROCESSES 5
#define MAX_PAGES 10
#define MEM_FRAMES 13 // 메인 메모리 내 프레임 개수

// 페이지 테이블 엔트리
typedef struct {
    int page_number;
    bool valid_bit;
    int frame_number;
} PageTableEntry;

// 프로세스 구조체
typedef struct {
    int pid;
    int total_pages;
    int reference_string[20];
    PageTableEntry page_table[MAX_PAGES];
    int page_faults;
    int accesses;
} Process;

// 메모리 프레임 테이블
typedef struct {
    int pid; // 어떤 프로세스의 페이지인지
    int page_number;
} Frame;

Frame memory[MEM_FRAMES]; // 메인 메모리 프레임
Process processes[MAX_PROCESSES]; // 프로세스 배열
int  clock_pointer;


void log_event(const char* event_type, int pid, int page, int victim_frame, int victim_page) {
    printf("\n[Event: %s] Process %d\n", event_type, pid);
    printf("Requested Page: %d\n", page);

    // 희생 페이지가 존재할 때만 출력
    if (victim_frame != -1) {
        printf("Victim Frame: %d, Victim Page: %d\n", victim_frame, victim_page);
    }
    printf("\n");
}

void log_event_no_fault(int pid, int page, int frame) {
    printf("[Event: No Page Fault] Process %d, Page %d is already in Frame %d\n", pid, page, frame);
}


// 프로세스 초기 정보 출력
void print_process_info() {
    printf("\n--- Process Information ---\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        printf("Process %d (Total Pages: %d):\n", processes[i].pid, processes[i].total_pages);
        printf("Reference String: ");
        for (int j = 0; j < 20; j++) {
            printf("%d ", processes[i].reference_string[j]);
        }
        printf("\n");
    }
    printf("\n");
}


// 초기화 함수
void initialize() {
    srand(time(NULL));
    clock_pointer = 0; // FIFO 포인터 초기화
    // 메모리 초기화
    for (int i = 0; i < MEM_FRAMES; i++) {
        memory[i].pid = -1;
        memory[i].page_number = -1;
    }

    // 프로세스 초기화
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = i;
        processes[i].total_pages = 6 + rand() % 5; // 6~10개 페이지 요구
        processes[i].page_faults = 0;
        processes[i].accesses = 0;

        // 참조열 생성
        for (int j = 0; j < 20; j++) {
            processes[i].reference_string[j] = rand() % processes[i].total_pages;
        }

        // 페이지 테이블 초기화
        for (int j = 0; j < processes[i].total_pages; j++) {
            processes[i].page_table[j].page_number = j;
            processes[i].page_table[j].valid_bit = false;
            processes[i].page_table[j].frame_number = -1;
        }

        // 30%의 페이지를 미리 메모리에 적재
        int preload_count = processes[i].total_pages * 0.3;
        for (int j = 0; j < preload_count; j++) {
            int page_to_load = rand() % processes[i].total_pages;
            if (!processes[i].page_table[page_to_load].valid_bit) {
                memory[clock_pointer].pid = i;
                memory[clock_pointer].page_number = page_to_load;
                processes[i].page_table[page_to_load].valid_bit = true;
                processes[i].page_table[page_to_load].frame_number = clock_pointer;
                clock_pointer = (clock_pointer + 1) % MEM_FRAMES;
            }
        }
    }
}

// 페이지 테이블 출력 함수
void print_page_table(Process* p) {
    printf("\nProcess %d Page Table:\n", p->pid);
    printf("Page\tValid\tFrame\n");
    for (int i = 0; i < p->total_pages; i++) {
        printf("%d\t%s\t%d\n", i, p->page_table[i].valid_bit ? "Yes" : "No", p->page_table[i].frame_number);
    }
}

// FIFO 페이지 교체 알고리
void fifo_replace(Process* p, int page) {
    // 빈 프레임 우선 확인
    for (int i = 0; i < MEM_FRAMES; i++) {
        if (memory[i].pid == -1) {
            memory[i].pid = p->pid;
            memory[i].page_number = page;
            p->page_table[page].valid_bit = true;
            p->page_table[page].frame_number = i;

            log_event("Page Allocated (FIFO)", p->pid, page, -1, -1);
            return;
        }
    }

    // FIFO 교체 수행
    int victim_frame = clock_pointer;
    int victim_pid = memory[victim_frame].pid;
    int victim_page = memory[victim_frame].page_number;

    // 디버깅 출력
    printf("[Debug] FIFO Clock Pointer: %d, Victim Frame: %d, Victim Page: %d (PID: %d)\n",
        clock_pointer, victim_frame, victim_page, victim_pid);

    // 희생 페이지 무효화
    if (victim_pid != -1) {
        processes[victim_pid].page_table[victim_page].valid_bit = false;
        processes[victim_pid].page_table[victim_page].frame_number = -1;
    }

    // 새로운 페이지 적재
    memory[victim_frame].pid = p->pid;
    memory[victim_frame].page_number = page;
    p->page_table[page].valid_bit = true;
    p->page_table[page].frame_number = victim_frame;

    // FIFO 포인터 이동
    clock_pointer = (clock_pointer + 1) % MEM_FRAMES;

    log_event("Page Replaced (FIFO)", p->pid, page, victim_frame, victim_page);}


// 최적 교체 알고리즘
void optimal_replace(Process* p, int page, int current_access) {
    // 빈 프레임 우선 확인
    for (int i = 0; i < MEM_FRAMES; i++) {
        if (memory[i].pid == -1) {
            // 빈 프레임에 페이지 적재
            memory[i].pid = p->pid;
            memory[i].page_number = page;
            p->page_table[page].valid_bit = true;
            p->page_table[page].frame_number = i;

            // 이벤트 로그와 메모리 상태 출력
            log_event("Page Allocated (Optimal)", p->pid, page, -1, -1);
            return; // 빈 프레임을 사용했으므로 교체 종료
        }
    }

    // 빈 프레임이 없을 경우 최적 교체 수행
    int farthest = -1; // 가장 멀리 참조되는 시점
    int victim_frame = -1; // 교체 대상 프레임 번호

    // 모든 메모리 프레임 확인
    for (int i = 0; i < MEM_FRAMES; i++) {
        int next_use = -1; // 다음 참조 시점
        for (int j = current_access + 1; j < 20; j++) {
            if (memory[i].pid == p->pid && memory[i].page_number == p->reference_string[j]) {
                next_use = j; // 참조 시점 발견
                break;
            }
        }

        // 가장 멀리 참조되거나 참조되지 않을 페이지 선택
        if (next_use == -1) {
            victim_frame = i; // 참조되지 않을 페이지
            break;
        }

        if (next_use > farthest) {
            farthest = next_use;
            victim_frame = i;
        }
    }

    // 희생 프레임이 없는 경우
    if (victim_frame == -1) {
        printf("[Optimal Replace] Error: No victim frame found!\n");
        return; // 교체 불가능한 상황에서 종료
    }

    // 희생 페이지 정보
    int victim_pid = memory[victim_frame].pid;
    int victim_page = memory[victim_frame].page_number;

    // 희생 페이지 무효화
    if (victim_pid != -1) {
        processes[victim_pid].page_table[victim_page].valid_bit = false;
        processes[victim_pid].page_table[victim_page].frame_number = -1;
    }

    // 새로운 페이지 적재
    memory[victim_frame].pid = p->pid;
    memory[victim_frame].page_number = page;
    p->page_table[page].valid_bit = true;
    p->page_table[page].frame_number = victim_frame;

    // 이벤트 로그 출력
    log_event("Page Replaced (Optimal)", p->pid, page, victim_frame, victim_page);

    // 메모리 상태 출력
}

// 프로세스 실행 시뮬레이션 (FIFO)
void simulate_fifo() {
    printf("\nSimulating FIFO Page Replacement\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process* p = &processes[i];
        for (int j = 0; j < 20; j++) {
            int page = p->reference_string[j];
            p->accesses++;

            if (!p->page_table[page].valid_bit) {
                // 페이지 폴트 발생
                p->page_faults++;
                fifo_replace(p, page);
                print_page_table(p);
            }
        }
        printf("Process %d finished with %d page faults.\n", p->pid, p->page_faults);
    }
}

// 프로세스 실행 시뮬레이션 (Optimal)
void simulate_optimal() {
    printf("\nSimulating Optimal Page Replacement\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process* p = &processes[i];
        for (int j = 0; j < 20; j++) {
            int page = p->reference_string[j];
            p->accesses++;

            if (!p->page_table[page].valid_bit) {
                // 페이지 폴트 발생
                p->page_faults++;
                optimal_replace(p, page, j);
                print_page_table(p);
            }
        }
        printf("Process %d finished with %d page faults.\n", p->pid, p->page_faults);
    }
}

void compare_page_faults() {
    int fifo_faults = 0, optimal_faults = 0;

    // FIFO 알고리즘 실행
    initialize(); // 재초기화
    printf("\n--- FIFO Algorithm ---\n");
    simulate_fifo();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        fifo_faults += processes[i].page_faults;
    }

    // Optimal 알고리즘 실행
    initialize(); // 재초기화
    printf("\n--- Optimal Algorithm ---\n");
    simulate_optimal();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        optimal_faults += processes[i].page_faults;
    }

    // 결과 비교 출력
    printf("\n--- Page Fault Comparison ---\n");
    printf("Total FIFO Page Faults: %d\n", fifo_faults);
    printf("Total Optimal Page Faults: %d\n", optimal_faults);
}


int main() {
    initialize();
    print_process_info(); // 랜덤 생성 정보 출력
    compare_page_faults(); // FIFO와 Optimal 알고리즘 비교
    getchar();
    return 0;
}

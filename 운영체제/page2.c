#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PROCESSES 5 // 최대 프로세스의 수
#define MAX_PAGES 10 // 각 프로세스가 최대 10개의 페이지를 가진다

// 페이지 테이블 구조체
typedef struct PageTableNode {
    int page_number; // 페이지 번호
    bool valid_bit; // 유효 비트
    int frame_number; // 페이지가 저장된 메모리 프레임의 번호
    struct PageTableNode* next; // 다음 노드를 가리키는 포인터
} PageTableNode;

// 프로세스 구조체
typedef struct Process {
    int id;
    int total_pages;
    int reference_string[20];
    PageTableNode* page_table;
    int page_faults;
    int accesses;
} Process;

// 메모리 프레임 구조체
typedef struct Frame {
    int id;
    int page_number;
    struct Frame* next;
} Frame;

Frame* memory = NULL; // 메모리 프레임 연결 리스트의 헤드
Process processes[PROCESSES]; // 프로세스 배열
int total_frames = 13; // 메인 메모리 내 프레임 개수

// 페이지 테이블에 노드를 추가하는 함수
PageTableNode* add_page_table_node(PageTableNode* head, int page_number) {
    PageTableNode* new_node = (PageTableNode*)malloc(sizeof(PageTableNode));
    new_node->page_number = page_number;
    new_node->valid_bit = false;
    new_node->frame_number = -1;
    new_node->next = head;
    return new_node;
}

// 메모리에 노드를 추가하는 함수
void add_frame(int id, int page_number) {
    Frame* new_frame = (Frame*)malloc(sizeof(Frame));
    new_frame->id = id;
    new_frame->page_number = page_number;
    new_frame->next = memory;
    memory = new_frame;
}

// 메모리에서 노드를 제거하는 함수
Frame* remove_frame() {
    if (!memory) return NULL;

    Frame* prev = NULL;
    Frame* current = memory;
    while (current->next) {
        prev = current;
        current = current->next;
    }

    if (prev) {
        prev->next = NULL;
    }
    else {
        memory = NULL;
    }

    return current;
}

// 초기화 함수
void initialize() {
    srand(time(NULL));

    // 프로세스 초기화
    for (int i = 0; i < PROCESSES; i++) {
        processes[i].id = i;
        processes[i].total_pages = 6 + rand() % 5; // 6~10개 페이지 요구
        processes[i].page_faults = 0;
        processes[i].accesses = 0;
        processes[i].page_table = NULL;

        // 참조열 생성
        for (int j = 0; j < 20; j++) {
            processes[i].reference_string[j] = rand() % processes[i].total_pages;
        }

        // 페이지 테이블 초기화
        for (int j = 0; j < processes[i].total_pages; j++) {
            processes[i].page_table = add_page_table_node(processes[i].page_table, j);
        }

        // 초기 30% 페이지를 메모리에 적재
        int preload_count = processes[i].total_pages * 0.3;
        for (int j = 0; j < preload_count; j++) {
            int page_to_load = rand() % processes[i].total_pages;
            PageTableNode* node = processes[i].page_table;
            while (node) {
                if (node->page_number == page_to_load && !node->valid_bit) {
                    add_frame(processes[i].id, page_to_load);
                    node->valid_bit = true;
                    node->frame_number = total_frames;
                    total_frames--;
                    break;
                }
                node = node->next;
            }
        }
    }
}

// 페이지 테이블 출력 함수
void print_page_table(Process* p) {
    printf("\n프로세스 %d 페이지 테이블:\n", p->id);
    printf("페이지\tValid\tFrame\n");
    PageTableNode* current = p->page_table;
    while (current) {
        printf("%d\t%s\t%d\n", current->page_number, current->valid_bit ? "Yes" : "No", current->frame_number);
        current = current->next;
    }
}

// 주요 이벤트 로그 출력 함수
void log_event(const char* event, Process* p) {
    printf("\n[Event: %s] Process %d\n", event, p->id);
    print_page_table(p);
}

// FIFO 페이지 교체 알고리즘
void fifo_replace(Process* p, int page) {
    if (!memory || total_frames > 0) {
        // 빈 프레임이 있을 경우
        add_frame(p->id, page);
        total_frames--;
    }
    else {
        // 교체 대상 찾기
        Frame* victim = remove_frame();
        if (victim) {
            Process* victim_process = &processes[victim->id];
            PageTableNode* node = victim_process->page_table;
            while (node) {
                if (node->page_number == victim->page_number) {
                    node->valid_bit = false;
                    node->frame_number = -1;
                    break;
                }
                node = node->next;
            }
            free(victim);
        }
        add_frame(p->id, page);
    }

    // 페이지 테이블 업데이트
    PageTableNode* node = p->page_table;
    while (node) {
        if (node->page_number == page) {
            node->valid_bit = true;
            node->frame_number = total_frames;
            break;
        }
        node = node->next;
    }
}

// 최적 교체 알고리즘
void optimal_replace(Process* p, int page, int current_access) {
    Frame* victim = NULL;
    Frame* prev_victim = NULL;
    Frame* prev = NULL;
    Frame* current = memory;

    int farthest = -1;

    while (current) {
        int next_use = -1;
        for (int j = current_access + 1; j < 20; j++) {
            if (current->id == p->id && current->page_number == p->reference_string[j]) {
                next_use = j;
                break;
            }
        }

        if (next_use == -1) {
            // 앞으로 참조되지 않을 페이지를 찾음
            victim = current;
            prev_victim = prev;
            break;
        }

        if (next_use > farthest) {
            farthest = next_use;
            victim = current;
            prev_victim = prev;
        }

        prev = current;
        current = current->next;
    }

    // 희생 페이지 제거
    if (victim) {
        if (prev_victim) {
            prev_victim->next = victim->next;
        }
        else {
            memory = victim->next;
        }

        Process* victim_process = &processes[victim->id];
        PageTableNode* node = victim_process->page_table;
        while (node) {
            if (node->page_number == victim->page_number) {
                node->valid_bit = false;
                node->frame_number = -1;
                break;
            }
            node = node->next;
        }

        free(victim);
    }

    add_frame(p->id, page);

    // 페이지 테이블 업데이트
    PageTableNode* node = p->page_table;
    while (node) {
        if (node->page_number == page) {
            node->valid_bit = true;
            node->frame_number = total_frames;
            break;
        }
        node = node->next;
    }
}

// 프로세스 정보 출력 함수
void print_process_info() {
    printf("\n--- Process Information ---\n");
    for (int i = 0; i < PROCESSES; i++) {
        printf("Process %d:\n", processes[i].id);
        printf("  Total Pages: %d\n", processes[i].total_pages);
        printf("  Reference String: ");
        for (int j = 0; j < 20; j++) {
            printf("%d ", processes[i].reference_string[j]);
        }
        printf("\n");
    }
    printf("\n");
}

// 프로세스 실행 시뮬레이션 (FIFO)
void simulate_fifo() {
    printf("\nSimulating FIFO Page Replacement\n");
    print_process_info(); // 프로세스 정보 출력
    for (int i = 0; i < PROCESSES; i++) {
        Process* p = &processes[i];
        for (int j = 0; j < 20; j++) {
            int page = p->reference_string[j];
            p->accesses++;

            // 페이지 테이블에서 확인
            PageTableNode* node = p->page_table;
            bool found = false;
            while (node) {
                if (node->page_number == page && node->valid_bit) {
                    found = true;
                    break;
                }
                node = node->next;
            }

            if (!found) {
                // 페이지 폴트 발생
                p->page_faults++;
                log_event("Page Fault", p);
                fifo_replace(p, page);
                log_event("Page Replaced", p);
            }
        }
        printf("Process %d finished with %d page faults (Total Pages: %d).\n",
            p->id, p->page_faults, p->total_pages);
    }
}

// 프로세스 실행 시뮬레이션 (Optimal)
void simulate_optimal() {
    printf("\nSimulating Optimal Page Replacement\n");
    print_process_info(); // 프로세스 정보 출력
    for (int i = 0; i < PROCESSES; i++) {
        Process* p = &processes[i];
        for (int j = 0; j < 20; j++) {
            int page = p->reference_string[j];
            p->accesses++;

            // 페이지 테이블에서 확인
            PageTableNode* node = p->page_table;
            bool found = false;
            while (node) {
                if (node->page_number == page && node->valid_bit) {
                    found = true;
                    break;
                }
                node = node->next;
            }

            if (!found) {
                // 페이지 폴트 발생
                p->page_faults++;
                log_event("Page Fault", p);
                optimal_replace(p, page, j);
                log_event("Page Replaced", p);
            }
        }
        printf("Process %d finished with %d page faults (Total Pages: %d).\n",
            p->id, p->page_faults, p->total_pages);
    }
}

// 두 알고리즘 비교 함수
void compare_algorithms() {
    printf("\n--- Comparison of Algorithms ---\n");

    // FIFO 실행
    printf("\nFIFO Algorithm:\n");
    initialize();
    simulate_fifo();
    int fifo_faults = 0;
    for (int i = 0; i < PROCESSES; i++) {
        fifo_faults += processes[i].page_faults;
    }

    // Optimal 실행
    printf("\nOptimal Algorithm:\n");
    initialize();
    simulate_optimal();
    int optimal_faults = 0;
    for (int i = 0; i < PROCESSES; i++) {
        optimal_faults += processes[i].page_faults;
    }

    printf("\n--- Summary ---\n");
    printf("Total FIFO Page Faults: %d\n", fifo_faults);
    printf("Total Optimal Page Faults: %d\n", optimal_faults);
}


int main() {
    compare_algorithms();
    getchar();
    return 0;
}

#include "scheduler.h"
#include "software_timer.h"

typedef struct {
    void (*pTask)(void);
    uint32_t Delay;
    uint32_t Period;
    uint8_t RunMe;
    uint32_t TaskID;
    int8_t next; // Con trỏ mảng cho danh sách liên kết tĩnh
} sTask;

static sTask SCH_tasks_G[SCH_MAX_TASKS];
static int8_t head_index = -1;
static uint32_t new_task_id = 1;

/* Hàm nội bộ: Chèn 1 node đã có vào danh sách theo thứ tự Delta Delay */
static void SCH_Insert_Node(int8_t idx) {
    SCH_tasks_G[idx].next = -1;
    if (head_index == -1) {
        head_index = idx;
        return;
    }

    int8_t curr = head_index;
    int8_t prev = -1;

    while (curr != -1 && SCH_tasks_G[idx].Delay >= SCH_tasks_G[curr].Delay) {
        SCH_tasks_G[idx].Delay -= SCH_tasks_G[curr].Delay;
        prev = curr;
        curr = SCH_tasks_G[curr].next;
    }

    SCH_tasks_G[idx].next = curr;
    if (prev == -1) {
        head_index = idx;
    } else {
        SCH_tasks_G[prev].next = idx;
    }

    /* Trừ phần Delay của node phía sau để bảo toàn tổng thời gian */
    if (curr != -1) {
        SCH_tasks_G[curr].Delay -= SCH_tasks_G[idx].Delay;
    }
}

void SCH_Init(void) {
    head_index = -1;
    for (int i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask = 0;
        SCH_tasks_G[i].Delay = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe = 0;
        SCH_tasks_G[i].TaskID = NO_TASK_ID;
        SCH_tasks_G[i].next = -1;
    }
}

/* Thêm Task mới (DELAY_MS và PERIOD_MS tính bằng mili-giây, ví dụ 10, 200, 2000) */
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY_MS, uint32_t PERIOD_MS) {
    int8_t free_idx = -1;
    for (int i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].pTask == 0) {
            free_idx = i;
            break;
        }
    }
    if (free_idx == -1) return NO_TASK_ID;

    SCH_tasks_G[free_idx].pTask  = pFunction;
    SCH_tasks_G[free_idx].Delay  = DELAY_MS / TIMER_TICK;
    SCH_tasks_G[free_idx].Period = PERIOD_MS / TIMER_TICK;
    SCH_tasks_G[free_idx].RunMe  = 0;
    SCH_tasks_G[free_idx].TaskID = new_task_id++;
    if (new_task_id == NO_TASK_ID) new_task_id++;

    SCH_Insert_Node(free_idx);
    return SCH_tasks_G[free_idx].TaskID;
}

/* Xóa Task theo TaskID và cộng trả lại Delay cho phần tử đứng sau */
uint8_t SCH_Delete_Task(uint32_t taskID) {
    if (taskID == NO_TASK_ID || head_index == -1) return 0;

    int8_t curr = head_index;
    int8_t prev = -1;

    while (curr != -1 && SCH_tasks_G[curr].TaskID != taskID) {
        prev = curr;
        curr = SCH_tasks_G[curr].next;
    }
    if (curr == -1) return 0;

    int8_t next_node = SCH_tasks_G[curr].next;
    if (next_node != -1) {
        SCH_tasks_G[next_node].Delay += SCH_tasks_G[curr].Delay;
    }

    if (prev == -1) {
        head_index = next_node;
    } else {
        SCH_tasks_G[prev].next = next_node;
    }

    SCH_tasks_G[curr].pTask = 0;
    SCH_tasks_G[curr].Delay = 0;
    SCH_tasks_G[curr].Period = 0;
    SCH_tasks_G[curr].RunMe = 0;
    SCH_tasks_G[curr].TaskID = NO_TASK_ID;
    SCH_tasks_G[curr].next = -1;
    return 1;
}

/* Hàm cập nhật O(1): Chỉ xử lý phần tử đầu tiên (head_index) */
void SCH_Update(void) {
    if (head_index != -1) {
        if (SCH_tasks_G[head_index].Delay > 0) {
            SCH_tasks_G[head_index].Delay--;
        }
        if (SCH_tasks_G[head_index].Delay == 0) {
            SCH_tasks_G[head_index].RunMe = 1;
        }
    }
}

/* Hàm điều phối chạy trong while(1) */
void SCH_Dispatch_Tasks(void) {
    while (head_index != -1 && SCH_tasks_G[head_index].Delay == 0 && SCH_tasks_G[head_index].RunMe > 0) {
        int8_t ready_idx = head_index;
        head_index = SCH_tasks_G[ready_idx].next;

        /* Đánh thức luôn các task có cùng thời điểm (Delay == 0) đứng kế tiếp */
        if (head_index != -1 && SCH_tasks_G[head_index].Delay == 0) {
            SCH_tasks_G[head_index].RunMe = 1;
        }

        SCH_tasks_G[ready_idx].RunMe = 0;
        if (SCH_tasks_G[ready_idx].pTask != 0) {
            (*SCH_tasks_G[ready_idx].pTask)(); // Thực thi hàm task
        }

        /* Nếu là task định kỳ (Period > 0) thì chèn lại vào hàng đợi */
        if (SCH_tasks_G[ready_idx].Period > 0) {
            SCH_tasks_G[ready_idx].Delay = SCH_tasks_G[ready_idx].Period;
            SCH_Insert_Node(ready_idx);
        } else {
            /* One-shot task (Period == 0): giải phóng slot */
            SCH_tasks_G[ready_idx].pTask = 0;
            SCH_tasks_G[ready_idx].TaskID = NO_TASK_ID;
            SCH_tasks_G[ready_idx].next = -1;
        }
    }
}

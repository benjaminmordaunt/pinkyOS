#define MIN(a, b) ((a < b) ? (a) : (b))
#define MAX(a, b) ((a > b) ? (a) : (b))

typedef struct list_head_struct {
    struct list_head_struct *next;
    struct list_head_struct *prev;
} list_head_t;

#define LIST_HEAD_TERM        UINTPTR_MAX
#define LIST_TAIL_TERM        UINTPTR_MAX

#define list_head_init(l) \
            ((list_head_t *)l)->next = LIST_TAIL_TERM; \
            ((list_head_t *)l)->prev = LIST_HEAD_TERM

#define list_head_init_invalid(l) \
            ((list_head_t *)l)->next = 0; \
            ((list_head_t *)l)->prev = 0

#define list_entry_valid(l) \
            (((list_head_t *)l)->next && ((list_head_t *)l)->prev)

#define list_tail(l, ptr) \
            do { \
                for ((ptr) = (l); (ptr)->next != LIST_TAIL_TERM; (ptr) = (ptr)->next) \
                    {} \
            } while (0)

#define list_remove(l) \
            do { \
                if (((list_head_t *)l)->next != LIST_TAIL_TERM) \
                    ((list_head_t *)l)->next->prev = ((list_head_t *)l)->prev; \
                if (((list_head_t *)l)->prev != LIST_HEAD_TERM) \
                    ((list_head_t *)l)->prev->next = ((list_head_t *)l)->next; \
                ((list_head_t *)l)->prev = 0; \
                ((list_head_t *)l)->next = 0; \
            } while (0)

int kstrcmp(const char *p, const char *q);

#define __packed __attribute__((packed))

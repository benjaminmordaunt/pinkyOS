#define MIN(a, b) ((a < b) ? (a) : (b))
#define MAX(a, b) ((a > b) ? (a) : (b))

typedef struct list_head_struct {
    struct list_head_struct *next;
    struct list_head_struct *prev;
} list_head_t;

#define list_head_init(l) \
            ((list_head_t *)l)->next = 0; \
            ((list_head_t *)l)->prev = 0;

int kstrcmp(const char *p, const char *q);

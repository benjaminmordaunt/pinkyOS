#include "types.h"
#include "stat.h"
#include "user.h"

static void
putc(int fd, char c)
{
    write(fd, &c, 1);
}

static void
printint(int fd, int xx, int base, int sgn)
{
    static char digits[] = "0123456789ABCDEF";
    char buf[16];
    int i, neg;
    uint x;
    
    neg = 0;
    if(sgn && xx < 0){
        neg = 1;
        x = -xx;
    } else {
        x = xx;
    }
    
    i = 0;
    do{
        buf[i++] = digits[x % base];
    }while((x /= base) != 0);
    if(neg)
        buf[i++] = '-';
    
    while(--i >= 0)
        putc(fd, buf[i]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void
printf(int fd, char *fmt, ...)
{
    char *s;
    int c, i, state;
    va_list argp;
    
    state = 0;
    va_start(argp, fmt);

    for(i = 0; fmt[i]; i++){
        c = fmt[i] & 0xff;
        if(state == 0){
            if(c == '%'){
                state = '%';
            } else {
                putc(fd, c);
            }
        } else if(state == '%'){
            if(c == 'd'){
                printint(fd, va_arg(argp, uint64), 10, 1);
            } else if(c == 'x' || c == 'p'){
                printint(fd, va_arg(argp, uint64), 16, 0);
            } else if(c == 's'){
                s = va_arg(argp, char*);
                if(s == 0)
                    s = "(null)";
                while(*s != 0){
                    putc(fd, *s);
                    s++;
                }
            } else if(c == 'c'){
		// char is promotable to int, so need to use (at least) int here.
                putc(fd, va_arg(argp, int));
            } else if(c == '%'){
                putc(fd, c);
            } else {
                // Unknown % sequence.  Print it to draw attention.
                putc(fd, '%');
                putc(fd, c);
            }
            state = 0;
        }
    }

    va_end(argp);
}

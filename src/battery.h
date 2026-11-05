#ifndef BATTERY_H
#define BATTERY_H
typedef struct {int valid,percent,charging,average,minutes,capacity,baseline_capacity,baseline_stamp;} Battery;
/* Lines come from the stock gauge: percent, signed mA, available mAh. */
static int battery_number(const char **p,int *out){
    const char *s=*p;int sign=1,n=0;
    if(*s=='-'){sign=-1;s++;}
    if(*s<'0'||*s>'9')return 0;
    while(*s>='0'&&*s<='9'){n=n*10+*s++-'0';if(n>100000)return 0;}
    while(*s&&*s!='\n')s++;
    if(*s=='\n')s++;
    *p=s;*out=n*sign;return 1;
}
static int battery_sample(Battery *b,const char *s){
    int percent,current,capacity;
    if(!battery_number(&s,&percent)||!battery_number(&s,&current)||!battery_number(&s,&capacity)||percent<0||percent>100||capacity<0||capacity>5000||current< -2000||current>2000)return 0;
    b->capacity=capacity;b->valid=1;b->percent=percent;b->charging=current>0;
    if(current<0){int draw=-current;b->average=b->average?(b->average*3+draw+2)/4:draw;b->minutes=capacity*60/b->average;}
    else {b->minutes=-1;b->average=0;}
    return 1;
}
/* Capacity loss over wall time includes sleep, unlike an awake current sample. */
static void battery_elapsed(Battery *b,int stamp){
    if(!b->valid)return;
    if(b->charging||!b->baseline_stamp||stamp<b->baseline_stamp||b->capacity>b->baseline_capacity){
        b->baseline_stamp=stamp;b->baseline_capacity=b->capacity;return;
    }
    int elapsed=stamp-b->baseline_stamp,used=b->baseline_capacity-b->capacity;
    if(elapsed>=900&&used>=5){
        b->minutes=(int)(((long long)b->capacity*elapsed)/(used*60));
    }
}
#endif

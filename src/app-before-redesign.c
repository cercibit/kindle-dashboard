#include "font.h"
void *memcpy(void *d,const void *s,unsigned int n){unsigned char *a=d;const unsigned char *b=s;for(unsigned int i=0;i<n;i++)a[i]=b[i];return d;}
int memcmp(const void *a,const void *b,unsigned int n){const unsigned char *x=a,*y=b;for(unsigned int i=0;i<n;i++)if(x[i]!=y[i])return x[i]-y[i];return 0;}
#include "weather.h"
static int syscall3(int nr,int a,int b,int c) {
    register int r0 __asm__("r0")=a;
    register int r1 __asm__("r1")=b;
    register int r2 __asm__("r2")=c;
    register int r7 __asm__("r7")=nr;
    __asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r7):"memory","cc");
    return r0;
}
static int openfile(const char *p,int flags){return syscall3(5,(int)p,flags,0);}
static int writeall(int fd,const void *p,int count) {
    const char *s=p;
    while(count) {int n=syscall3(4,fd,(int)s,count);if(n<=0)return -1;s+=n;count-=n;}
    return 0;
}
static int message(const char *s){writeall(2,s,length(s));return 1;}
static unsigned char pixels[240000];
static void pixel(int x,int y) {
    if(x<0||x>=600||y<0||y>=800)return;
    int i=y*300+x/2; pixels[i]|=(unsigned char)((x&1)?15:240);
}
static void rect(int x,int y,int w,int h){for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)pixel(i,j);}
static void text(int x,int y,const char *s,int scale) {
    while(*s) {
        unsigned int c=(unsigned char)*s++;
        if(c>=32&&c<=126)for(int j=0;j<20;j++)for(int i=0;i<10;i++)
            if(font[c-32][j]&(1<<i))rect(x+i*scale,y+j*scale,scale,scale);
        x+=10*scale;
    }
}
static int syscall4(int nr,int a,int b,int c,int d){
    register int r0 __asm__("r0")=a;register int r1 __asm__("r1")=b;
    register int r2 __asm__("r2")=c;register int r3 __asm__("r3")=d;register int r7 __asm__("r7")=nr;
    __asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r3),"r"(r7):"memory","cc");return r0;
}
static int now(void){int tv[2]={0,0};syscall3(78,(int)tv,0,0);return tv[0];}
static void centered(int y,const char *s,int scale){text((600-length(s)*10*scale)/2,y,s,scale);}
static void append(char *d,const char *s){strcopy(d+length(d),s);}
static void render(Weather *w,int stamp,int fetching){
    const char *week[]={"SUNDAY","MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY"};
    const char *months[]={"JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"};
    for(int i=0;i<240000;i++)pixels[i]=0;
    centered(24,"BILLERICA, MA 01821",2);
    if(stamp<1577836800||stamp>2145916799){centered(112,"SET KINDLE DATE",2);return;}
    Date d=eastern(stamp);char buf[96],tmp[20],clock[8];
    centered(78,week[d.weekday],2);
    strcopy(buf,months[d.month-1]);append(buf," ");decimal(d.day,tmp);append(buf,tmp);append(buf,", ");decimal(d.year,tmp);append(buf,tmp);centered(116,buf,1);
    int hour=d.hour%12;if(!hour)hour=12;
    decimal(hour,clock);append(clock,":");two(d.minute,tmp);append(clock,tmp);
    centered(155,clock,8);
    strcopy(buf,d.hour<12?"AM / ":"PM / ");append(buf,d.dst?"EASTERN DAYLIGHT TIME":"EASTERN STANDARD TIME");centered(320,buf,1);
    rect(30,364,540,2);
    if(w->valid){
        strcopy(buf,w->temp);append(buf," F");centered(398,buf,length(buf)>8?3:5);
        strcopy(buf,w->label[0]?w->label:condition(w->code));if(length(buf)>27)buf[27]=0;centered(510,buf,2);
        strcopy(buf,"FEELS LIKE ");append(buf,w->feels);append(buf," F");centered(558,buf,1);
        strcopy(buf,"WIND ");append(buf,w->wind);append(buf," MPH   HUMIDITY ");append(buf,w->humidity);append(buf,"%");centered(600,buf,1);
        if(!equal(w->high,"--")){strcopy(buf,"TODAY HIGH ");append(buf,w->high);append(buf," F / LOW ");append(buf,w->low);append(buf," F");centered(636,buf,1);}
        Date update=eastern(w->stamp);strcopy(buf,"UPDATED ");two(update.hour,tmp);append(buf,tmp);append(buf,":");two(update.minute,tmp);append(buf,tmp);append(buf,update.dst?" EDT":" EST");
        if(stamp-w->stamp>3600||w->stamp-stamp>300)append(buf," / STALE");
        centered(686,buf,1);
    }else{centered(430,"WEATHER UNAVAILABLE",2);centered(500,"CONNECT WI-FI AND PRESS R",1);centered(540,"CLOCK CONTINUES OFFLINE",1);}
    rect(30,726,540,1);
    centered(739,fetching?"FETCHING WEATHER... / X EXIT":"R REFRESH / X EXIT",1);
    centered(773,w->label[0]?"WEATHER: WTTR.IN":"WEATHER: OPEN-METEO.COM",1);
}
static char jsondata[16384];
static int load_weather(const char *path,Weather *weather){
    int fd=openfile(path,0);if(fd<0)return 0;
    int total=0,n;
    while(total<16383&&(n=syscall3(3,fd,(int)(jsondata+total),16383-total))>0)total+=n;
    syscall3(6,fd,0,0);if(total==16383)return 0;jsondata[total]=0;
    Weather candidate;if(!read_weather(jsondata,&candidate))return 0;
    *weather=candidate;return 1;
}
struct Poll {int fd;short events,revents;};
struct Event {unsigned int sec,usec;unsigned short type,code;int value;};
static int spawn_fetch(char **env){
    int pid=syscall3(2,0,0,0);
    if(!pid){
        syscall3(66,0,0,0); /* Dedicated process group for timeout and exit cleanup. */
        char *args[]={"/bin/sh","/mnt/us/extensions/weather-clock/fetch.sh",0};
        syscall3(11,(int)args[0],(int)args,(int)env);syscall3(1,127,0,0);
    }
    return pid;
}
int appmain(int argc,char **argv){
    Weather weather;weather.valid=0;weather.label[0]=0;
    int preview=argc>1&&equal(argv[1],"--preview");
    if(argc>1&&equal(argv[1],"--validate")){
        if(argc!=3||!load_weather(argv[2],&weather))return message("Invalid weather response\n");
        return 0;
    }
    if(preview){
        if(argc>2&&!load_weather(argv[2],&weather))return message("Invalid preview weather\n");
        int stamp=argc>3?integer(argv[3]):now();render(&weather,stamp,0);return writeall(1,pixels,240000);
    }
    int fb=openfile("/dev/fb0",2);if(fb<0)fb=openfile("/dev/fb/0",2);
    if(fb<0)return message("Cannot open Kindle framebuffer\n");
    unsigned int info[40],fixed[20];
    if(syscall3(54,fb,0x4600,(int)info)<0||info[0]!=600||info[1]!=800||info[6]!=4||info[4]||info[5]||syscall3(54,fb,0x4602,(int)fixed)<0||fixed[11]!=300)return message("Expected Kindle Keyboard framebuffer\n");
    struct Poll fds[8];int count=0;
    for(int i=0;i<8;i++){
        char path[]="/dev/input/event0";path[16]=(char)('0'+i);int fd=openfile(path,0);if(fd<0)continue;
        unsigned char bits[32];for(int j=0;j<32;j++)bits[j]=0;
        if(syscall3(54,fd,0x80204521,(int)bits)<0||!(bits[2]&1)||syscall3(54,fd,0x40044590,1)<0){syscall3(6,fd,0,0);continue;}
        fds[count].fd=fd;fds[count].events=1;fds[count].revents=0;count++;
    }
    if(!count)return message("Cannot grab Kindle keyboard\n");
    int running=1,dirty=1,lastminute=-1,child=0,started=0,lastfetch=0,updates=0;
    char **env=argv+argc+1;
    while(running){
        int stamp=now();
        if(stamp/60!=lastminute){lastminute=stamp/60;dirty=1;load_weather("/mnt/us/extensions/weather-clock/weather.json",&weather);}
        if(child>0){
            int status=0,reaped=syscall4(114,child,(int)&status,1,0);
            if(reaped==child){child=0;load_weather("/mnt/us/extensions/weather-clock/weather.json",&weather);dirty=1;}
            else if(stamp-started>50){syscall3(37,-child,9,0);syscall4(114,child,(int)&status,0,0);child=0;dirty=1;}
        }
        if(!child&&(!lastfetch||stamp-lastfetch>=900)){
            child=spawn_fetch(env);if(child<0)child=0;started=lastfetch=stamp;dirty=1;
        }
        if(dirty){
            render(&weather,stamp,child>0);syscall3(19,fb,0,0);
            if(writeall(fb,pixels,240000)<0||syscall3(54,fb,0x46db,(updates++%15==0)?1:0)<0){running=0;break;}
            dirty=0;
        }
        int ready=syscall3(168,(int)fds,count,1000);if(ready<0)break;
        for(int i=0;i<count;i++)if(fds[i].revents&1){struct Event e;if(syscall3(3,fds[i].fd,(int)&e,16)!=16){running=0;break;}
            if(e.type==1&&e.value==1){if(e.code==45||e.code==102||e.code==158)running=0;if(e.code==19){if(!child&&stamp-lastfetch>=30)lastfetch=0;dirty=1;}}
        }
    }
    if(child>0){syscall3(37,-child,9,0);int status;syscall4(114,child,(int)&status,0,0);}
    for(int i=0;i<count;i++)syscall3(6,fds[i].fd,0,0);
    syscall3(6,fb,0,0);return 0;
}

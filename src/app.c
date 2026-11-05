#include "ui-assets.h"
void *memcpy(void *d,const void *s,unsigned int n){unsigned char *a=d;const unsigned char *b=s;for(unsigned int i=0;i<n;i++)a[i]=b[i];return d;}
int memcmp(const void *a,const void *b,unsigned int n){const unsigned char *x=a,*y=b;for(unsigned int i=0;i<n;i++)if(x[i]!=y[i])return x[i]-y[i];return 0;}
#include "weather.h"
#include "server.h"
#include "navigation.h"
#include "quotes.h"
#include "battery.h"
#include "power.h"
static Battery battery;
static int wifi_status,ssh_mode,sleeping;
static int maintenance(void);
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
static int syscall4(int nr,int a,int b,int c,int d){
    register int r0 __asm__("r0")=a;register int r1 __asm__("r1")=b;
    register int r2 __asm__("r2")=c;register int r3 __asm__("r3")=d;register int r7 __asm__("r7")=nr;
    __asm__ volatile("svc 0":"+r"(r0):"r"(r1),"r"(r2),"r"(r3),"r"(r7):"memory","cc");return r0;
}
static int now(void){int tv[2]={0,0};syscall3(78,(int)tv,0,0);return tv[0];}
static unsigned char pixels[240000];
/* Supersample all geometry and glyphs for smooth 4-bit e-ink edges. */
static unsigned char canvas[240000];
static void dot(int x,int y){if(x>=0&&x<1200&&y>=0&&y<1600)canvas[y*150+x/8]|=(unsigned char)(128>>(x&7));}
static void disk(int x,int y,int radius){for(int j=-radius;j<=radius;j++)for(int i=-radius;i<=radius;i++)if(i*i+j*j<=radius*radius)dot(x+i,y+j);}
static int absolute(int n){return n<0?-n:n;}
static void stroke(int x,int y,int x2,int y2,int width){
    int dx=absolute(x2-x),sx=x<x2?1:-1,dy=-absolute(y2-y),sy=y<y2?1:-1,err=dx+dy;
    for(;;){disk(x,y,width);if(x==x2&&y==y2)break;int e=2*err;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
}
static int sine(int a){a%=720;if(a<0)a+=720;return a<=180?sinequarter[a]:a<=360?sinequarter[360-a]:a<=540?-sinequarter[a-360]:-sinequarter[720-a];}
static int vx(int a,int r){return sine(a)*r/10000;}
static int vy(int a,int r){return -sine(a+180)*r/10000;}
static void ring(int cx,int cy,int r,int width){for(int i=0;i<720;i++)stroke(cx+vx(i,r),cy+vy(i,r),cx+vx(i+1,r),cy+vy(i+1,r),width);}
static int measure(const char *s,int face){int w=0;while(*s){unsigned int c=(unsigned char)*s++;if(c>=32&&c<=126)w+=faces[face]->widths[c-32];}return (w+1)/2;}
static void label(int x,int y,const char *s,int face){
    const Face *f=faces[face];int xx=x*2;
    while(*s){unsigned int c=(unsigned char)*s++;if(c<32||c>126)continue;int w=f->widths[c-32],stride=(w+7)/8;const unsigned char *b=f->bits+f->offsets[c-32];
        for(int j=0;j<f->height;j++)for(int i=0;i<w;i++)if(b[j*stride+i/8]&(128>>(i&7)))dot(xx+i,y*2+j);
        xx+=w;
    }
}
static void middle(int y,const char *s,int face){label((600-measure(s,face))/2,y,s,face);}
static void right(int x,int y,const char *s,int face){label(x-measure(s,face),y,s,face);}
static void rule(int y){stroke(64,y*2,1136,y*2,0);}
static void append(char *d,const char *s){strcopy(d+length(d),s);}
static void gauge_bar(int x,int y,int w,int h,int pct){
    int x0=x*2,y0=y*2,ww=w*2,hh=h*2;
    stroke(x0,y0,x0+ww,y0,1);
    stroke(x0,y0+hh,x0+ww,y0+hh,1);
    stroke(x0,y0,x0,y0+hh,1);
    stroke(x0+ww,y0,x0+ww,y0+hh,1);
    for(int t=1;t<=3;t++){
        int tx=x0+ww*t/4;
        stroke(tx,y0,tx,y0+5,0);
        stroke(tx,y0+hh-5,tx,y0+hh,0);
    }
    int p=clamp_pct(pct),fill=(ww-12)*p/100;
    for(int yy=y0+6;yy<=y0+hh-6;yy++)
        for(int xx=x0+6;xx<x0+6+fill;xx++)
            dot(xx,yy);
}
static void analog(Date d,int center_y,int radius){
    int cx=600,cy=center_y*2,r=radius*2;
    ring(cx,cy,r,1);
    for(int i=0;i<60;i++){int a=i*12;stroke(cx+vx(a,r-(i%5?14:28)),cy+vy(a,r-(i%5?14:28)),cx+vx(a,r-7),cy+vy(a,r-7),i%5?1:3);}
    int offset=radius*73/100,face=radius>180?2:1;
    label(300-measure("12",face)/2,center_y-offset-11,"12",face);
    label(300+offset-measure("3",face)/2,center_y-13,"3",face);
    label(300-measure("6",face)/2,center_y+offset-13,"6",face);
    label(300-offset-measure("9",face)/2,center_y-13,"9",face);
    int hour=(d.hour%12)*60+d.minute,minute=d.minute*12;
    stroke(cx-vx(hour,25),cy-vy(hour,25),cx+vx(hour,r*53/100),cy+vy(hour,r*53/100),radius>180?9:7);
    stroke(cx-vx(minute,32),cy-vy(minute,32),cx+vx(minute,r*79/100),cy+vy(minute,r*79/100),radius>180?5:4);
    disk(cx,cy,12);
}
static int cloudinside(int x,int y){
    int a=x+38,b=y-4,c=x-12,d=y+16,e=x-55,f=y-6;
    return a*a+b*b<=34*34||c*c+d*d<=49*49||e*e+f*f<=32*32||(y>=0&&y<=38&&x>=-38&&x<=55);
}
static void weathericon(int code,int px,int py){
    int x=px*2,y=py*2;
    if(code<=2){ring(x,y,39,3);for(int i=0;i<8;i++){int a=i*90;stroke(x+vx(a,55),y+vy(a,55),x+vx(a,71),y+vy(a,71),3);}}
    if(code>0){
        for(int j=-70;j<=42;j++)for(int i=-76;i<=90;i++)if(cloudinside(i,j)){
            int px=x+i,py=y+j;
            canvas[py*150+px/8]&=(unsigned char)~(128>>(px&7));
            if(!cloudinside(i-4,j)||!cloudinside(i+4,j)||!cloudinside(i,j-4)||!cloudinside(i,j+4))dot(px,py);
        }
    }
    if((code>=51&&code<70)||code>=80){for(int i=-1;i<=1;i++)stroke(x+i*35,y+52,x+i*35-10,y+76,3);}
    if(code>=71&&code<80){for(int i=-1;i<=1;i++){stroke(x+i*35,y+54,x+i*35,y+72,2);stroke(x+i*35-9,y+63,x+i*35+9,y+63,2);}}
    if(code>=45&&code<=48){stroke(x-60,y+55,x+65,y+55,2);stroke(x-45,y+70,x+48,y+70,2);}
}
static void finish(void){
    for(int y=0;y<800;y++)for(int x=0;x<600;x++){
        int xx=x*2,yy=y*2,n=0;
        for(int j=0;j<2;j++)for(int i=0;i<2;i++)n+=(canvas[(yy+j)*150+(xx+i)/8]>>(7-((xx+i)&7)))&1;
        int shade=(n*15+2)/4,idx=y*300+x/2;
        if(x&1)pixels[idx]|=(unsigned char)shade;else pixels[idx]=(unsigned char)(shade<<4);
    }
}
static void render_home(Weather *w,Date d){
    char buf[128],tmp[20],clock[16];
    analog(d,285,140);
    int hour=d.hour%12;if(!hour)hour=12;decimal(hour,clock);append(clock,":");two(d.minute,tmp);append(clock,tmp);append(clock,d.hour<12?" AM":" PM");middle(438,clock,2);
    middle(477,d.dst?"EASTERN DAYLIGHT TIME":"EASTERN STANDARD TIME",0);rule(516);
    if(w->valid){
        weathericon(w->code,110,567);
        strcopy(buf,w->temp);int n=length(buf);if(n>2&&buf[n-2]=='.'&&buf[n-1]=='0')buf[n-2]=0;
        label(223,520,buf,4);int tx=223+measure(buf,4);ring((tx+12)*2,544*2,7,1);label(tx+23,533,"F",2);
        strcopy(buf,w->label[0]?w->label:condition(w->code));if(length(buf)>26)buf[26]=0;label(225,606,buf,1);
        strcopy(buf,"Feels like ");append(buf,w->feels);append(buf," F");label(225,634,buf,0);
        rule(677);
        label(32,693,"WIND",0);strcopy(buf,w->wind);append(buf," mph");label(32,713,buf,1);
        label(215,693,"HUMIDITY",0);strcopy(buf,w->humidity);append(buf,"%");label(215,713,buf,1);
        label(402,693,"HIGH / LOW",0);strcopy(buf,w->high);append(buf," / ");append(buf,w->low);label(402,713,buf,1);
    }else{middle(555,"Weather unavailable",2);middle(605,"Connect Wi-Fi and press R",1);}
}
static void render_clock(Weather *w,Date d){
    char buf[128],tmp[20];
    analog(d,359,224);
    decimal(d.hour%12?d.hour%12:12,buf);append(buf,":");two(d.minute,tmp);append(buf,tmp);append(buf,d.hour<12?" AM":" PM");middle(597,buf,3);
    middle(646,d.dst?"EASTERN DAYLIGHT TIME":"EASTERN STANDARD TIME",0);rule(682);
    if(w->valid){strcopy(buf,w->temp);append(buf," F   /   ");append(buf,w->label[0]?w->label:condition(w->code));middle(699,buf,1);}
    else middle(699,"Weather unavailable - R to refresh",1);
}
/* Monitor surface: a single high-contrast instrument, no decorative panels.
 * All geometry is at 2x resolution. No seconds hand or animated refresh. */
static void render_easy_read(Date d){
    int cx=600,cy=788,r=484;
    ring(cx,cy,r,2);
    for(int i=0;i<60;i++){
        int a=i*12,major=i%5==0;
        stroke(cx+vx(a,r-(major?38:17)),cy+vy(a,r-(major?38:17)),
               cx+vx(a,r-10),cy+vy(a,r-10),major?5:2);
    }
    for(int i=1;i<=12;i++){
        char n[4];decimal(i,n);
        label(300+vx(i*60,192)-measure(n,5)/2,394+vy(i*60,192)-22,n,5);
    }
    int hour=(d.hour%12)*60+d.minute,minute=d.minute*12;
    /* Short, heavy hour hand; long, slimmer minute hand. */
    stroke(cx,cy,cx+vx(hour,244),cy+vy(hour,244),15);
    stroke(cx,cy,cx+vx(minute,340),cy+vy(minute,340),7);
    disk(cx,cy,20);
    char clock[32],tmp[16];decimal(d.hour%12?d.hour%12:12,clock);
    append(clock,":");two(d.minute,tmp);append(clock,tmp);
    append(clock,d.hour<12?" AM":" PM");middle(651,clock,5);
    middle(706,d.dst?"EASTERN DAYLIGHT TIME":"EASTERN STANDARD TIME",0);
}
static const char *weekdays[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
static const char *monthnames[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
static void render_forecast(Weather *w,Date d){
    char buf[128],tmp[20];
    if(w->valid){
        label(32,147,"RIGHT NOW",0);strcopy(buf,w->temp);append(buf," F");label(28,165,buf,4);
        weathericon(w->code,487,221);
        strcopy(buf,w->label[0]?w->label:condition(w->code));label(32,258,buf,1);
        strcopy(buf,"Feels like ");append(buf,w->feels);append(buf," F   /   Humidity ");append(buf,w->humidity);append(buf,"%");label(32,298,buf,0);
        strcopy(buf,"Wind ");append(buf,w->wind);append(buf," mph   /   Today ");append(buf,w->high);append(buf," / ");append(buf,w->low);append(buf," F");label(32,323,buf,0);
    }else{middle(195,"Weather unavailable",2);middle(250,"Connect Wi-Fi and press R",1);}
    rule(370);label(32,383,"THE NEXT TWO DAYS",0);
    int shown=0,today=days_before(d.year,d.month,d.day);
    for(int i=0;i<w->forecast_count&&shown<2;i++){
        Forecast *f=&w->forecast[i];Date day=eastern(f->stamp);int gap=days_before(day.year,day.month,day.day)-today;
        if(gap<1||gap>2)continue;
        int y=416+shown*162;
        strcopy(buf,weekdays[day.weekday]);append(buf,", ");append(buf,monthnames[day.month-1]);append(buf," ");decimal(day.day,tmp);append(buf,tmp);label(32,y,buf,2);
        weathericon(f->code,83,y+81);
        strcopy(buf,f->high);append(buf," / ");append(buf,f->low);append(buf," F");label(161,y+44,buf,3);
        label(161,y+86,condition(f->code),1);
        strcopy(buf,"Rain ");append(buf,f->rain);append(buf,"%   /   Wind up to ");append(buf,f->wind);append(buf," mph");label(161,y+117,buf,0);
        if(!shown)rule(y+151);
        shown++;
    }
    if(!shown){middle(488,"Forecast not available",2);middle(534,"Press R to download the next two days",0);}
}
/* Word wrapping uses the actual embedded serif glyph widths. */
static int quote_lines(const char *text,int y,int draw){
    int count=0;
    while(*text){
        char line[128];int n=0,lastspace=-1;
        while(text[n]&&n<126){
            line[n]=text[n];line[n+1]=0;
            if(measure(line,3)>480)break;
            if(text[n]==' ')lastspace=n;
            n++;
        }
        if(text[n]&&lastspace>0)n=lastspace;
        line[n]=0;
        if(draw)middle(y+count*48,line,3);
        count++;text+=n;while(*text==' ')text++;
    }
    return count;
}
static void render_quote(Date d){
    const char *text=quotes[quote_index(d)];
    middle(186,"DAILY QUOTE",2);rule(238);
    int lines=quote_lines(text,0,0);
    quote_lines(text,424-lines*24,1);
    rule(610);middle(644,"A fresh thought for each new day",1);
    middle(678,"Changes at midnight Eastern time",0);
}
static void render_server(Server *s){
    char buf[128];
    label(32,136,"LENOVO SERVER",2);
    if(!s->valid){
        rule(198);
        middle(350,"Server telemetry unavailable",2);
        middle(400,"Connect Wi-Fi and press R",1);
        return;
    }
    strcopy(buf,"UP ");append(buf,s->uptime);right(568,142,buf,1);
    strcopy(buf,s->hostname);append(buf,"  /  i7-3630QM  /  GT 750M");label(32,172,buf,0);
    strcopy(buf,"LOAD ");append(buf,s->load);right(568,172,buf,0);
    rule(198);
    label(32,206,"CPU",1);
    strcopy(buf,s->cpu_usage);append(buf,"%   (");append(buf,s->cpu_freq);append(buf," MHz / ");append(buf,s->cpu_threads);append(buf,"T)");
    right(568,210,buf,0);
    gauge_bar(32,234,536,18,s->cpu_pct);
    label(32,264,"MEMORY",1);
    strcopy(buf,s->mem_pct_str);append(buf,"%   (");append(buf,s->mem_used_gib);append(buf," / ");append(buf,s->mem_total_gib);append(buf," GiB   Swap ");append(buf,s->swap_pct_str);append(buf,"%)");
    right(568,268,buf,0);
    gauge_bar(32,292,536,18,s->mem_pct);
    label(32,322,"CPU TEMP",1);
    strcopy(buf,s->cpu_temp);append(buf," C");right(284,324,buf,1);
    gauge_bar(32,350,252,18,s->cpu_temp_pct);
    label(316,322,"GPU TEMP",1);
    strcopy(buf,s->gpu_temp);append(buf," C");right(568,324,buf,1);
    gauge_bar(316,350,252,18,s->gpu_temp_pct);
    for(int i=0;i<s->disk_count&&i<2;i++){
        int dy=380+i*58;
        ServerDisk *d=&s->disks[i];
        label(32,dy,d->label,1);
        strcopy(buf,d->pct_str);append(buf,"%   (");append(buf,d->used_gib);append(buf," / ");append(buf,d->size_gib);append(buf," GiB, ");append(buf,d->avail_gib);append(buf," GiB free)");
        right(568,dy+4,buf,0);
        gauge_bar(32,dy+28,536,18,d->use_pct);
    }
    rule(506);
    label(32,514,"TOP PROCESSES",0);
    right(355,514,"USER",0);
    right(462,514,"CPU%",0);
    right(568,514,"MEM%",0);
    for(int i=0;i<s->proc_count&&i<5;i++){
        int py=542+i*38;
        ServerProc *p=&s->procs[i];
        label(32,py,p->command,1);
        right(355,py+3,p->user,0);
        strcopy(buf,p->cpu);append(buf,"%");right(462,py,buf,1);
        strcopy(buf,p->mem);append(buf,"%");right(568,py,buf,1);
    }
}
static void render(Weather *w,Server *s,int stamp,int fetching,int page){
    for(int i=0;i<240000;i++)canvas[i]=0;
    label(32,6,ssh_mode?"SSH MODE":"BATTERY-SAVING",2);
    const char *wifi_names[]={"Wi-Fi OFF","Wi-Fi CONNECTING","Wi-Fi CONNECTED","Wi-Fi UNKNOWN"};
    char connection[80];strcopy(connection,wifi_names[wifi_status]);
    append(connection,sleeping?" / SLEEPING":" / AWAKE");label(32,46,connection,0);
    char power[80],number[20];
    if(battery.valid){
        strcopy(power,"BATTERY ");decimal(battery.percent,number);append(power,number);append(power,"%");right(568,14,power,0);
        if(battery.charging)strcopy(power,"Charging");
        else if(battery.minutes>=0){
            int minutes=((battery.minutes+7)/15)*15;
            strcopy(power,"~ ");decimal(minutes/60,number);append(power,number);append(power,"h ");two(minutes%60,number);append(power,number);append(power,"m remaining");
        }else strcopy(power,"Estimating runtime...");
        right(568,39,power,0);
    }else right(568,34,"Battery unavailable",0);
    rule(78);
    if(stamp<1577836800||stamp>2145916799){middle(250,"Please set the Kindle date",2);finish();return;}
    Date d=eastern(stamp);char buf[128],tmp[20];
    strcopy(buf,weekdays[d.weekday]);append(buf,", ");append(buf,monthnames[d.month-1]);append(buf," ");decimal(d.day,tmp);append(buf,tmp);middle(93,buf,3);
    if(page==5)render_server(s);else if(page==4)render_easy_read(d);else if(page==1)render_clock(w,d);else if(page==2)render_forecast(w,d);else if(page==3)render_quote(d);else render_home(w,d);
    rule(746);
    if(page==5){
        if(s->valid){
            strcopy(buf,"Server ");
            append(buf,s->updated[0]?s->updated:"online");
            append(buf," (5s live)");
        }else if(fetching)strcopy(buf,"Updating...");
        else strcopy(buf,"Offline");
    }else{
        strcopy(buf,fetching?"Updating...":w->valid?"Updated ":"Offline");
        if(w->valid&&!fetching){Date update=eastern(w->stamp);two(update.hour,tmp);append(buf,tmp);append(buf,":");two(update.minute,tmp);append(buf,tmp);}
        if(w->valid&&(stamp-w->stamp>3600||w->stamp-stamp>300))append(buf," / STALE");
    }
    label(32,750,buf,0);right(568,750,"M MODE   R REFRESH   X EXIT",0);
    const char *pages[]={"< PAGE     1 / 6  OVERVIEW     PAGE >","< PAGE     2 / 6  CLOCK     PAGE >","< PAGE     3 / 6  FORECAST     PAGE >","< PAGE     4 / 6  DAILY QUOTE     PAGE >","< PAGE     5 / 6  EASY READ     PAGE >","< PAGE     6 / 6  LENOVO SERVER     PAGE >"};
    middle(775,sleeping?"SLEEPING - Slide power to wake, then M to switch mode":pages[page],0);finish();
}
static char jsondata[16384];
static int read_json_file(const char *path){
    int fd=openfile(path,0);if(fd<0)return 0;
    int total=0,n;
    while(total<16383&&(n=syscall3(3,fd,(int)(jsondata+total),16383-total))>0)total+=n;
    syscall3(6,fd,0,0);if(total==16383||total<=0)return 0;
    jsondata[total]=0;return 1;
}
static int load_weather(const char *path,Weather *weather){
    if(!read_json_file(path))return 0;
    Weather candidate;if(!read_weather(jsondata,&candidate))return 0;
    *weather=candidate;return 1;
}
static int load_server(const char *path,Server *server){
    if(!read_json_file(path))return 0;
    Server candidate;if(!read_server(jsondata,&candidate))return 0;
    *server=candidate;return 1;
}
static void load_battery(void){
    char sample[128];int fd=openfile("/tmp/weather-clock-battery",0);battery.valid=0;
    if(fd<0)return;
    int n=syscall3(3,fd,(int)sample,127);syscall3(6,fd,0,0);
    if(n<=0||n>=127)return;
    sample[n]=0;
    if(battery_sample(&battery,sample))battery_elapsed(&battery,now());
}
static int spawn_battery(char **env){
    int pid=syscall3(2,0,0,0);
    if(!pid){
        char *args[]={"/bin/sh","/mnt/us/extensions/weather-clock/battery.sh",0};
        syscall3(11,(int)args[0],(int)args,(int)env);syscall3(1,127,0,0);
    }
    return pid;
}
static int maintenance(void){
    int fd=openfile("/tmp/weather-clock-maintenance",0);
    if(fd<0)return 0;
    syscall3(6,fd,0,0);return 1;
}
/* This model's RTC driver accepts a relative alarm in wakeup_enable. */
static int radio_loaded(void){
    int fd=openfile("/sys/module/ar6000",0);if(fd<0)return 0;
    syscall3(6,fd,0,0);return 1;
}
static int read_wifi_status(void){
    if(!radio_loaded())return 0;
    char carrier=0;int fd=openfile("/sys/class/net/wlan0/carrier",0);
    if(fd<0)return 3;
    int n=syscall3(3,fd,(int)&carrier,1);syscall3(6,fd,0,0);
    if(n!=1)return 3;
    return carrier=='1'?2:1;
}
static int sleep_until(int seconds){
    char value[20];decimal(seconds,value);
    int fd=openfile("/sys/devices/platform/mxc_rtc.0/wakeup_enable",1);
    if(fd<0)return 0;
    int ok=writeall(fd,value,length(value))==0;syscall3(6,fd,0,0);
    if(!ok)return 0;
    fd=openfile("/sys/power/state",1);if(fd<0)return 0;
    int before=now();ok=writeall(fd,"mem",3)==0;syscall3(6,fd,0,0);
    char log[80],number[20];decimal(before,log);append(log," ");decimal(now(),number);append(log,number);append(log,ok?" resumed\n":" failed\n");
    fd=syscall3(5,(int)"/tmp/weather-clock-sleep",577,384);
    if(fd>=0){writeall(fd,log,length(log));syscall3(6,fd,0,0);}
    return ok;
}
struct Poll {int fd;short events,revents;};
struct Event {unsigned int sec,usec;unsigned short type,code;int value;};
static int spawn_fetch(char **env,int mode){
    int pid=syscall3(2,0,0,0);
    if(!pid){
        syscall3(66,0,0,0); /* Dedicated process group for timeout and exit cleanup. */
        const char *flag=mode==1?"--server":(mode==2?"--wifi-off":0);
        char *args[]={"/bin/sh","/mnt/us/extensions/weather-clock/fetch.sh",(char *)flag,0};
        syscall3(11,(int)args[0],(int)args,(int)env);syscall3(1,127,0,0);
    }
    return pid;
}
int appmain(int argc,char **argv){
    Weather weather;weather.valid=0;weather.forecast_count=0;weather.label[0]=0;
    Server server;server.valid=0;server.disk_count=0;server.proc_count=0;
    int preview=argc>1&&equal(argv[1],"--preview");
    if(argc>1&&equal(argv[1],"--validate")){
        if(argc!=3||!load_weather(argv[2],&weather))return message("Invalid weather response\n");
        return 0;
    }
    if(argc>1&&equal(argv[1],"--validate-server")){
        if(argc!=3||!load_server(argv[2],&server))return message("Invalid server response\n");
        return 0;
    }
    if(preview){
        if(argc>2&&!load_weather(argv[2],&weather)&&!load_server(argv[2],&server))return message("Invalid preview data\n");
        if(argc>5)battery_sample(&battery,argv[5]);
        if(argc>6){wifi_status=integer(argv[6]);if(wifi_status<0||wifi_status>3)wifi_status=3;}
        if(argc>7)ssh_mode=integer(argv[7])!=0;
        if(argc>8)sleeping=integer(argv[8])!=0;
        if(argc>9&&!load_server(argv[9],&server))return message("Invalid preview server\n");
        int stamp=argc>3?integer(argv[3]):now(),page=argc>4?integer(argv[4]):0;if(page<0||page>=PAGE_COUNT)page=0;render(&weather,&server,stamp,0,page);return writeall(1,pixels,240000);
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
    int running=1,dirty=1,lastminute=-1,child=0,started=0,lastfetch=0,lastserver=0,updates=0,page=0,batterychild=0,batterystart=0,lastinput=now(),lastday=-1,sleepfailed=0;
    char **env=argv+argc+1;
    load_weather("/mnt/us/extensions/weather-clock/weather.json",&weather);
    load_server("/mnt/us/extensions/weather-clock/server.json",&server);
    while(running){
        int stamp=now();
        int wifi=read_wifi_status(),mode=maintenance();
        if(wifi!=wifi_status||mode!=ssh_mode){wifi_status=wifi;ssh_mode=mode;dirty=1;}
        if(batterychild>0){
            int status=0;
            if(syscall4(114,batterychild,(int)&status,1,0)==batterychild){batterychild=0;load_battery();dirty=1;}
            else if(stamp-batterystart>10){syscall3(37,batterychild,9,0);syscall4(114,batterychild,(int)&status,0,0);batterychild=0;battery.valid=0;dirty=1;}
        }
        if(!batterychild&&(!batterystart||stamp-batterystart>=300)){batterychild=spawn_battery(env);if(batterychild<0)batterychild=0;batterystart=stamp;}
        if(stamp/60!=lastminute){
            lastminute=stamp/60;
            if(page<2||page==4)dirty=1;
            load_weather("/mnt/us/extensions/weather-clock/weather.json",&weather);
        }
        if(child>0){
            int status=0,reaped=syscall4(114,child,(int)&status,1,0);
            if(reaped==child){
                child=0;
                if(page==5)load_server("/mnt/us/extensions/weather-clock/server.json",&server);
                else load_weather("/mnt/us/extensions/weather-clock/weather.json",&weather);
                dirty=1;
            }
            else if(stamp-started>100){syscall3(37,-child,9,0);syscall4(114,child,(int)&status,0,0);child=0;dirty=1;}
        }
        if(!child){
            if(page==5){
                /* Only fetch server telemetry when on page 5, every 5 seconds. */
                if(!lastserver||stamp-lastserver>=5){
                    child=spawn_fetch(env,1);if(child<0)child=0;started=lastserver=stamp;
                    if(!server.valid)dirty=1;
                }
            }else{
                /* Never fetch server telemetry when page 5 is inactive; only hourly weather. */
                if(!lastfetch||stamp-lastfetch>=3600){
                    child=spawn_fetch(env,0);if(child<0)child=0;started=lastfetch=stamp;dirty=1;
                }
            }
        }
        Date local=eastern(stamp);int today=days_before(local.year,local.month,local.day);
        if(today!=lastday){lastday=today;dirty=1;}
        if(dirty){
            render(&weather,&server,stamp,child>0,page);syscall3(19,fb,0,0);
            int full=(page==5)?(updates++%60==0):(updates++%15==0);
            if(writeall(fb,pixels,240000)<0||syscall3(54,fb,0x46db,full?1:0)<0){running=0;break;}
            dirty=0;
        }
        int ready=syscall3(168,(int)fds,count,child>0?150:1000);if(ready<0)break;
        for(int i=0;i<count;i++)if(fds[i].revents&1){struct Event e;if(syscall3(3,fds[i].fd,(int)&e,16)!=16){running=0;break;}
            if(e.type==1&&e.value==1){lastinput=now();sleeping=0;
                if(e.code==50){
                    if(maintenance())syscall3(10,(int)"/tmp/weather-clock-maintenance",0,0);
                    else {int fd=syscall3(5,(int)"/tmp/weather-clock-maintenance",65,384);if(fd>=0)syscall3(6,fd,0,0);}
                    ssh_mode=maintenance();
                    /* Acknowledge the mode before starting any network work. */
                    render(&weather,&server,now(),child>0,page);syscall3(19,fb,0,0);
                    if(writeall(fb,pixels,240000)<0||syscall3(54,fb,0x46db,0)<0){running=0;break;}
                    if(!child){if(page==5)lastserver=0;else lastfetch=0;}
                    dirty=1;
                }
                int next=turn_page(page,e.code);
                if(next!=page){
                    int prev=page;
                    page=next;dirty=1;updates=0;
                    if(page==5&&!child)lastserver=0;
                    else if(prev==5&&!child&&!maintenance()){
                        child=spawn_fetch(env,2);if(child<0)child=0;started=stamp;
                    }
                }
                if(e.code==45||e.code==102||e.code==158)running=0;
                if(e.code==19){
                    if(!child){
                        if(page==5)lastserver=0;
                        else if(stamp-lastfetch>=30)lastfetch=0;
                    }
                    dirty=1;
                }
            }
        }
        if(running&&page!=5&&!child&&!batterychild&&!dirty&&stamp-lastinput>=5&&!maintenance()&&stamp>=sleepfailed&&!radio_loaded()){
            int wait=idle_seconds(stamp,page,batterystart,lastfetch);
            /* A page key or power switch can wake the device before the alarm. */
            if(wait>3){
                sleeping=1;render(&weather,&server,now(),0,page);syscall3(19,fb,0,0);
                if(writeall(fb,pixels,240000)<0||syscall3(54,fb,0x46db,0)<0){running=0;break;}
                /* Do not freeze over a key that arrived while drawing the status. */
                if(syscall3(168,(int)fds,count,0)==0){
                    int before=now();
                    if(!sleep_until(wait))sleepfailed=before+300;
                    else if(now()-before<wait-2)lastinput=now();
                }
                sleeping=0;dirty=1;
            }
        }
    }
    if(batterychild>0){syscall3(37,batterychild,9,0);int status;syscall4(114,batterychild,(int)&status,0,0);}
    if(child>0){syscall3(37,-child,9,0);int status;syscall4(114,child,(int)&status,0,0);}
    for(int i=0;i<count;i++)syscall3(6,fds[i].fd,0,0);
    syscall3(6,fb,0,0);return 0;
}

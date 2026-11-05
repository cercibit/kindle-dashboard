#ifndef WEATHER_H
#define WEATHER_H
/* A bounded JSON reader. Every token retains its subtree boundary. */
typedef struct { int start,end,next; char type; } Token;
typedef struct { const char *s; int pos,n,len; Token t[1024]; } Json;
static int length(const char *s){int n=0;while(s[n])n++;return n;}
static void strcopy(char *d,const char *s){while((*d++=*s++)){} }
static int equal(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void space(Json *j){while(j->pos<j->len&&(j->s[j->pos]==' '||j->s[j->pos]=='\n'||j->s[j->pos]=='\r'||j->s[j->pos]=='\t'))j->pos++;}
static int value(Json *j,int depth){
    space(j);if(depth>20||j->n>=1024||j->pos>=j->len)return -1;
    int id=j->n++;Token *t=&j->t[id];t->start=j->pos;t->type=j->s[j->pos++];
    if(t->type=='{'||t->type=='['){
        char close=t->type=='{'?'}':']';space(j);
        if(j->pos>=j->len)return -1;
        if(j->s[j->pos]!=close)for(;;){
            if(t->type=='{'){
                int k=value(j,depth+1);if(k<0||j->t[k].type!='"')return -1;
                space(j);if(j->pos>=j->len||j->s[j->pos++]!=':')return -1;
            }
            if(value(j,depth+1)<0)return -1;
            space(j);if(j->pos>=j->len)return -1;
            if(j->s[j->pos]==close)break;
            if(j->s[j->pos++]!=',')return -1;
        }
        j->pos++;
    }else if(t->type=='"'){
        int closed=0;
        while(j->pos<j->len){char ch=j->s[j->pos++];if((unsigned char)ch<32)return -1;if(ch=='"'){closed=1;break;}if(ch=='\\'){if(j->pos>=j->len)return -1;j->pos++;}}
        if(!closed)return -1;
    }else{
        while(j->pos<j->len){char ch=j->s[j->pos];if(ch==','||ch=='}'||ch==']'||ch==' '||ch=='\n'||ch=='\r'||ch=='\t')break;j->pos++;}
    }
    t->end=j->pos;t->next=j->n;return id;
}
static int parse(Json *j,const char *s){j->s=s;j->pos=0;j->n=0;j->len=length(s);int id=value(j,0);space(j);return id==0&&j->pos==j->len&&j->t[0].type=='{';}
static int token_is(Json *j,int id,const char *s){
    if(id<0)return 0;
    Token t=j->t[id];int start=t.start+(t.type=='"'),end=t.end-(t.type=='"');
    return end-start==length(s)&&!__builtin_memcmp(j->s+start,s,(unsigned int)(end-start));
}
static int field(Json *j,int id,const char *name){
    if(id<0||j->t[id].type!='{')return -1;
    for(int k=id+1;k<j->t[id].next;){int v=k+1;if(v>=j->t[id].next)return -1;if(token_is(j,k,name))return v;k=j->t[v].next;}
    return -1;
}
static int numeric(Json *j,int id,char *out,int cap){
    if(id<0)return 0;
    Token t=j->t[id];int n=t.end-t.start;
    if(n<1||n>=cap||t.type=='"')return 0;
    int digits=0,dots=0;
    for(int i=0;i<n;i++){char ch=j->s[t.start+i];if(ch>='0'&&ch<='9')digits++;else if(ch=='.'&&dots++==0){}else if(ch=='-'&&i==0){}else return 0;out[i]=ch;}
    out[n]=0;return digits>0;
}
static int integer(const char *s){int n=0;while(*s>='0'&&*s<='9')n=n*10+(*s++-'0');return n;}
static int element(Json *j,int id,int index){
    if(id<0||j->t[id].type!='[')return -1;
    int k=id+1;while(index--&&k<j->t[id].next)k=j->t[k].next;
    return k<j->t[id].next?k:-1;
}
typedef struct {char high[16],low[16],rain[16],wind[16];int stamp,code;} Forecast;
typedef struct {char temp[16],feels[16],wind[16],humidity[16],high[16],low[16],label[40];int code,stamp,valid,forecast_count;Forecast forecast[3];} Weather;
static int read_weather(const char *data,Weather *w){
    Json j;char n[16];w->valid=0;w->forecast_count=0;
    if(!parse(&j,data))return 0;
    int current=field(&j,0,"current"),daily=field(&j,0,"daily"),units=field(&j,0,"current_units");
    int tempunit=field(&j,units,"temperature_2m");
    if((!token_is(&j,tempunit,"\xc2\xb0" "F")&&!token_is(&j,tempunit,"F"))||!token_is(&j,field(&j,units,"wind_speed_10m"),"mp/h"))return 0;
    if(!numeric(&j,field(&j,current,"time"),n,16)||length(n)!=10)return 0;
    if(__builtin_memcmp(n,"2145916799",10)>0)return 0;
    w->stamp=integer(n);
    if(w->stamp<1577836800||w->stamp>2145916799)return 0;
    if(!numeric(&j,field(&j,current,"temperature_2m"),w->temp,16)||
       !numeric(&j,field(&j,current,"apparent_temperature"),w->feels,16)||
       !numeric(&j,field(&j,current,"wind_speed_10m"),w->wind,16)||
       !numeric(&j,field(&j,current,"relative_humidity_2m"),w->humidity,16))return 0;
    w->code=-1;w->label[0]=0;
    if(numeric(&j,field(&j,current,"weather_code"),n,16)&&length(n)<4)w->code=integer(n);
    int label=field(&j,current,"condition");
    if(label>=0&&j.t[label].type=='"'){
        int size=j.t[label].end-j.t[label].start-2;if(size>39)size=39;
        for(int i=0;i<size;i++){char ch=j.s[j.t[label].start+1+i];w->label[i]=(ch>='a'&&ch<='z')?ch-32:((ch>='A'&&ch<='Z')||ch==' '?ch:' ');}w->label[size]=0;
    }
    int hi=field(&j,daily,"temperature_2m_max"),lo=field(&j,daily,"temperature_2m_min");
    strcopy(w->high,"--");strcopy(w->low,"--");
    if(hi>=0&&j.t[hi].type=='['&&j.t[hi].next>hi+1&&numeric(&j,hi+1,n,16))strcopy(w->high,n);
    if(lo>=0&&j.t[lo].type=='['&&j.t[lo].next>lo+1&&numeric(&j,lo+1,n,16))strcopy(w->low,n);
    for(int i=0;i<3;i++){
        Forecast *f=&w->forecast[i];
        if(!numeric(&j,element(&j,field(&j,daily,"time"),i),n,16)||length(n)!=10||__builtin_memcmp(n,"2145916799",10)>0)break;
        f->stamp=integer(n);if(f->stamp<1577836800)break;
        if(!numeric(&j,element(&j,hi,i),f->high,16)||!numeric(&j,element(&j,lo,i),f->low,16))break;
        if(!numeric(&j,element(&j,field(&j,daily,"weather_code"),i),n,16)||length(n)>2||n[0]=='-'||n[0]=='.')break;
        f->code=integer(n);
        if(!numeric(&j,element(&j,field(&j,daily,"precipitation_probability_max"),i),f->rain,16)||length(f->rain)>3||f->rain[0]=='-'||integer(f->rain)>100)break;
        if(!numeric(&j,element(&j,field(&j,daily,"wind_speed_10m_max"),i),f->wind,16))break;
        w->forecast_count++;
    }
    w->valid=1;return 1;
}
static const char *condition(int code){
    switch(code){case 0:return "CLEAR SKY";case 1:return "MOSTLY CLEAR";case 2:return "PARTLY CLOUDY";case 3:return "OVERCAST";
    case 45:case 48:return "FOG";case 51:case 53:case 55:return "DRIZZLE";case 56:case 57:return "FREEZING DRIZZLE";
    case 61:case 63:case 65:return "RAIN";case 66:case 67:return "FREEZING RAIN";case 71:case 73:case 75:case 77:return "SNOW";
    case 80:case 81:case 82:return "RAIN SHOWERS";case 85:case 86:return "SNOW SHOWERS";case 95:case 96:case 99:return "THUNDERSTORM";default:return "WEATHER UNKNOWN";}
}
typedef struct {int year,month,day,hour,minute,weekday,dst;} Date;
static int leap(int y){return y%4==0&&(y%100!=0||y%400==0);}
static int monthdays(int y,int m){const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};return days[m-1]+(m==2&&leap(y));}
static int days_before(int y,int m,int d){int days=0;for(int i=1970;i<y;i++)days+=365+leap(i);for(int i=1;i<m;i++)days+=monthdays(y,i);return days+d-1;}
static Date eastern(int stamp){
    Date d;int days=stamp/86400,rest=days;d.year=1970;
    while(rest>=365+leap(d.year)){rest-=365+leap(d.year);d.year++;}
    int march1=days_before(d.year,3,1),nov1=days_before(d.year,11,1);
    int startday=1+(7-(march1+4)%7)%7+7,endday=1+(7-(nov1+4)%7)%7;
    int start=days_before(d.year,3,startday)*86400+7*3600;
    int end=days_before(d.year,11,endday)*86400+6*3600;
    d.dst=stamp>=start&&stamp<end;
    int local=stamp-(d.dst?4:5)*3600;
    days=local/86400;rest=days;d.year=1970;
    while(rest>=365+leap(d.year)){rest-=365+leap(d.year);d.year++;}
    d.month=1;while(rest>=monthdays(d.year,d.month)){rest-=monthdays(d.year,d.month);d.month++;}
    d.day=rest+1;d.weekday=(days+4)%7;d.hour=local%86400/3600;d.minute=local%3600/60;return d;
}
static void decimal(int n,char *out){char rev[16];int i=0,j=0;do{rev[i++]=(char)('0'+n%10);n/=10;}while(n);while(i)out[j++]=rev[--i];out[j]=0;}
static void two(int n,char *out){out[0]=(char)('0'+n/10);out[1]=(char)('0'+n%10);out[2]=0;}
#endif

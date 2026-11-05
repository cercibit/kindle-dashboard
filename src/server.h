#ifndef SERVER_H
#define SERVER_H
typedef struct {
    char label[28],pct_str[8],used_gib[12],size_gib[12],avail_gib[12];
    int use_pct;
} ServerDisk;
typedef struct {
    char command[24],user[16],cpu[12],mem[12];
} ServerProc;
typedef struct {
    int valid,cpu_pct,mem_pct,cpu_temp_pct,gpu_temp_pct,disk_count,proc_count;
    char hostname[28],uptime[28],load[32],updated[16];
    char cpu_usage[12],cpu_freq[12],cpu_threads[8],cpu_temp[12];
    char gpu_temp[12],gpu_mem[24];
    char mem_pct_str[12],mem_used_gib[12],mem_total_gib[12],swap_pct_str[12];
    ServerDisk disks[2];
    ServerProc procs[5];
} Server;
static int jstring(Json *j,int id,char *out,int cap){
    if(id<0||j->t[id].type!='"'||cap<1)return 0;
    int start=j->t[id].start+1,n=j->t[id].end-start-1;
    if(n<0)return 0;
    if(n>=cap)n=cap-1;
    for(int i=0;i<n;i++){
        char ch=j->s[start+i];
        out[i]=(ch>=32&&ch<=126)?ch:' ';
    }
    out[n]=0;return 1;
}
static int clamp_pct(int v){return v<0?0:(v>100?100:v);}
static int round_pct(const char *s){
    if(!s||!*s||*s=='-')return 0;
    int n=0;
    while(*s>='0'&&*s<='9')n=n*10+(*s++-'0');
    if(*s=='.'&&s[1]>='5'&&s[1]<='9')n++;
    return clamp_pct(n);
}
static void strip_dot_zero(char *s){
    int n=length(s);
    if(n>2&&s[n-2]=='.'&&s[n-1]=='0')s[n-2]=0;
}
static unsigned long long u64(const char *s){
    unsigned long long n=0;
    while(*s>='0'&&*s<='9')n=n*10+(unsigned long long)(*s++-'0');
    return n;
}
static void gib_whole(const char *bytes_str,char *out){
    unsigned long long b=u64(bytes_str);
    int g=(int)((b+(1ULL<<29))>>30);
    decimal(g,out);
}
static void gib_tenths(const char *bytes_str,char *out){
    unsigned long long b=u64(bytes_str);
    int t=(int)((b*10ULL+(1ULL<<29))>>30);
    decimal(t/10,out);
    int len=length(out);
    out[len]='.';
    out[len+1]=(char)('0'+(t%10));
    out[len+2]=0;
}
static int read_server(const char *data,Server *s){
    Json j;char n[32],n2[32];
    s->valid=0;s->disk_count=0;s->proc_count=0;
    if(!parse(&j,data))return 0;
    if(!token_is(&j,field(&j,0,"ok"),"true"))return 0;
    int sys=field(&j,0,"system"),cpu=field(&j,0,"cpu"),mem=field(&j,0,"memory");
    if(sys<0||cpu<0||mem<0)return 0;
    if(!jstring(&j,field(&j,sys,"hostname"),s->hostname,28))strcopy(s->hostname,"mehmet-lenovo");
    if(!jstring(&j,field(&j,sys,"uptime_human"),s->uptime,28))strcopy(s->uptime,"--");
    s->load[0]=0;
    int lavg=field(&j,sys,"load_avg");
    for(int i=0;i<3;i++){
        if(numeric(&j,element(&j,lavg,i),n,16)){
            if(s->load[0])strcopy(s->load+length(s->load)," / ");
            strcopy(s->load+length(s->load),n);
        }
    }
    if(!s->load[0])strcopy(s->load,"--");
    s->updated[0]=0;
    if(jstring(&j,field(&j,0,"timestamp"),n,32)&&length(n)>=19&&n[13]==':'){
        for(int i=0;i<8;i++)s->updated[i]=n[11+i];
        s->updated[8]=0;
    }
    if(!numeric(&j,field(&j,cpu,"usage_pct"),s->cpu_usage,12))return 0;
    s->cpu_pct=round_pct(s->cpu_usage);
    if(!numeric(&j,field(&j,cpu,"avg_freq_mhz"),s->cpu_freq,12))strcopy(s->cpu_freq,"--");
    if(!numeric(&j,field(&j,cpu,"threads"),s->cpu_threads,8))strcopy(s->cpu_threads,"8");
    if(numeric(&j,field(&j,cpu,"temp_c"),s->cpu_temp,12)){
        strip_dot_zero(s->cpu_temp);
        s->cpu_temp_pct=round_pct(s->cpu_temp);
    }else{strcopy(s->cpu_temp,"--");s->cpu_temp_pct=0;}
    if(!numeric(&j,field(&j,mem,"used_pct"),s->mem_pct_str,12))return 0;
    s->mem_pct=round_pct(s->mem_pct_str);
    if(!numeric(&j,field(&j,mem,"used_bytes"),n,24)||!numeric(&j,field(&j,mem,"total_bytes"),n2,24))return 0;
    gib_tenths(n,s->mem_used_gib);
    gib_tenths(n2,s->mem_total_gib);
    if(!numeric(&j,field(&j,mem,"swap_used_pct"),s->swap_pct_str,12))strcopy(s->swap_pct_str,"0");
    int gpu=field(&j,0,"gpu");
    if(gpu>=0&&numeric(&j,field(&j,gpu,"temp_c"),s->gpu_temp,12)){
        strip_dot_zero(s->gpu_temp);
        s->gpu_temp_pct=round_pct(s->gpu_temp);
    }else{strcopy(s->gpu_temp,"--");s->gpu_temp_pct=0;}
    s->gpu_mem[0]=0;
    if(gpu>=0&&numeric(&j,field(&j,gpu,"mem_used_mib"),n,12)&&numeric(&j,field(&j,gpu,"mem_total_mib"),n2,12)){
        strcopy(s->gpu_mem,n);
        strcopy(s->gpu_mem+length(s->gpu_mem)," / ");
        strcopy(s->gpu_mem+length(s->gpu_mem),n2);
        strcopy(s->gpu_mem+length(s->gpu_mem)," MiB");
    }
    int storage=field(&j,0,"storage"),mounts=field(&j,storage,"mounts");
    for(int i=0;i<8&&s->disk_count<2;i++){
        int m=element(&j,mounts,i);if(m<0)break;
        char mp[64];if(!jstring(&j,field(&j,m,"mountpoint"),mp,64))continue;
        if(mp[0]=='/'&&mp[1]=='b'&&mp[2]=='o'&&mp[3]=='o'&&mp[4]=='t')continue;
        ServerDisk *d=&s->disks[s->disk_count];
        if(!numeric(&j,field(&j,m,"use_pct"),d->pct_str,8)||
           !numeric(&j,field(&j,m,"used_bytes"),n,24)||
           !numeric(&j,field(&j,m,"size_bytes"),n2,24))continue;
        d->use_pct=round_pct(d->pct_str);
        gib_whole(n,d->used_gib);
        gib_whole(n2,d->size_gib);
        if(numeric(&j,field(&j,m,"avail_bytes"),n,24))gib_whole(n,d->avail_gib);
        else strcopy(d->avail_gib,"--");
        if(equal(mp,"/"))strcopy(d->label,"SSD  /");
        else{
            const char *base=mp;
            for(int k=0;mp[k];k++)if(mp[k]=='/'&&mp[k+1])base=mp+k+1;
            strcopy(d->label,"HDD  ");
            int off=length(d->label),k=0;
            while(base[k]&&off+k<27){d->label[off+k]=base[k];k++;}
            d->label[off+k]=0;
        }
        s->disk_count++;
    }
    int procs=field(&j,0,"processes");
    for(int i=0;i<5;i++){
        int p=element(&j,procs,i);if(p<0)break;
        ServerProc *pr=&s->procs[s->proc_count];
        if(!jstring(&j,field(&j,p,"command"),pr->command,22))continue;
        if(!jstring(&j,field(&j,p,"user"),pr->user,14))strcopy(pr->user,"-");
        if(!numeric(&j,field(&j,p,"cpu_pct"),pr->cpu,12))strcopy(pr->cpu,"0.0");
        if(!numeric(&j,field(&j,p,"mem_pct"),pr->mem,12))strcopy(pr->mem,"0.0");
        s->proc_count++;
    }
    s->valid=1;return 1;
}
#endif

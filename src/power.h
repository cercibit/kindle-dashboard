#ifndef POWER_H
#define POWER_H
static int idle_seconds(int stamp,int page,int battery_started,int fetched){
    int wait=page==3?300-stamp%300:60-stamp%60;
    int batterywait=300-(stamp-battery_started);if(batterywait<wait)wait=batterywait;
    int fetchwait=3600-(stamp-fetched);if(fetchwait<wait)wait=fetchwait;
    return wait;
}
#endif

#ifndef NAVIGATION_H
#define NAVIGATION_H
enum { PAGE_COUNT = 6 };
/* Kindle Keyboard: left/right NEXT = 104/191; PREVIOUS = 109/193. */
static int turn_page(int page,int key){
    if(key==104||key==191)return (page+1)%PAGE_COUNT;
    if(key==109||key==193)return (page+PAGE_COUNT-1)%PAGE_COUNT;
    return page;
}
#endif

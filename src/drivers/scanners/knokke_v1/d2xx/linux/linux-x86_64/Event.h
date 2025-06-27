#if !defined(__EVENT_H)
#define __EVENT_H

#if !defined(_WINDOWS)
#include <pthread.h>
#include <sys/time.h>
#endif

#include <stdio.h>

struct Event
{
#if defined(_WINDOWS)
    HANDLE hEvent;
#else
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int             iEvent;
#endif
};

void EventInit(struct Event *pE);
void EventDestroy(struct Event *pE);
void EventSet(struct Event *pE);
void EventReset(struct Event *pE);
int  EventWait(struct Event *pE, long lTimeOut);

#endif

#ifndef ZHLTTHREAD_H__
#define ZHLTTHREAD_H__

#include "cc++/thread.h"

class ZHLTThread : public Thread
{
public:
    ZHLTThread()
        { }
    virtual ~ZHLTThread()
    {
    }

    inline bool _zhlt_isCancelled()
    {
        return m_Cancelled.Wait(0);
    }
    inline bool _zhlt_isCancelled(timeout_t timer)
    {
        return m_Cancelled.Wait(timer);
    }

    Event m_Cancelled;
};

#endif//ZHLTTHREAD_H__

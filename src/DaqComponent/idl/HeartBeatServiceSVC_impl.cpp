#include "HeartBeatServiceSVC_impl.h"

HeartBeatServiceSVC_impl() {}
virtual ~HeartBeatServiceSVC_impl() {}
void HeartBeatServiceSVC_impl::setHB(const char* hb)
{
    m_hb = hb;
}

char* HeartBeatServiceSVC_impl::getHB()
{
    return m_hb;
}
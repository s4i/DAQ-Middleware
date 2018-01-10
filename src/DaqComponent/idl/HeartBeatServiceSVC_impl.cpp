#include "HeartBeatServiceSVC_impl.h"

void HeartBeatServiceSVC_impl::setHeartBeat(const CORBA::Char hb)
{
    m_hbs = hb;
}

CORBA::Char HeartBeatServiceSVC_impl::getHeartBeat()
{
    return m_hbs;
}
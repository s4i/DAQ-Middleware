#include "HeartBeatServiceSVC_impl.h"

HeartBeatServiceSVC_impl::HeartBeatServiceSVC_impl()
{

}

HeartBeatServiceSVC_impl::~HeartBeatServiceSVC_impl()
{

}

RTC::ReturnCode_t HeartBeatServiceSVC_impl::setOperatorToComp(const HB hbs)
{
    m_oc = hbs;
    return RTC::RTC_OK;
}

HB HeartBeatServiceSVC_impl::getOperatorToComp()
{
    HB* hb = new HB;
    *hb = m_oc;
    return m_oc;
}

RTC::ReturnCode_t HeartBeatServiceSVC_impl::setCompToOperator(const HB hbs)
{
    m_co = hbs;
    return RTC::RTC_OK;
}

HB HeartBeatServiceSVC_impl::getCompToOperator()
{
    HB* hb = new HB;
    *hb = m_co;
    return m_co;
}

DAQDone HeartBeatServiceSVC_impl::checkDone()
{
    return m_done;
}

void HeartBeatServiceSVC_impl::setDone()
{
    m_done = DONE;
    ///std::cerr << "set DONE\n";///
}

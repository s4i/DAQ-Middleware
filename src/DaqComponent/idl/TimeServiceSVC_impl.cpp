#include "TimeServiceSVC_impl.h"

TimeServiceSVC_impl::TimeServiceSVC_impl()
{
    // Please add extra constructor code here.
}

TimeServiceSVC_impl::~TimeServiceSVC_impl()
{
    // Please add extra destructor code here.
}

RTC::ReturnCode_t TimeServiceSVC_impl::setTime(const CORBA::Long usec)
{
	m_start = usec;
    return RTC::RTC_OK;
}

CORBA::Long TimeServiceSVC_impl::getTime()
{
    return m_start;
}

DAQDone TimeServiceSVC_impl::checkDone()
{
    return m_done;
}

void TimeServiceSVC_impl::setDone()
{
    m_done = DONE;
    ///std::cerr << "set DONE\n";///
}
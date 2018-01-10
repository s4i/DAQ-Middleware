#include "TimeServiceSVC_impl.h"

TimeServiceSVC_impl::TimeServiceSVC_impl()
{
    // Please add extra constructor code here.
}

TimeServiceSVC_impl::~TimeServiceSVC_impl()
{
    // Please add extra destructor code here.
}

void TimeServiceSVC_impl::setTime(const CORBA::Long tv_usec)
{
	m_start = tv_usec;
}

CORBA::Long TimeServiceSVC_impl::getTime()
{
    return m_start;
}
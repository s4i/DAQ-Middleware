// -*-C++-*-

#include "TimeServiceSkel.h"

#ifndef TIMESERVICESVC_IMPL_H
#define TIMESERVICESVC_IMPL_H

class TimeServiceSVC_impl
    : public virtual POA_TimeService,
      public virtual PortableServer::RefCountServantBase
{
public:
	TimeServiceSVC_impl();
	virtual ~TimeServiceSVC_impl();

    RTC::ReturnCode_t setTime(const CORBA::Long usec);
    CORBA::Long getTime();
    DAQDone checkDone();
    void setDone();

private:
    CORBA::Long m_start;
    DAQDone m_done;
};

#endif // TIMESERVICESVC_IMPL_H
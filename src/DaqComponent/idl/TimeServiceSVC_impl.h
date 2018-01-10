// -*-C++-*-

#include "TimeServiceSkel.h"

#ifndef TIMEOFDAYSERVICESVC_IMPL_H
#define TIMEOFDAYSERVICESVC_IMPL_H

class TimeServiceSVC_impl
    : public virtual POA_TimeService,
      public virtual PortableServer::RefCountServantBase
{
private:
public:
	TimeServiceSVC_impl();
	virtual ~TimeServiceSVC_impl();

    void setTime(const CORBA::Long tv_usec);
    CORBA::Long getTime();

private:
    CORBA::Long m_start;
};

#endif // TIMEOFDAYSERVICESVC_IMPL_H
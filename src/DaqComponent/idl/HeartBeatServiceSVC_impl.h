// -*-C++-*-

#include "HeartBeatServiceSkel.h"

#ifndef HEARTBEATSERVICESVC_IMPL_H
#define HEARTBEATSERVICESVC_IMPL_H

class HeartBeatServiceSVC_impl
	: public virtual POA_HeartBeatService,
      public virtual PortableServer::RefCountServantBase
{
public:
	HeartBeatServiceSVC_impl();
	virtual ~HeartBeatServiceSVC_impl();

	RTC::ReturnCode_t setOperatorToComp(const HB hbs);
	HB getOperatorToComp();
	RTC::ReturnCode_t setCompToOperator(const HB hbs);
	HB getCompToOperator();
    DAQDone checkDone();
    void setDone();

private:
    HB m_oc;
	HB m_co;
    DAQDone m_done;
};

#endif // HEARTBEATSERVICESVC_IMPL_H
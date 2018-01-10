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

	void setOperatorToComp(const CORBA::Char hb);
	CORBA::Char getOperatorToComp();
	void setCompToOperator(const CORBA::Char hb);
	CORBA::Char getCompToOperator();

private:
    CORBA::Char m_oc;
	CORBA::Char m_co;
};

#endif // HEARTBEATSERVICESVC_IMPL_H
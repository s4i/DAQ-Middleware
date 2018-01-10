// -*-C++-*-

#include "HeartBeatServiceSkel.h"

#ifndef HEARTBEATSERVICESVC_IMPL_H
#define HEARTBEATSERVICESVC_IMPL_H

class HeartBeatServiceSVC_impl
	: public virtual POA_HeartBeatService,
      public virtual PortableServer::RefCountServantBase
{
private:
public:
	HeartBeatServiceSVC_impl();
	virtual ~HeartBeatServiceSVC_impl();

    void setHeartBeat(const CORBA::Char hb);
	CORBA::Char getHeartBeat();

private:
    CORBA::Char m_hbs;
};

#endif // HEARTBEATSERVICESVC_IMPL_H
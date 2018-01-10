// -*-C++-*-
#include <iostream>
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

    void setHB(const std::string hb);
	char* getHB();

private:
    char* m_hb;
};

#endif // HEARTBEATSERVICESVC_IMPL_H
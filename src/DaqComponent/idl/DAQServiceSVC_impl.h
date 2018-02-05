// -*-C++-*-
/*!
 * @file  DAQServiceSVC_impl.h
 * @brief Service implementation header of DAQService.idl
 * @date 1-January-2008
 * @author Kazuo Nakayoshi (kazuo.nakayoshi@kek.jp)
 * @author Yoshiji Yasu (yoshiji.yasu@kek.jp)
 *
 * Copyright (C) 2008-2011
 *     Kazuo Nakayoshi and Yoshiji Yasu
 *     High Energy Accelerator Research Organization (KEK), Japan.
 *     All rights reserved.
 *
 */

#include "DAQServiceSkel.h"
#include <iostream>
#include <memory>
#include <rtm/CORBA_SeqUtil.h>

#ifndef DAQSERVICESVC_IMPL_H
#define DAQSERVICESVC_IMPL_H

/*
 * Example class implementing IDL interface DAQService
 */

class DAQServiceSVC_impl
	: public virtual POA_DAQService,
	  public virtual PortableServer::RefCountServantBase
{
private:
	// Make sure all instances are built on the heap by making the
	// destructor non-public
	//virtual ~DAQServiceSVC_impl();

public:
	// standard constructor
	DAQServiceSVC_impl();
	virtual ~DAQServiceSVC_impl();

	DAQLifeCycleState getState() throw(CORBA::SystemException);
	RTC::ReturnCode_t setCommand(DAQCommand command) throw(CORBA::SystemException);
	DAQCommand getCommand() throw(CORBA::SystemException);
	DAQDone checkDone() throw(CORBA::SystemException);
	void setDone() throw(CORBA::SystemException);
	void setStatus(const Status& stat) throw(CORBA::SystemException);
	Status* getStatus() throw(CORBA::SystemException);
	void setCompParams(const NVList& comp_params) throw(CORBA::SystemException);
	NVList* getCompParams() throw(CORBA::SystemException);
	void setRunNo(const CORBA::Long run_no) throw(CORBA::SystemException);
	CORBA::Long getRunNo() throw(CORBA::SystemException);
	void setFatalStatus(const FatalErrorStatus& fatalStaus) throw(CORBA::SystemException);

	FatalErrorStatus* getFatalStatus() throw(CORBA::SystemException);

	RTC::ReturnCode_t setHB() throw(CORBA::SystemException);
	HBMSG getHB() throw(CORBA::SystemException);
	void setStopDaqSystem() throw(CORBA::SystemException)
	{
		this->m_oc = END;
	}
	HeartBeatDone hb_checkDone() throw(CORBA::SystemException);
	void hb_setDone() throw(CORBA::SystemException);

	RTC::ReturnCode_t setTime(const TimeVal& now) throw(CORBA::SystemException);
	TimeVal getTime() throw(CORBA::SystemException);

private:
	DAQCommand m_command;
	int m_new;
	DAQDone m_done;
	DAQLifeCycleState m_state;
	Status m_status;
	FatalErrorStatus m_fatalStatus;
	NVList m_comp_params;
	CORBA::Long   m_run_no;

	HBMSG m_oc;
	int m_hb_new;
	HeartBeatDone m_hb_done;

	TimeVal m_start;
};

#endif // DAQSERVICESVC_IMPL_H
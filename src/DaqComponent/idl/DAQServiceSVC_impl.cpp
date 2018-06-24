// -*- C++ -*-
/*!
 * @file DAQServiceSVC_impl.cpp
 * @brief Service implementation code of DAQService.idl
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

#include "DAQServiceSVC_impl.h"
/*
 * Example implementational code for IDL interface DAQService
 */
DAQServiceSVC_impl::DAQServiceSVC_impl()
    : m_command(CMD_NOP),
      m_new(0),
      m_done(DONE),
      m_state(LOADED),
      m_run_no(0),
      m_hb_msg(DEAD),
      m_hb_new(0),
      m_send_count(0)
{
    // Please add extra constructor code here.
}
DAQServiceSVC_impl::~DAQServiceSVC_impl()
{
    // Please add extra destructor code here.
}
/*
 * Methods corresponding to IDL attributes and operations
 */
DAQLifeCycleState DAQServiceSVC_impl::getState()
{
    return m_state;
}
RTC::ReturnCode_t DAQServiceSVC_impl::setCommand(DAQCommand command)
{
#ifdef OLD
    if (m_done == DONE)
    {
        m_command = command;
        m_new = 1;
        m_done = UNDONE;
        ///std::cerr << "UNDONE\n";
        return RTC::RTC_OK;
    }
    else
    {
        return RTC::RTC_ERROR;
    }
#endif
    m_command = command;
    m_new = 1;
    m_done = UNDONE;
    ///std::cerr << "UNDONE\n";
    return RTC::RTC_OK;
}
DAQCommand DAQServiceSVC_impl::getCommand()
{
    if (m_new)
    {
        m_new = 0;
        return m_command;
    }
    else
        return CMD_NOP;
}
DAQDone DAQServiceSVC_impl::checkDone()
{
    return m_done;
}
void DAQServiceSVC_impl::setDone()
{
    m_done = DONE;
}
void DAQServiceSVC_impl::setStatus(const Status &stat)
{
    m_status = stat;
}
Status *DAQServiceSVC_impl::getStatus()
{
    Status *mystatus = new Status;
    *mystatus = m_status;
    return mystatus;
}
void DAQServiceSVC_impl::setCompParams(const NVList &comp_params)
{
    m_comp_params = comp_params;
}
NVList *DAQServiceSVC_impl::getCompParams()
{
    return &m_comp_params;
}
void DAQServiceSVC_impl::setRunNo(const CORBA::Long run_no)
{
    m_run_no = run_no;
}
CORBA::Long DAQServiceSVC_impl::getRunNo()
{
    return m_run_no;
}
void DAQServiceSVC_impl::setFatalStatus(const FatalErrorStatus &fatalStatus)
{
    std::cerr << "### setFatalStatus:" << fatalStatus.fatalTypes << std::endl;
    m_fatalStatus = fatalStatus;
}
FatalErrorStatus *DAQServiceSVC_impl::getFatalStatus()
{
    FatalErrorStatus *myfatal = new FatalErrorStatus;
    *myfatal = m_fatalStatus;
    return myfatal;
}
void DAQServiceSVC_impl::setHB() // Usually zero
{
    if (m_hb_new)
    {
        m_hb_msg = LIVE;
        m_hb_new = false;
    }
}
HBMSG DAQServiceSVC_impl::getHB()
{
    m_hb_new = true;
    return m_hb_msg;
}
void DAQServiceSVC_impl::resetHB()
{
    m_hb_msg = DEAD;
}
HeartBeatDone DAQServiceSVC_impl::hb_checkDone()
{
    return m_hb_done;
}
void DAQServiceSVC_impl::hb_setDone()
{
    m_hb_done = HBDONE;
}
RTC::ReturnCode_t DAQServiceSVC_impl::setTime(const TimeVal &now)
{
    m_start = now;
    return RTC::RTC_OK;
}
TimeVal DAQServiceSVC_impl::getTime()
{
    TimeVal *start_time = new TimeVal;
    *start_time = m_start;
    return *start_time;
}
void DAQServiceSVC_impl::reset_send_count()
{
    m_send_count = 0;
}
void DAQServiceSVC_impl::inc_send_count()
{
    m_send_count++;
}
CORBA::Short DAQServiceSVC_impl::get_send_count()
{
    return m_send_count;
}
// End of example implementational code
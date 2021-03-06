// -*- C++ -*-
/*!
 * @file DaqOperator.h
 * @brief Run Controller and user interface component.
 * @date
 * @author Kazuo Nakayoshi <kazuo.nakayoshi@kek.jp>
 *
 * Copyright (C) 2008
 *     Kazuo Nakayoshi
 *     High Energy Accelerator Research Organization (KEK), Japan.
 *     All rights reserved.
 *
 */

#ifndef DAQOPERATOR_H
#define DAQOPERATOR_H

#include <rtm/Manager.h>
#include <rtm/DataFlowComponentBase.h>
#include <rtm/CorbaPort.h>
#include <rtm/DataInPort.h>
#include <rtm/DataOutPort.h>
//#include <rtm/StringUtil.h>
#include <rtm/CorbaConsumer.h>
#include <rtm/CORBA_SeqUtil.h>
#include <rtm/idl/BasicDataTypeSkel.h>
#include <rtm/SdoConfiguration.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <cstdlib>
#include <pwd.h>
#include <sys/select.h>
#include <sys/time.h>

#include "DAQServiceStub.h"

#include "ComponentInfoContainer.h"
#include "ConfFileParser.h"

#include <xercesc/framework/MemBufInputSource.hpp>
#include "CreateDom.h"
#include "ParameterServer.h"

#include "Timer.h"

using namespace std;
using namespace RTC;

// error code for dom
static constexpr int RET_CODE_IO_ERR = (-14);
static constexpr int RET_CODE_REQ_INV_IN_STS = (-26);

struct serviceInfo
{
    string comp_id;
    RTC::CorbaConsumer<DAQService> daqService;
};

typedef vector<serviceInfo> DaqServiceList;

/*!
 * @class DaqOperator
 * @brief DaqOperator class
 */
class DaqOperator
    : public RTC::DataFlowComponentBase
{
  public:
    static DaqOperator *Instance();

    DaqOperator(RTC::Manager *manager);
    ~DaqOperator();

    // The initialize action (on CREATED->ALIVE transition)
    virtual RTC::ReturnCode_t onInitialize();

    // The startup action when ExecutionContext startup
    virtual RTC::ReturnCode_t onStartup(RTC::UniqueId ec_id);

    virtual RTC::ReturnCode_t onActivated(RTC::UniqueId ec_id);

    // The execution action that is invoked periodically
    virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

    string getMsg();
    string getBody();
    bool parse_body(const char *buf, const string tagname);

    // for callback
    int command_configure();
    int command_unconfigure();
    int command_start();
    int command_stop();
    int command_abort();
    int command_confirmend();
    int command_putparams();
    int command_putstatus();
    int command_log();
    int command_pause();
    int command_resume();
    int command_stopparamsset();
    int command_resetparams();
    int command_save();
    int command_confirmconnection();
    int command_dummy();

    void set_console_flag(bool console);
    void set_port_no(int port);
    string getConfFilePath();

  protected:
    vector<RTC::CorbaPort *> m_DaqServicePorts;
    vector<RTC::CorbaConsumer<DAQService>> m_daqservices;

  private:
    static constexpr int PARAM_PORT = 30000;
    static DaqOperator *_instance;

    int m_comp_num;
    int m_service_num;
    int set_runno(RTC::CorbaConsumer<DAQService> daqservice, unsigned runno);
    int set_command(RTC::CorbaConsumer<DAQService> daqservice, DAQCommand daqcom);

    /* Add flags */
    bool deadFlag; // Dead flag
    bool resFlag;  // Restart flag

    /* Console viewer */
    vector<string> compnames;
    int m_new;
    int copy_compname();

    /* HeartBeat */
    // vector<int> keep_alive;
    // vector<int> keep_dead;
    int check_hb_done(RTC::CorbaConsumer<DAQService> daqservice);

    /* Heart beat timer */
    // Timer *mytimer;
    static constexpr int HB_CYCLE_SEC = 5;
    unique_ptr<Timer> mytimer{new Timer(HB_CYCLE_SEC)};
    int clockwork_hb_recv();
    int reset_mytimer();

    /* Time measurement */
    int set_time();
    int output_performance(int command);
    int state_change_automation();

    int check_done(RTC::CorbaConsumer<DAQService> daqservice);
    int set_sitcp_num(int sitcp_num);
    int set_service_list();

    /* new */
    int error_stop_procedure();
    int other_stop_procedure();

    int configure_procedure();
    int unconfigure_procedure();
    int start_procedure();
    int stop_procedure();
    int pause_procedure();
    int resume_procedure();
    int abort_procedure();
    int putstatus_procedure();
    int log_procedure();

    RTC::ReturnCode_t run_console_mode();
    RTC::ReturnCode_t run_http_mode();
    ///string check_fatal(FatalErrorStatus errStatus);
    string check_state(DAQLifeCycleState compState);
    string check_compStatus(CompStatus compStatus);
    void run_data();
#ifdef NOUSE
    void addCorbaPort();
    void delCorbaPort();
#endif
    void createDom_ok(string name);
    void createDom_ng(string name);
    void createDom_ng(string name, int code, char *str_e, char *str_j);

    int m_curState;
    CORBA::Long m_status;
    CompInfoList m_compInfoList;
    DaqServiceList m_daqServiceList;

    fd_set m_allset;
    fd_set m_rset;
    int m_maxfd;
    struct timeval m_tout;
    DAQLifeCycleState m_state;

    unsigned int m_runNumber;
    string m_start_date;
    string m_stop_date;
    string m_conf_file;
    int m_param_port;

    bool m_com_completed;
    bool m_isConsoleMode;

    vector<::NVList> m_nv_list;
    string m_msg;
    string m_err_msg;
    string m_body;

    string m_config_file;
    string m_config_file_tmp;

    bool m_debug;
    bool m_time;
};
extern "C"
{
    void DaqOperatorInit(RTC::Manager *manager);
};
#endif // DAQOPERATOR_H
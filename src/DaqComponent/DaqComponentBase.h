// -*- C++ -*-
/*!
 * @file DaqComponentBase.h
 * @brief DAQ Component Base class
 * @date 1-Januaary-2008
 * @author Kazuo Nakayoshi (kazuo.nakayoshi@kek.jp)
 * @author Yoshiji Yasu (yoshiji.yasu@kek.jp)
 *
 * Copyright (C) 2008-2011
 *     Kazuo Nakayoshi and Yoshiji Yasu
 *     High Energy Accelerator Research Organization (KEK), Japan.
 *     All rights reserved.
 *
 */

#ifndef DAQCOMPONENTBASE_H
#define DAQCOMPONENTBASE_H

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <memory>
#include <pwd.h> // Mesure Time use
#include <unistd.h>
#include <sys/time.h>

#include <rtm/Manager.h>
#include <rtm/DataFlowComponentBase.h>
#include <rtm/idl/BasicDataTypeSkel.h>
#include <rtm/CorbaPort.h>
#include <rtm/DataInPort.h>
#include <rtm/DataOutPort.h>
#include <rtm/DataPortStatus.h>

#include "DAQServiceSVC_impl.h"
#include "DAQService.hh"
#include "DaqComponentException.h"
#include "Timer.h"

using namespace std;

/*!
 * @namespace DAQMW
 * @brief common namespace of DAQ-Middleware
 */
namespace DAQMW
{
/*!
   * @class DaqComponentBase
   * @brief DaqComponentBase class
   *
   * This is default condition class. User uses the class as base class.
   *
   */
class DaqComponentBase : public RTC::DataFlowComponentBase
{

  public:
    DaqComponentBase(RTC::Manager *manager)
        : RTC::DataFlowComponentBase(manager),
          m_comp_name("NONAME"),
          m_runNumber(0),
          m_eventByteSize(0),
          m_loop(0),
          m_totalEventNum(0),
          m_totalDataSize(0),
          m_trans_lock(false),
          m_DAQServicePort("DAQService"),
          m_command(CMD_NOP),
          m_state(LOADED),
          m_state_prev(LOADED),
          m_isOnError(false),
          m_isTimerAlarm(false),
          m_has_printed_error_log(false),
          m_debug(false),
          m_time(false)
    {
    }

    virtual ~DaqComponentBase()
    {
    }

    enum BufferStatus
    {
        BUF_FATAL = -1,
        BUF_SUCCESS,
        BUF_TIMEOUT,
        BUF_NODATA,
        BUF_NOBUF
    };

  protected:
    DAQServiceSVC_impl m_daq_service0;
    Status m_status;

    static constexpr unsigned int HEADER_BYTE_SIZE = 8;
    static constexpr unsigned int FOOTER_BYTE_SIZE = 8;
    static constexpr unsigned char HEADER_MAGIC = 0xe7;
    static constexpr unsigned char FOOTER_MAGIC = 0xcc;
    static constexpr unsigned int EVENT_BUF_OFFSET = HEADER_BYTE_SIZE;

    /**
         *  The data structure transferring between DAQ-Components is
         *  Header data(8bytes) + Event data + Footer data(8bytes).
         *
         *  Header data includes magic number(2bytes), and data byte size(4bytes)
         *  except header and footer.
         *  Footer data includes magic number(2bytes), sequence number(4bytes).
         *
         *                dat[0] dat[1] dat[2]    dat[3]   dat[4]     dat[5]     dat[6]    dat[7]
         *  Header        0xe7   0xe7   reserved  reserved siz(24:31) siz(16:23) siz(8:15) siz(0:7)
         *  Event data1
         *  ...
         *  Event dataN
         *  Footer        0xcc   0xcc   reserved  reserved seq(24:31) seq(16:23) seq(8:15) seq(0:7)
         */

    virtual int set_header(unsigned char *header, unsigned int data_byte_size)
    {
        header[0] = HEADER_MAGIC;
        header[1] = HEADER_MAGIC;
        header[2] = 0;
        header[3] = 0;
        header[4] = (data_byte_size & 0xff000000) >> 24;
        header[5] = (data_byte_size & 0x00ff0000) >> 16;
        header[6] = (data_byte_size & 0x0000ff00) >> 8;
        header[7] = (data_byte_size & 0x000000ff);
        return 0;
    }

    virtual int set_footer(unsigned char *footer)
    {
        footer[0] = FOOTER_MAGIC;
        footer[1] = FOOTER_MAGIC;
        footer[2] = 0;
        footer[3] = 0;
        footer[4] = (m_loop & 0xff000000) >> 24;
        footer[5] = (m_loop & 0x00ff0000) >> 16;
        footer[6] = (m_loop & 0x0000ff00) >> 8;
        footer[7] = (m_loop & 0x000000ff);
        return 0;
    }

    bool check_header(unsigned char *header, unsigned int received_byte)
    {
        bool ret = false;

        if (header[0] == HEADER_MAGIC && header[1] == HEADER_MAGIC)
        {
            unsigned int event_size = (header[4] << 24) + (header[5] << 16) + (header[6] << 8) + header[7];
            if (received_byte == event_size)
            {
                ret = true;
            }
            else
            {
                cerr << "### ERROR: Event byte size missmatch" << '\n';
                cerr << "event_size in header: " << event_size
                     << "  received data size: " << received_byte << '\n';
            }
        }
        else
        {
            cerr << "### ERROR: Bad Magic Num:"
                 << hex << (unsigned)header[0] << " " << (unsigned)header[1]
                 << '\n';
        }
        cerr << dec;
        return ret;
    }

    bool check_footer(unsigned char *footer)
    {
        bool ret = false;
        if (footer[0] == FOOTER_MAGIC && footer[1] == FOOTER_MAGIC)
        {
            unsigned int seq_num = (footer[4] << 24) + (footer[5] << 16) + (footer[6] << 8) + footer[7];
            if (seq_num == m_loop)
            {
                ret = true;
            }
            else
            {
                cerr << "### ERROR: Sequence No. missmatch" << '\n';
                cerr << "sequece no. in footer :" << seq_num << '\n';
                cerr << "loop cnts at component:" << m_loop << '\n';
            }
        }
        return ret;
    }

    bool check_header_footer(const RTC::TimedOctetSeq &in_data, unsigned int block_byte_size)
    {
        unsigned int event_byte_size =
            block_byte_size - HEADER_BYTE_SIZE - FOOTER_BYTE_SIZE;
        if (m_debug)
        {
            cerr << "event_byte_size: " << event_byte_size << '\n';
        }

        ////////// Check Header and Footer in received data /////////
        unsigned char header[HEADER_BYTE_SIZE];
        unsigned char footer[FOOTER_BYTE_SIZE];

        for (unsigned int i = 0; i < HEADER_BYTE_SIZE; i++)
        {
            header[i] = in_data.data[i];
        }
        if (check_header(header, event_byte_size) == false)
        {
            cerr << "### ERROR: header invalid in loop" << m_loop
                 << '\n';
            fatal_error_report(FatalType::HEADER_DATA_MISMATCH);
        }

        for (unsigned int i = 0; i < FOOTER_BYTE_SIZE; i++)
        {
            footer[i] = in_data.data[block_byte_size - FOOTER_BYTE_SIZE + i];
        }
        if (check_footer(footer) == false)
        {
            cerr << "### ERROR: footer invalid" << '\n';
            fatal_error_report(FatalType::FOOTER_DATA_MISMATCH);
        }
        return true;
    }

    unsigned int get_event_size(unsigned int block_byte_size)
    {
        return (block_byte_size - HEADER_BYTE_SIZE - FOOTER_BYTE_SIZE);
    }

    int init_command_port()
    {
        // Set service provider to Ports
        m_DAQServicePort.registerProvider("daq_svc", "DAQService", m_daq_service0);
        // Set CORBA Service Ports
        registerPort(m_DAQServicePort);
        return 0;
    }

    int set_comp_name(const char *name)
    {
        m_comp_name = name;
        return 0;
    }

    int set_run_number(unsigned int runNumber)
    {
        m_runNumber = runNumber;
        return 0;
    }

    int set_run_number()
    {
        cerr << "set_run_number" << '\n';
        m_runNumber = m_daq_service0.getRunNo();
        cerr << "m_runNumber: " << m_runNumber << '\n';
        return 0;
    }

    unsigned int get_run_number()
    {
        return m_runNumber;
    }

    int set_event_byte_size(unsigned int eventByteSize)
    {
        m_eventByteSize = eventByteSize;
        return 0;
    }

    int inc_sequence_num()
    {
        m_loop++;
        return 0;
    }

    int reset_sequence_num()
    {
        m_loop = 0;
        return 0;
    }

    unsigned long long get_sequence_num()
    {
        return m_loop;
    }

    int inc_total_data_size(unsigned int byteSize)
    {
        m_totalDataSize += byteSize;
        return 0;
    }

    int reset_total_data_size()
    {
        m_totalDataSize = 0;
        return 0;
    }

    unsigned long long get_total_byte_size()
    {
        return m_totalDataSize;
    }

    int inc_total_event_num(unsigned int eventNum)
    {
        m_totalEventNum += eventNum;
        return 0;
    }

    int reset_total_event_num()
    {
        m_totalEventNum = 0;
        return 0;
    }

    unsigned long long get_total_event_num()
    {
        return m_totalEventNum;
    }

    bool check_trans_lock()
    {
        return m_trans_lock;
    }

    void set_trans_lock()
    {
        m_trans_lock = true;
    }

    void set_trans_unlock()
    {
        m_trans_lock = false;
    }

    virtual int daq_dummy() = 0;
    virtual int daq_configure() = 0;
    virtual int daq_unconfigure() = 0;
    virtual int daq_start() = 0;
    virtual int daq_run() = 0;
    virtual int daq_stop() = 0;
    virtual int daq_pause() = 0;
    virtual int daq_resume() = 0;

    virtual int parse_params(::NVList *list) = 0;

    void fatal_error_report(FatalType::Enum type, int code = -1)
    {
        m_isOnError = true;
        set_status(COMP_FATAL);
        throw DaqCompDefinedException(type, code);
    }

    void fatal_error_report(FatalType::Enum type, const char *desc, int code = -1)
    {
        m_isOnError = true;
        set_status(COMP_FATAL);
        throw DaqCompUserException(type, desc, code);
    }

    void init_state_table()
    {
        m_daq_trans_func[CMD_CONFIGURE] = &DAQMW::DaqComponentBase::daq_base_configure;
        m_daq_trans_func[CMD_START] = &DAQMW::DaqComponentBase::daq_base_start;
        m_daq_trans_func[CMD_PAUSE] = &DAQMW::DaqComponentBase::daq_pause;
        m_daq_trans_func[CMD_RESUME] = &DAQMW::DaqComponentBase::daq_resume;
        m_daq_trans_func[CMD_STOP] = &DAQMW::DaqComponentBase::daq_base_stop;
        m_daq_trans_func[CMD_UNCONFIGURE] = &DAQMW::DaqComponentBase::daq_base_unconfigure;

        m_daq_do_func[LOADED] = &DAQMW::DaqComponentBase::daq_base_dummy;
        m_daq_do_func[CONFIGURED] = &DAQMW::DaqComponentBase::daq_base_dummy;
        m_daq_do_func[RUNNING] = &DAQMW::DaqComponentBase::daq_run;
        m_daq_do_func[PAUSED] = &DAQMW::DaqComponentBase::daq_base_dummy;
    }

    int reset_timer()
    {
        status_timer->resetTimer();
        return 0;
    }

    int reset_onError()
    {
        m_isOnError = false;
        return 0;
    }

    /**
         * Convert OutPort RTC::DataPortStatus to DAQMW::BufferStatus
         * We use RTC::RingBuffer with the following condition:
         *   buffer.write.full_policy: block with timeout
         *   buffer.read.empty_policy: block with timeout
         * OutPort::write() returns:
         *   PORT_OK, PORT_ERROR, SEND_FULL, SEND_TIMEOUT, UNKNOWN_ERROR,
         *   PRECONDITION_NOT_MET, CONNECTION_LOST
         * In this case, we get a DataPortStatus such as PORT_OK, SEND_TIMEOUT,
         * PRECONDITION_NOT_MET, CONNECTION_LOST and UNKNOWN_ERROR.
         */
    BufferStatus check_outPort_status(RTC::OutPort<RTC::TimedOctetSeq> &myOutPort)
    {
        BufferStatus ret = BUF_SUCCESS;
        int index = 0;
        RTC::DataPortStatus::Enum out_status = myOutPort.getStatus(index);
        if (m_debug)
        {
            cerr << "OutPort status: "
                 << RTC::DataPortStatus::toString(out_status)
                 << '\n';
        }
        switch (out_status)
        {
        case RTC::DataPortStatus::PORT_OK:
            ret = BUF_SUCCESS;
            break;
        case RTC::DataPortStatus::SEND_TIMEOUT:
            ret = BUF_TIMEOUT;
            break;
        case RTC::DataPortStatus::SEND_FULL:
            ret = BUF_NOBUF;
            break;
        case RTC::DataPortStatus::PORT_ERROR:
        case RTC::DataPortStatus::UNKNOWN_ERROR:
        case RTC::DataPortStatus::PRECONDITION_NOT_MET:
        case RTC::DataPortStatus::CONNECTION_LOST:
            cerr << "OutPort status: "
                 << RTC::DataPortStatus::toString(out_status)
                 << '\n';
            ret = BUF_FATAL;
            break;
            /*** Could never happen in this case ***/
        case RTC::DataPortStatus::BUFFER_EMPTY:
        case RTC::DataPortStatus::RECV_TIMEOUT:
        case RTC::DataPortStatus::BUFFER_TIMEOUT:
        case RTC::DataPortStatus::BUFFER_FULL:
        case RTC::DataPortStatus::RECV_EMPTY:
        case RTC::DataPortStatus::BUFFER_ERROR:
        case RTC::DataPortStatus::INVALID_ARGS:
            cerr << "Impossible OutPort status: "
                 << RTC::DataPortStatus::toString(out_status)
                 << '\n';
            ret = BUF_FATAL;
            break;
        }
        return ret;
    }

    /**
         * Convert InPort RTC::DataPortStatus to DAQMW::BufferStatus
         * We use RTC::RingBuffer with the following condition:
         *   buffer.write.full_policy: block with timeout
         *   buffer.read.empty_policy: block with timeout
         * InPort::read returns:
         *   PORT_OK, BUFFER_EMPTY, BUFFER_TIMEOUT, PORT_ERROR, PRECONDITION_NOT_MET
         * In this case, we get a DataPortStatus such as PORT_OK, BUFFER_TIMEOUT.
         */
    BufferStatus check_inPort_status(RTC::InPort<RTC::TimedOctetSeq> &myInPort)
    {
        BufferStatus ret = BUF_SUCCESS;
        int index = 0;
        RTC::DataPortStatus::Enum in_status = myInPort.getStatus(index);
        if (m_debug)
        {
            cerr << "InPort status: "
                 << RTC::DataPortStatus::toString(in_status)
                 << '\n';
        }
        switch (in_status)
        {
        case RTC::DataPortStatus::PORT_OK:
            ret = BUF_SUCCESS;
            break;
        case RTC::DataPortStatus::BUFFER_TIMEOUT:
            ret = BUF_TIMEOUT;
            break;
        case RTC::DataPortStatus::BUFFER_EMPTY:
            ret = BUF_NODATA;
            break;
        case RTC::DataPortStatus::PORT_ERROR:
        case RTC::DataPortStatus::PRECONDITION_NOT_MET:
            cerr << "InPort status: "
                 << RTC::DataPortStatus::toString(in_status)
                 << '\n';
            ret = BUF_FATAL;
            break;
        /*** Could never happen in this case ***/
        case RTC::DataPortStatus::BUFFER_FULL:
        case RTC::DataPortStatus::RECV_TIMEOUT:
        case RTC::DataPortStatus::SEND_TIMEOUT:
        case RTC::DataPortStatus::SEND_FULL:
        case RTC::DataPortStatus::RECV_EMPTY:
        case RTC::DataPortStatus::BUFFER_ERROR:
        case RTC::DataPortStatus::INVALID_ARGS:
        case RTC::DataPortStatus::CONNECTION_LOST:
        case RTC::DataPortStatus::UNKNOWN_ERROR:
            cerr << "Impossible InPort status: "
                 << RTC::DataPortStatus::toString(in_status)
                 << '\n';
            ret = BUF_FATAL;
            break;
        }
        return ret;
    }

    bool check_dataPort_connections(RTC::OutPort<RTC::TimedOctetSeq> &myOutPort)
    {
        coil::vstring conn_list = myOutPort.getConnectorIds();
        bool ret = false;
        if (conn_list.size() == 1)
        {
            ret = true;
        }
        return ret;
    }

    bool check_dataPort_connections(RTC::InPort<RTC::TimedOctetSeq> &myInPort)
    {
        coil::vstring conn_list = myInPort.getConnectorIds();
        bool ret = false;
        if (conn_list.size() == 1)
        {
            ret = true;
        }
        return ret;
    }

    int daq_do()
    {
        int ret = 0;
        bool status = true;

        get_command();

        if (m_command != CMD_NOP)
        {                                  // got other command
            status = set_state(m_command); // set next state

            if (status)
            {
                while (check_trans_lock())
                { // check if transition is locked
                    if (m_debug)
                    {
                        cerr << "### trans locked" << '\n';
                    }
                    usleep(0);
                    if (!m_isOnError)
                    {
                        try
                        {
                            doAction(m_state_prev); // daq_base_XX{}
                        }
                        catch (DaqCompDefinedException &e)
                        {
                            cerr << status_timer->getDate() << " ";
                            FatalType::Enum mytype = e.type();
                            int mycode = e.reason();
                            const char *mydesc = e.what();
                            fatal_report_to_operator(mytype, mydesc, mycode);
                        }
                        catch (DaqCompUserException &e)
                        {
                            cerr << status_timer->getDate() << " ";
                            FatalType::Enum mytype = e.type();
                            int mycode = e.reason();
                            const char *mydesc = e.what();
                            fatal_report_to_operator(mytype, mydesc, mycode);
                        }
                        catch (...)
                        {
                            cerr << "### got unknown exception at transition\n";
                        }
                    }
                    else
                    {
                        daq_onError();
                    }
                } // while

                ///valid command, do transition
                try
                {
                    ret = transAction(m_command);
                }
                catch (DaqCompDefinedException &e)
                {
                    cerr << status_timer->getDate() << " ";
                    FatalType::Enum mytype = e.type();
                    int mycode = e.reason();
                    const char *mydesc = e.what();
                    fatal_report_to_operator(mytype, mydesc, mycode);
                }
                catch (DaqCompUserException &e)
                {
                    cerr << status_timer->getDate() << " ";
                    FatalType::Enum mytype = e.type();
                    int mycode = e.reason();
                    const char *mydesc = e.what();
                    fatal_report_to_operator(mytype, mydesc, mycode);
                }
                catch (...)
                {
                    cerr << "### got unknown exception at transition\n";
                }
                set_status(COMP_WORKING);
            }
            else
            {
                cerr << "daq_do: transAction call: illegal command"
                     << '\n';
            }
            set_done();

            if (m_time)
            {
                get_time_inline(m_command);
                get_time_output(m_command);
            }
        }
        else
        {
            ///same command as previous, stay same state, do same action
            if (!m_isOnError)
            {
                try
                {
                    doAction(m_state);
                }
                catch (DaqCompDefinedException &e)
                {
                    cerr << status_timer->getDate() << " ";
                    FatalType::Enum mytype = e.type();
                    int mycode = e.reason();
                    const char *mydesc = e.what();
                    fatal_report_to_operator(mytype, mydesc, mycode);
                }
                catch (DaqCompUserException &e)
                {
                    cerr << status_timer->getDate() << " ";
                    FatalType::Enum mytype = e.type();
                    int mycode = e.reason();
                    const char *mydesc = e.what();
                    fatal_report_to_operator(mytype, mydesc, mycode);
                }
                catch (...)
                {
                    cerr << "### caught unknown exception on DaqComponentBase\n";
                    return ret;
                }
                clockwork_status_report();
            }
            else
            {
                daq_onError();
            }
            get_hb_from_operator_clockwork();

            // cerr << m_command <<'\n';
            // cerr << m_state_prev << '\n';
            // cerr << m_state <<'\n';
        }

        return ret;
    } /// daq_do()

    int set_debug_on()
    {
        m_debug = true;
        return 0;
    }

    int set_debug_off()
    {
        m_debug = false;
        return 0;
    }

    int set_status(CompStatus comp_status)
    {
        unique_ptr<Status> mystatus(new Status);
        mystatus->comp_name = CORBA::string_dup(m_comp_name.c_str());
        mystatus->state = m_state;
        ///mystatus->event_num = m_totalEventNum;
        mystatus->event_size = m_totalDataSize;
        mystatus->comp_status = comp_status;

        m_daq_service0.setStatus(*mystatus);

        return 0;
    }

  private:
    static constexpr int DAQ_CMD_SIZE = 12;
    static constexpr int DAQ_STATE_SIZE = 6;
    static constexpr int DAQ_IDLE_TIME_USEC = 10000; // 10 m sec
    static constexpr int STATUS_CYCLE_SEC = 3;       // default = 3
    static constexpr int CHECK_HB_CYCLE_SEC = 4;     // default = 3
    // static const int DAQ_HB_SIZE            =  5;

    string m_comp_name;
    unsigned int m_runNumber;
    unsigned int m_eventByteSize;
    unsigned long long m_loop;
    unsigned long long m_totalEventNum;
    unsigned long long m_totalDataSize;

    bool m_trans_lock;

    RTC::CorbaPort m_DAQServicePort;

    // Timer* status_timer;
    unique_ptr<Timer> status_timer{new Timer(STATUS_CYCLE_SEC)};
    unique_ptr<Timer> hb_timer{new Timer(CHECK_HB_CYCLE_SEC)};

    DAQCommand m_command;
    DAQLifeCycleState m_state;
    DAQLifeCycleState m_state_prev;

    string m_err_message;

    bool m_isOnError;
    bool m_isTimerAlarm;
    bool m_has_printed_error_log;

    bool m_debug;
    bool m_time;

    typedef int (DAQMW::DaqComponentBase::*DAQFunc)();

    DAQFunc m_daq_trans_func[DAQ_CMD_SIZE];
    DAQFunc m_daq_do_func[DAQ_STATE_SIZE];
    // DAQFunc m_daq_hb_func[DAQ_HB_SIZE];

    int transAction(int command)
    {
        return (this->*m_daq_trans_func[command])();
    }

    void doAction(int state)
    {
        (this->*m_daq_do_func[state])();
    }

    int daq_base_dummy()
    {
        daq_dummy();
        set_status(COMP_WORKING);
        usleep(DAQ_IDLE_TIME_USEC);
        return 0;
    }

    int daq_base_configure()
    {
        set_status(COMP_WORKING);
        daq_configure();
        return 0;
    }

    int daq_base_unconfigure()
    {
        m_totalDataSize = 0;
        if (m_isOnError)
        {
            reset_onError(); /// reset error flag
        }
        set_status(COMP_WORKING);
        daq_unconfigure();
        return 0;
    }

    int daq_base_start()
    {
        m_totalDataSize = 0;
        m_loop = 0;
        set_run_number();
        set_status(COMP_WORKING);
        m_has_printed_error_log = false;
        daq_start();
        return 0;
    }

    int daq_base_stop()
    {
        if (m_isOnError)
        {
            reset_onError(); /// reset error flag
        }

        m_err_message = "";
        set_status(COMP_WORKING);
        daq_stop();

        cerr << "event byte size = " << m_totalDataSize << '\n';
        return 0;
    }

    int daq_base_pause()
    {
        set_status(COMP_WORKING);
        daq_pause();
        return 0;
    }

    int get_command()
    {
        m_command = m_daq_service0.getCommand();
        if (m_debug)
        {
            cerr << "m_command=" << m_command << '\n';
        }
        return 0;
    }

    int get_hb_from_operator_clockwork()
    {
        if (hb_timer->checkTimer())
        {
            m_daq_service0.setHB();
            hb_timer->resetTimer();
        }
        return 0;
    }
    int get_time_inline(int command)
    {
        constexpr char fname_inline[] = "s4i-file-inline";

        TimeVal st;
        struct timeval end_time;
        struct timezone tz;
        long result;

        struct passwd *pw;
        uid_t uid;

        char date[128];
        char fname[128];

        // end time
        gettimeofday(&end_time, &tz);

        // start time
        st = m_daq_service0.getTime();

        // calc
        result = (end_time.tv_sec - st.sec) * 1000000 + (end_time.tv_usec - st.usec);

        if (result < 0)
        {
            result = (st.sec - end_time.tv_sec) * 1000000 + (st.usec - end_time.tv_usec);
        }

        uid = getuid();
        if ((pw = getpwuid(uid)))
        {
            sprintf(date, "/home/%s/DAQ-Middleware/csv/s4i/%s", pw->pw_name, fname_inline);
        }
        sprintf(fname, "%s.csv", date);
        ofstream csv_file(fname, ios::app);

        switch (command)
        {
        case CMD_CONFIGURE:
            csv_file << "Configure," << result << '\n';
            break;
        case CMD_START:
            csv_file << "Start," << result << '\n';
            break;
        case CMD_STOP:
            csv_file << "Stop," << result << '\n';
            break;
        case CMD_UNCONFIGURE:
            csv_file << "Unconfigure," << result << '\n';
            break;
        case CMD_PAUSE:
            csv_file << "Pause," << result << '\n';
            break;
        case CMD_RESUME:
            csv_file << "Resume," << result << '\n';
            break;
        case CMD_RESTART:
            csv_file << "Restart," << result << '\n';
            break;
        }
        csv_file.close();

        return 0;
    }
    int get_time_output(int command)
    {
        string fname_output = "s4i-output";

        struct timeval end_time;
        struct timezone tz;

        struct passwd *pw;
        uid_t uid;

        char date[128];
        char fname[128];

        long gt[2];

        /* end time */
        gettimeofday(&end_time, &tz);
        gt[0] = end_time.tv_sec;
        gt[1] = end_time.tv_usec;

        uid = getuid();
        if ((pw = getpwuid(uid)))
        {
            sprintf(date, "/home/%s/DAQ-Middleware/csv/s4i/%s", pw->pw_name, fname_output.c_str());
        }
        sprintf(fname, "%s.csv", date);
        ofstream csv_file(fname, ios::app);
        switch (command)
        {
        case CMD_CONFIGURE:
            csv_file << "et,Configure," << gt[0] << ',' << gt[1] << '\n';
            break;
        case CMD_START:
            csv_file << "et,Start," << gt[0] << ',' << gt[1] << '\n';
            break;
        case CMD_STOP:
            csv_file << "et,Stop," << gt[0] << ',' << gt[1] << '\n';
            break;
        case CMD_UNCONFIGURE:
            csv_file << "et,Unconfigure," << gt[0] << ',' << gt[1] << '\n';
            break;
        case CMD_PAUSE:
            csv_file << "et,Pause," << gt[0] << ',' << gt[1] << '\n';
            break;
        case CMD_RESUME:
            csv_file << "et,Resume," << gt[0] << ',' << gt[1] << '\n';
            break;
        case CMD_RESTART:
            csv_file << "et,Restart," << gt[0] << ',' << gt[1] << '\n';
            break;
        }
        csv_file.close();
        return 0;
    }

    int set_done()
    {
        m_daq_service0.setDone();
        if (m_debug)
        {
            cerr << "set_done()\n";
        }
        return 0;
    }

    virtual int daq_onError()
    {
        if (check_trans_lock())
        {
            set_trans_unlock();
        }
        if (!m_has_printed_error_log)
        {
            cerr << "### daq_onError(): ERROR Occured\n";
            set_status(COMP_FATAL);
            m_has_printed_error_log = true;
        }
        usleep(DAQ_IDLE_TIME_USEC);
        return 0;
    }

    void fatal_report_to_operator(FatalType::Enum type, const char *desc, int code = -1)
    {
        FatalErrorStatus errStatus;
        errStatus.fatalTypes = type;
        errStatus.errorCode = code;
        errStatus.description = CORBA::string_dup(desc);
        m_daq_service0.setFatalStatus(errStatus);
    }

    int clockwork_status_report()
    {
        if (status_timer->checkTimer())
        {
            m_isTimerAlarm = true;
            set_status(COMP_WORKING);
            status_timer->resetTimer();
        }
        return 0;
    }

    bool set_state(DAQCommand command)
    {
        bool ret = true;
        /// new command has come, chage new state
        switch (command)
        {
        case CMD_CONFIGURE:
            m_state_prev = LOADED;
            m_state = CONFIGURED;
            break;
        case CMD_START:
            m_state_prev = CONFIGURED;
            m_state = RUNNING;
            break;
        case CMD_PAUSE:
            m_state_prev = RUNNING;
            m_state = PAUSED;
            set_trans_lock();
            break;
        case CMD_RESUME:
            m_state_prev = PAUSED;
            m_state = RUNNING;
            break;
        case CMD_STOP:
            m_state_prev = RUNNING;
            m_state = CONFIGURED;
            set_trans_lock();
            break;
        case CMD_UNCONFIGURE:
            m_state_prev = RUNNING;
            m_state = LOADED;
            break;
        default:
            //status = false;
            ret = false;
            break;
        }
        return ret;
    }
}; /// class
} // namespace DAQMW

#endif

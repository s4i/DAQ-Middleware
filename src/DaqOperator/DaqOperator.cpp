// -*- C++ -*-
/*!
 * @file DaqOperator.cpp
 * @brief Run controller and user interface component.
 * @date 1-January-2008
 * @author Kazuo Nakayoshi <kazuo.nakayoshi@kek.jp>
 *
 * Copyright (C) 2008-2011
 *     Kazuo Nakayoshi
 *     High Energy Accelerator Research Organization (KEK), Japan.
 *     All rights reserved.
 */

#include "DaqOperator.h"

static const char *daqserviceconsumer_spec[] = {
	"implementation_id", "DaqOperator",
	"type_name", "DaqOperator",
	"description", "Controller of DAQ components",
	"version", "0.1",
	"vendor", "KEK",
	"category", "Generic",
	"activity_type", "DataFlowComponent",
	"max_instance", "10",
	"language", "C++",
	"lang_type", "compile",
	""};

DAQMW::ParameterServer *g_server = nullptr;

#include "callback.h"

// error message for dom
static const char *FORMAT_IO_ERR_E = "[%s] No such file or directory.";
static const char *FORMAT_IO_ERR_J = "[%s] No such file or directory.";
static const char *FORMAT_REQ_INV_IN_STS_E = "Invalid request in status %s.";
static const char *FORMAT_REQ_INV_IN_STS_J = "Invalid request in status %s.";

DaqOperator *DaqOperator::_instance = 0;

DaqOperator *DaqOperator::Instance()
{
	return _instance;
}
DaqOperator::DaqOperator(RTC::Manager *manager)
	: RTC::DataFlowComponentBase(manager),
	  m_comp_num(0),
	  m_service_num(0),
	  deadFlag(false),
	  resFlag(false),
	  m_new(0),
	  m_state(LOADED),
	  m_runNumber(0),
	  m_start_date(" "),
	  m_stop_date(" "),
	  m_param_port(PARAM_PORT),
	  m_com_completed(true),
	  m_isConsoleMode(true),
	  m_msg(" "),
	  m_err_msg(" "),
	  m_debug(false),
	  m_time(false)
{
	if (m_debug)
	{
		cerr << "Create DaqOperator\n";
	}
	try
	{
		XMLPlatformUtils::Initialize(); //Initialize Xerces
	}
	catch (XMLException &e)
	{
		char *message = XMLString::transcode(e.getMessage());
		cerr << "### ERROR: XML toolkit initialization error: "
			 << message << '\n';
		XMLString::release(&message);
		// throw exception here to return ERROR_XERCES_INIT
	}

	ParamList paramList;
	ConfFileParser MyParser;

	m_conf_file = getConfFilePath();
	m_comp_num = MyParser.readConfFile(m_conf_file.c_str(), false);
	if (m_debug)
	{
		cerr << "Conf file:" << m_conf_file << '\n';
		cerr << "comp num = " << m_comp_num << '\n';
	}

	/// create CorbaConsumer for the number of components
	for (int i = 0; i < m_comp_num; i++)
	{
		RTC::CorbaConsumer<DAQService> daqservice;
		m_daqservices.emplace_back(daqservice);
	}
	if (m_debug)
	{
		cerr << "*** m_daqservices.size():" << m_daqservices.size() << '\n';
	}

	/// create CorbaPort for the number of components
	for (int i = 0; i < m_comp_num; i++)
	{
		stringstream strstream;
		strstream << m_service_num++;
		string service_name = "service" + strstream.str();
		if (m_debug)
		{
			cerr << "service name: " << service_name << '\n';
		}
		m_DaqServicePorts.emplace_back(new RTC::CorbaPort(service_name.c_str()));
	}
	/// register CorbaPort
	for (int i = 0; i < m_comp_num; i++)
	{
		m_DaqServicePorts[i]->registerConsumer("daq_svc", "DAQService", m_daqservices[i]);

		registerPort(*m_DaqServicePorts[i]);

		if (m_debug)
		{
			cerr << "m_daqservices.size() = "
				 << m_daqservices.size() << '\n';
			cerr << "m_DaqServicePorts.size() = "
				 << m_DaqServicePorts.size() << '\n';
		}
	}

	FD_ZERO(&m_allset);
	FD_ZERO(&m_rset);

	m_tout.tv_sec = 3;
	m_tout.tv_usec = 0;
}
DaqOperator::~DaqOperator()
{
	XMLPlatformUtils::Terminate();
}
RTC::ReturnCode_t DaqOperator::onInitialize()
{
	if (m_debug)
	{
		cerr << "**** DaqOperator::onInitialize()\n";
	}
	_instance = this;

	return RTC::RTC_OK;
}
RTC::ReturnCode_t DaqOperator::onStartup(RTC::UniqueId ec_id)
{
	if (m_debug)
		cerr << "\n**** DaqOperator::onStartup()\n";
	return RTC::RTC_OK;
}
RTC::ReturnCode_t DaqOperator::onActivated(RTC::UniqueId ec_id)
{
	if (m_debug)
		cerr << "\n**** DaqOperator::onActivated()\n";
	return RTC::RTC_OK;
}
RTC::ReturnCode_t DaqOperator::onExecute(RTC::UniqueId ec_id)
{
	RTC::ReturnCode_t ret = RTC::RTC_OK;

	clockwork_hb_recv();

	if (m_isConsoleMode == true)
		ret = run_console_mode();
	else
		ret = run_http_mode();

	return ret;
}
RTC::ReturnCode_t DaqOperator::run_http_mode()
{
	if (g_server == nullptr)
	{
		cerr << "m_param_port:" << m_param_port << '\n';
		g_server = new DAQMW::ParameterServer(m_param_port);
		cerr << "ParameterServer starts..." << '\n';

		g_server->bind("put:Params", &m_body, cb_command_configure);
		g_server->bind("put:ResetParams", &m_body, cb_command_unconfigure);
		//g_server->bind("put:Reset", &m_body, cb_command_unconfigure);
		g_server->bind("put:Begin", &m_body, cb_command_start);
		g_server->bind("put:End", &m_body, cb_command_stop);
		//g_server->bind("put:Abort", &m_body, cb_command_abort);
		g_server->bind("put:ConfirmEnd", &m_body, cb_command_confirmend);
		//g_server->bind("get:Params", &m_body, cb_command_putparams);
		g_server->bind("get:Status", &m_body, cb_command_putstatus);
		g_server->bind("get:Log", &m_body, cb_command_log);
		g_server->bind("put:Pause", &m_body, cb_command_pause);
		g_server->bind("put:Restart", &m_body, cb_command_resume);
		g_server->bind("put:StopParamsSet", &m_body, cb_command_stopparamsset);
		g_server->bind("put:Save", &m_body, cb_command_save);
		g_server->bind("put:ConfirmConnection", &m_body,
					   cb_command_confirmconnection);
		g_server->bind("put:dummy", &m_body, cb_command_dummy);
		if (m_debug)
		{
			cerr << "*** bind callback functions done\n";
			cerr << "*** Ready to accept a command\n";
		}
	}

	g_server->Run();
	run_data();

	return RTC::RTC_OK;
}
string DaqOperator::check_state(DAQLifeCycleState compState)
{
	string comp_state = "";
	switch (compState)
	{
	case LOADED:
		comp_state = "LOADED";
		break;
	case CONFIGURED:
		comp_state = "CONFIGURED";
		break;
	case RUNNING:
		comp_state = "RUNNING";
		break;
	case PAUSED:
		comp_state = "PAUSED";
		break;
	case ERRORED:
		break;
	}
	return comp_state;
}
string DaqOperator::check_compStatus(CompStatus compStatus)
{
	string comp_status = "";
	switch (compStatus)
	{
	case COMP_WORKING:
		comp_status = "WORKING";
		break;
	case COMP_FINISHED:
		comp_status = "FINISHED";
		break;
	case COMP_WARNING:
		comp_status = "WORNING";
		break;
	case COMP_FATAL:
		comp_status = "ERROR"; //"ERROR";
		break;
	case COMP_RESTART:
		comp_status = "Pls set command:6";
		break;
	}
	return comp_status;
}
void DaqOperator::run_data()
{
	cerr << "\033[;H\033[2J";

	try
	{
		for (int i = 0; i < m_comp_num; i++)
		{
			Status_var status;
			status = m_daqservices[i]->getStatus();

			if (status->comp_status == COMP_FATAL)
			{
				RTC::ConnectorProfileList_var myprof =
					m_DaqServicePorts[i]->get_connector_profiles();
				cerr << myprof[0].name << " "
					 << "### on ERROR ###  " << '\n';

				FatalErrorStatus_var errStatus;
				errStatus = m_daqservices[i]->getFatalStatus();
				cerr << "\033[1;0H";
				cerr << "errStatus.fatalTypes:"
					 << errStatus->fatalTypes << '\n';
				cerr << "errStatus.errorCode:"
					 << errStatus->errorCode << '\n';
				cerr << "errStatus.description:"
					 << errStatus->description << '\n';
				m_err_msg = errStatus->description;
			} // if fatal
		}
	}
	catch (...)
	{
		cerr << "DaqOperator::run_data() Exception was caught" << '\n';
	}

	cerr << "\033[0;0H";
	cerr << "RUN#" << m_runNumber
		 << " start at: " << m_start_date
		 << " stop at: " << m_stop_date << '\n';
}
RTC::ReturnCode_t DaqOperator::run_console_mode()
{
	int command;
	string srunNo = "0";
	/* console error display */
	vector<string> d_compname;
	vector<FatalErrorStatus_var> d_message;

	m_tout.tv_sec = 2;
	m_tout.tv_usec = 0;

	FD_ZERO(&m_rset);
	FD_SET(0, &m_rset);

	cerr << "\033[0;0H"
		 << " Command:\t" << '\n';
	cerr << " "
		 << CMD_CONFIGURE << ":configure\t"
		 << CMD_START << ":start\t   "
		 << CMD_STOP << ":stop\t"
		 << CMD_UNCONFIGURE << ":unconfigure" << '\n'
		 << " "
		 << CMD_PAUSE << ":pause\t"
		 << CMD_RESUME << ":resume   "
		 << CMD_RESTART << ":restart"
		 << '\n';

	cerr << '\n'
		 << " RUN NO: " << m_runNumber;
	cerr << '\n'
		 << " start at: " << m_start_date
		 << " stop at: " << m_stop_date << "\n\n";
	cerr << "\033[0;11H";

	select(1, &m_rset, NULL, NULL, &m_tout);
	if (m_com_completed == false)
	{
		return RTC::RTC_OK;
	}

	// Time
	if (m_time)
	{
		state_change_automation();
	}

	if (FD_ISSET(0, &m_rset))
	{
		char comm[2];
		if (read(0, comm, sizeof(comm)) == -1)
		{ //read(0:stdin))
			return RTC::RTC_OK;
		}
		command = (int)(comm[0] - '0');

		// set time1
		if (m_time)
		{
			set_time();
			output_performance(command);
		}

		cerr << "\033[0;10H";
		switch (m_state)
		{ // m_state init (LOADED)
		case PAUSED:
			switch ((DAQCommand)command)
			{
			case CMD_RESUME:
				resume_procedure(); ///
				m_state = RUNNING;
				break;
			default:
				cerr << "   Bad Command:" << command << '\n';
				break;
			}
			break;
		case LOADED:
			switch ((DAQCommand)command)
			{
			case CMD_CONFIGURE:
				configure_procedure();
				m_state = CONFIGURED;
				break;
			default:
				cerr << "   Bad Command\n";
				break;
			}
			break;
		case CONFIGURED:
			switch ((DAQCommand)command)
			{
			case CMD_START:
				cerr << "\033[5;20H"; // default=3;20H
				cerr << "input RUN NO(same run no is prohibited):   ";
				cerr << "\033[5;62H";
				cin >> srunNo;

				// set time2
				if (m_time)
				{
					set_time();
					output_performance(command);
				}

				m_runNumber = atoi(srunNo.c_str());
				start_procedure();
				m_state = RUNNING;
				break;
			case CMD_UNCONFIGURE:
				unconfigure_procedure();
				m_state = LOADED;
				break;
			default:
				cerr << "   Bad Command\n";
				break;
			}
			break;
		case RUNNING:
			switch ((DAQCommand)command)
			{
			case CMD_STOP:
				stop_procedure();
				m_state = CONFIGURED;
				break;
			case CMD_PAUSE:		   ///
				pause_procedure(); ///
				m_state = PAUSED;
				break;
			default:
				cerr << "   Bad Command: ";
				cerr << command << '\n';
				break;
			}
			break;
		case ERRORED:
			switch ((DAQCommand)command)
			{
			case CMD_STOP:
				stop_procedure();
				m_state = CONFIGURED; ///
				break;
			case CMD_RESTART:
				error_stop_procedure();
				sleep(2);
				other_stop_procedure();
				sleep(1);
				cerr << "\033[5;20H"; // default:3;20H
				cerr << "input RUN NO(same run no is prohibited):   ";
				cerr << "\033[5;62H";
				cin >> srunNo;
				m_runNumber = atoi(srunNo.c_str());
				start_procedure();
				cerr << "\033[0;13H"
					 << "\033[34m"
					 << "Send reboot command"
					 << "\033[39m" << '\n';
				m_state = RUNNING;
				break;
			default:
				break;
			}
			break;
		} // switch (m_state)
	}	 // if
	else
	{
		// Console memu
		Status_var status;
		FatalErrorStatus_var errStatus;

		cerr << " " << '\n';
		cerr << "\033[0;0H\033[2J";
		cerr << "\033[8;0H";
		cerr << setw(16) << right << "GROUP:COMP_NAME"
			 << setw(22) << right << "EVENT_SIZE"
			 << setw(12) << right << "STATE"
			 << setw(14) << right << "COMP_STATUS"
			 << '\n';
		///cerr << "RUN NO: " << m_runNumber << '\n';

		string compname;
		for (int i = (m_comp_num - 1); i >= 0; i--)
		{
			try
			{
				// copy_compname();

				RTC::ConnectorProfileList_var myprof =
					m_DaqServicePorts[i]->get_connector_profiles();
				compname = myprof[0].name;

				status = m_daqservices[i]->getStatus();
				cerr << " " << setw(22) << left
					 << compname
					 << '\t'
					 << setw(14) << right
					 << status->event_size; // data size(byte)

				if (status->comp_status == COMP_FATAL)
				{
					errStatus = m_daqservices[i]->getFatalStatus();
					cerr << "\033[35m"
						 << setw(12) << right
						 << "RUNNING"
						 << "\033[39m"
						 << "\033[31m" << setw(14) << right
						 << check_compStatus(status->comp_status)
						 << "\033[39m" << '\n';

					/** Use error console display **/
					d_compname.emplace_back(compname);
					d_message.emplace_back(move(errStatus));
					m_state = ERRORED;
				} ///if Fatal
				else if (status->comp_status == COMP_RESTART)
				{
					errStatus = m_daqservices[i]->getFatalStatus();
					cerr << "\033[35m"
						 << setw(12) << right
						 << "RUNNING"
						 << "\033[39m"
						 << "\033[33m" << setw(14) << right
						 << check_compStatus(status->comp_status)
						 << "\033[39m" << '\n';

					/** Use error console display **/
					d_compname.emplace_back(compname);
					d_message.emplace_back(move(errStatus));
					m_state = ERRORED;
					resFlag = true;
				} ///if Restart Request
				else
				{
					cerr << setw(12) << right
						 << check_state(status->state)
						 << "\033[32m"
						 << setw(14) << right
						 << check_compStatus(status->comp_status)
						 << "\033[39m" << '\n';
				}
			}
			catch (...)
			{
				cerr << " ### ERROR: "
					 << setw(22) << right
					 << compname << " : cannot connect\n";
				// m_daqservices[i]->setStopDaqSystem();
			}
		} //for
		cerr << '\n';
		for (auto &da : m_daqservices)
		{
			if (da->getHB())
				cerr << "1";
			else
				cerr << "0";
		}
		cerr << '\n';

		/* Display Error Console */
		if (m_state == ERRORED)
		{
			int cnt = 0;
			for (auto &compname : d_compname)
			{
				++cnt;
				cerr << " [ERROR" << cnt << "] "
					 << compname << '\t'
					 << "\033[31m"
					 << "<- " << d_message[cnt - 1]->description
					 << "\033[39m" << '\n';
			} ///for
			if (deadFlag == true)
			{
				// for (auto& k_d : keep_dead) {
				// 	if (k_d == 1) {
				cerr << "\033[31m"
					 << "No reach Heart beat.\n"
					 << "\033[39m";
				// 	}
				// }
			}
			else if (deadFlag == true && resFlag == true)
			{
				// for (auto& k_a : keep_alive) {
				// 	if (k_a == 1) {
				cerr << "\033[36m"
					 << "Heart beat reacquisition."
					 << "Push command 2:stop or 6:reboot"
					 << "\033[39m" << '\n';
				// 	}
				// }
			}
		}
		else
		{
			resFlag = false;
			deadFlag = false;
		} /// if
	}	 // if

	return RTC::RTC_OK;
}
int DaqOperator::copy_compname()
{
	string compname;
	RTC::ConnectorProfileList_var myprof;

	if (m_new == 0)
	{
		for (int i = 0; i < m_comp_num; i++)
		{
			myprof = m_DaqServicePorts[i]->get_connector_profiles();
			compnames.emplace_back(move(myprof[0].name));
		}
		m_new = 1;
	}
	return 0;
}
bool DaqOperator::parse_body(const char *buf, const string tagname)
{
	XercesDOMParser *parser = new XercesDOMParser;

	MemBufInputSource *memBufIS = new MemBufInputSource((const XMLByte *)buf, strlen(buf), "test", false);

	parser->parse(*memBufIS);
	///XMLCh* name = XMLString::transcode("params");
	XMLCh *name = XMLString::transcode(tagname.c_str());

	DOMDocument *doc = parser->getDocument();
	DOMElement *root = doc->getDocumentElement();
	DOMNodeList *list = root->getElementsByTagName(name);

	DOMElement *ele = (DOMElement *)list->item(0);
	DOMNode *node = ele->getFirstChild();
	char *tag = XMLString::transcode(node->getTextContent());

	if (m_debug)
	{
		cerr << "tagname:" << tagname << '\n';
	}

	if (strlen(tag) != 0 && tagname == "params")
	{
		m_config_file_tmp = tag;
		cerr << "*** m_config_file:" << m_config_file_tmp << '\n';
	}
	if (strlen(tag) != 0 && tagname == "runNo")
	{
		int tag_len = strlen(tag);

		if (tag_len > 6 || tag_len < 1)
		{
			cerr << "DaqOperator: Invalid Run No.\n";
			return false;
		}
		m_runNumber = atoi(tag);

		if (m_debug)
		{
			cerr << "strlen(tag):" << tag_len << '\n';
			cerr << "*** m_runNumber:" << m_runNumber << '\n';
		}
	}

	XMLString::release(&name);
	XMLString::release(&tag);

	delete (memBufIS);
	parser->resetDocumentPool();
	delete (parser);

	return true;
}
int DaqOperator::set_runno(RTC::CorbaConsumer<DAQService> daqservice, unsigned runno)
{
	try
	{
		daqservice->setRunNo(runno);
	}
	catch (...)
	{
		cerr << "*** setRunNumber: failed" << '\n';
	}
	return 0;
}
int DaqOperator::set_command(RTC::CorbaConsumer<DAQService> daqservice,
							 DAQCommand daqcom)
{
	try
	{
		daqservice->setCommand(daqcom);
	}
	catch (...)
	{
		cerr << "### ERROR: set command: exception occured\n ";
	}
	return 0;
}
int DaqOperator::clockwork_hb_recv()
{
	if (mytimer->checkTimer())
	{
		for (auto &daqservice : m_daqservices)
		{
			if (daqservice->getHB())
			{
				if (deadFlag == true)
				{
					deadFlag = false;
					resFlag = true;
				}
				daqservice->reset_send_count();
			}
			else
			{
				if (deadFlag == false)
				{
					if (daqservice->get_send_count() > 10)
					{
						daqservice->reset_send_count();
						deadFlag = true;
					}
				}
				else
				{
					cout << "Dead end\n";
				}
			}
			daqservice->inc_send_count();
		}
		mytimer->resetTimer();
	}
	return 0;
}
int DaqOperator::check_done(RTC::CorbaConsumer<DAQService> daqservice)
{
	try
	{
		if (daqservice->checkDone() == 0)
		{
			usleep(0);
		}
	}
	catch (...)
	{
		cerr << "### checkDone: failed" << '\n';
	}
	return 0;
}
int DaqOperator::set_time()
{
	TimeVal *st = new TimeVal;
	struct timeval start_time;
	struct timezone tz;

	gettimeofday(&start_time, &tz);
	st->sec = start_time.tv_sec;
	st->usec = start_time.tv_usec;

	try
	{
		for (auto &daqservice : m_daqservices)
		{
			try
			{
				daqservice->setTime(*st);
			}
			catch (...)
			{
				cerr << "### ERROR: set time: exception occured\n";
			}
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to set Time.\n";
	}
	return 0;
}

int DaqOperator::error_stop_procedure()
{
	Status_var status;
	m_com_completed = false;

	try
	{
		for (int i = (m_comp_num - 1); i >= 0; i--)
		{
			status = m_daqservices[i]->getStatus();
			if (status->comp_status == COMP_FATAL)
			{ // RESTART
				set_command(m_daqservices[i], CMD_STOP);
				check_done(m_daqservices[i]);
			}
		}
	}
	catch (...)
	{
		cerr << "### ERROR: Failed to restart(stop) Component.\n";
		return 1;
	}

	time_t now = time(0);
	m_stop_date = asctime(localtime(&now));
	m_stop_date[m_stop_date.length() - 1] = ' ';
	m_stop_date.erase(0, 4);

	try
	{
		for (auto &daqservice : m_daqservices)
		{
			status = daqservice->getStatus();
			if (status->state == CONFIGURED)
			{
				set_command(daqservice, CMD_UNCONFIGURE);
				check_done(daqservice);
			}
		}

		ParamList paramList;
		for (int i = 0; i < (int)m_daqservices.size(); i++)
		{
			RTC::ConnectorProfileList_var myprof = m_DaqServicePorts[i]->get_connector_profiles();

			char *id = CORBA::string_dup(myprof[0].name);

			for (int j = 0; j < (int)paramList.size(); j++)
			{
				if (paramList[j].getId() == id)
				{
					int len = paramList[j].getList().length();
					::NVList mylist(len);
					mylist = paramList[j].getList();
					m_daqservices[i]->setCompParams(paramList[j].getList());
				}
			}
			CORBA::string_free(id);
		}

		for (auto &daqservice : m_daqservices)
		{
			status = daqservice->getStatus();
			if (status->state == LOADED)
			{
				set_command(daqservice, CMD_CONFIGURE);
				check_done(daqservice);
			}
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: unconfigure, configure Components.\n";
		return 1;
	}

	m_com_completed = true;
	return 0;
}
int DaqOperator::other_stop_procedure()
{
	m_com_completed = false;
	Status_var status;

	time_t now = time(0);
	m_stop_date = asctime(localtime(&now));
	m_stop_date[m_stop_date.length() - 1] = ' ';
	m_stop_date.erase(0, 4);

	try
	{
		for (auto &daqservice : m_daqservices)
		{
			status = daqservice->getStatus();
			if (status->state == RUNNING)
			{
				set_command(daqservice, CMD_STOP);
				check_done(daqservice);
			}
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to stop Component.\n";
		return 1;
	}

	m_com_completed = true;
	return 0;
}
int DaqOperator::set_service_list()
{

	if (m_debug)
	{
		cerr << "==========================================\n";
		cerr << "\n\n---- service num = " << m_service_num << '\n';
		cerr << "==========================================\n";
	}

	m_daqServiceList.clear();

	for (int i = 0; i < m_service_num; i++)
	{
		RTC::ConnectorProfileList_var myprof;
		myprof = m_DaqServicePorts[i]->get_connector_profiles();
		if (m_debug)
		{
			cerr << " ====> index     :" << i << '\n';
			cerr << " ====> prof name:" << myprof[0].name << '\n';
			string id = (string)myprof[0].name;
			cerr << "====> ID: " << id << '\n';
		}
		struct serviceInfo serviceInfo;
		serviceInfo.comp_id = myprof[0].name;
		serviceInfo.daqService = m_daqservices[i];
		m_daqServiceList.emplace_back(serviceInfo);
	}

	return 0;
}
int DaqOperator::configure_procedure()
{
	if (m_debug)
	{
		cout << "*** configure_procedure: enter" << '\n';
	}
	m_com_completed = false;
	ConfFileParser MyParser;
	ParamList paramList;
	CompGroupList groupList;
	::NVList systemParamList;
	::NVList groupParamList;
	m_start_date = "";
	m_stop_date = "";

	try
	{

		m_comp_num = MyParser.readConfFile(m_conf_file.c_str(), true);
		paramList = MyParser.getParamList();
		groupList = MyParser.getGroupList();

		if (m_debug)
		{
			cerr << "*** Comp num = " << m_comp_num << '\n';
			cerr << "*** paramList.size()  = " << paramList.size() << '\n';
			cerr << "*** groupList.size()  = " << groupList.size() << '\n';
			cerr << "*** serviceList.size()= " << m_daqServiceList.size() << '\n';
		}

		for (int index = 0; index < (int)paramList.size(); index++)
		{
			if (m_debug)
			{
				cerr << "ID:" << paramList[index].getId() << '\n';
			}
			::NVList mylist = paramList[index].getList();
			if (m_debug)
			{
				for (int i = 0; i < (int)mylist.length(); i++)
				{
					cerr << "  name :" << mylist[i].name << '\n';
					cerr << "  value:" << mylist[i].value << '\n';
				}
			}
		}
		if (m_debug)
		{
			for (int i = 0; i < (int)m_daqServiceList.size(); i++)
			{
				cerr << "*** id:" << m_daqServiceList[i].comp_id << '\n';
			}
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to read the Configuration file\n";
		cerr << "### Check the Configuration file\n";
		return 1;
	}

	if (m_debug)
	{
		cerr << "m_daqServiceList.size():" << m_daqServiceList.size() << '\n';
	}
	try
	{
		for (int i = 0; i < (int)m_daqservices.size(); i++)
		{
			RTC::ConnectorProfileList_var myprof = m_DaqServicePorts[i]->get_connector_profiles();

			char *id = CORBA::string_dup(myprof[0].name);

			if (m_debug)
			{
				cerr << "*** id:" << id << '\n';
			}

			for (int j = 0; j < (int)paramList.size(); j++)
			{
				if (m_debug)
				{
					cerr << "paramList[i].getId():" << paramList[j].getId() << '\n';
				}
				if (paramList[j].getId() == id)
				{
					if (m_debug)
					{
						cerr << "paramList[i].getId():" << paramList[j].getId() << '\n';
						cerr << "m_daqServiceList  id:" << id << '\n';
					}
					int len = paramList[j].getList().length();
					if (m_debug)
					{
						cerr << "paramList[i].getList().size()" << len << '\n';
					}
					::NVList mylist(len);
					mylist = paramList[j].getList();

					if (m_debug)
					{
						for (int k = 0; k < len; k++)
						{
							cerr << "mylist[" << k << "].name: " << mylist[k].name << '\n';
							cerr << "mylist[" << k << "].valu: " << mylist[k].value << '\n';
						}
					}
					m_daqservices[i]->setCompParams(paramList[j].getList());
				}
			}
			CORBA::string_free(id);
		}

		for (auto &daqservice : m_daqservices)
		{
			set_command(daqservice, CMD_CONFIGURE);
			check_done(daqservice);
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to configure Components.\n";
		return 1;
	}
	m_com_completed = true;
	return 0;
}
int DaqOperator::unconfigure_procedure()
{
	m_com_completed = false;
	try
	{
		for (auto &daqservice : m_daqservices)
		{
			set_command(daqservice, CMD_UNCONFIGURE);
			check_done(daqservice);
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to unconfigure Component.\n";
		return 1;
	}
	m_com_completed = true;
	return 0;
}
int DaqOperator::start_procedure()
{
	m_com_completed = false;
	try
	{
		time_t now = time(0);

		m_start_date = asctime(localtime(&now));
		m_start_date[m_start_date.length() - 1] = ' ';
		m_start_date.erase(0, 4);
		m_stop_date = "";

		if (m_debug)
		{
			cerr << "start_parocedure: runno: " << m_runNumber << '\n';
		}

		for (auto &daqservice : m_daqservices)
		{
			set_runno(daqservice, m_runNumber);
			check_done(daqservice);
		}

		for (auto &daqservice : m_daqservices)
		{
			set_command(daqservice, CMD_START);
			check_done(daqservice);
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to start Component.\n";
		return 1;
	}
	m_com_completed = true;
	return 0;
}
int DaqOperator::stop_procedure()
{
	m_com_completed = false;
	try
	{
		for (int i = (m_comp_num - 1); i >= 0; i--)
		{
			set_command(m_daqservices[i], CMD_STOP);
			check_done(m_daqservices[i]);
		}

		time_t now = time(0);
		m_stop_date = asctime(localtime(&now));
		m_stop_date[m_stop_date.length() - 1] = ' ';
		m_stop_date.erase(0, 4);
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to stop Component.\n";
		return 1;
	}
	// keep_alive.clear();
	// keep_dead.clear();
	m_com_completed = true;
	return 0;
}
int DaqOperator::pause_procedure()
{
	m_com_completed = false;
	try
	{

		for (int i = (m_comp_num - 1); i >= 0; i--)
		{
			set_command(m_daqservices[i], CMD_PAUSE);
			check_done(m_daqservices[i]);
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to pause Component.\n";
		return 1;
	}
	m_com_completed = true;
	return 0;
}
int DaqOperator::resume_procedure()
{
	m_com_completed = false;
	try
	{

		for (auto &daqservice : m_daqservices)
		{
			set_command(daqservice, CMD_RESUME);
			check_done(daqservice);
		}
	}
	catch (...)
	{
		cerr << "### ERROR: DaqOperator: Failed to resume Component.\n";
		return 1;
	}
	m_com_completed = true;
	return 0;
}
int DaqOperator::abort_procedure()
{
	cout << "abort_procedure: enter" << '\n';

	return 0;
}
int DaqOperator::putstatus_procedure()
{
	return 0;
}

int DaqOperator::log_procedure()
{
	return 0;
}

#ifdef NOUSE
void DaqOperator::addCorbaPort()
{
	RTC::CorbaConsumer<DAQService> daqservice;

	m_daqservices.emplace_back(daqservice);

	stringstream strstream;
	strstream << m_service_num++;
	string service_name = "service" + strstream.str();
}

void DaqOperator::delCorbaPort() {}
#endif

void DaqOperator::set_console_flag(bool isConsole)
{
	cerr << "set_console_flag(): " << isConsole << '\n';
	m_isConsoleMode = isConsole;
}

void DaqOperator::set_port_no(int port)
{
	m_param_port = port;
}

string DaqOperator::getConfFilePath()
{
	string pathFile = ".confFilePath";
	ifstream ifs(pathFile.c_str());
	string mypath = "";
	ifs >> mypath;
	return mypath;
}

string DaqOperator::getMsg()
{
	return m_msg;
}

string DaqOperator::getBody()
{
	return m_body;
}

int DaqOperator::command_configure()
{
	if (m_state != LOADED)
	{
		createDom_ng("Params");
		cerr << "   Bad Command\n";
		return 1;
	}

	m_config_file = m_config_file_tmp;
	if (configure_procedure() == 1)
	{
		char str_e[128];
		sprintf(str_e, FORMAT_IO_ERR_E, m_config_file.c_str());

		char str_j[128];
		sprintf(str_j, FORMAT_IO_ERR_J, m_config_file.c_str());

		createDom_ng("Params", RET_CODE_IO_ERR, str_e, str_j);

		return 1;
	}

	m_state = CONFIGURED;
	createDom_ok("Params");
	return 0;
}

int DaqOperator::command_unconfigure()
{
	//cout << "command_unconfigure: enter" << '\n';

	if (m_state != CONFIGURED)
	{
		createDom_ng("ResetParams");
		cerr << "   Bad Command\n";
		return 1;
	}
	unconfigure_procedure();
	m_state = LOADED;
	createDom_ok("ResetParams");
	return 0;
}

int DaqOperator::command_start()
{
	//cout << "command_start: enter" << '\n';

	if (m_state != CONFIGURED)
	{
		createDom_ng("Begin");
		cerr << "   Bad Command\n";
		return 1;
	}
	start_procedure();
	m_state = RUNNING;
	createDom_ok("Begin");
	return 0;
}

int DaqOperator::command_stop()
{
	//cout << "command_stop: enter" << '\n';
	if (m_state != RUNNING)
	{
		createDom_ng("End");
		cerr << "   Bad Command\n";
		return 1;
	}

	stop_procedure();
	m_state = CONFIGURED;
	createDom_ok("End");

	return 0;
}

int DaqOperator::command_abort()
{
	//cout << "command_abort: enter" << '\n';

	if (m_state != RUNNING)
	{
		createDom_ng("Abort");
		cerr << "   Bad Command\n";
		return 1;
	}

	abort_procedure();
	unconfigure_procedure();
	m_state = LOADED;

	createDom_ok("Abort");

	return 0;
}

int DaqOperator::command_confirmend()
{
	//cout << "command_confirmend: enter" << '\n';

	if (m_state == RUNNING)
	{
		createDom_ng("ConfirmEnd");
		cerr << "   Bad Command\n";
		return 1;
	}

	createDom_ok("ConfirmEnd");

	return 0;
}

int DaqOperator::command_putparams()
{
	//cout << "command_putparams: enter" << '\n';

	if (m_state == LOADED)
	{
		createDom_ng("Params");
		cerr << "   Bad Command\n";
		return 1;
	}

	DAQMW::CreateDom createDom;
	m_msg = createDom.getParams("Params", &m_nv_list[0]);

	return 0;
}

int DaqOperator::command_putstatus()
{
	putstatus_procedure();
	DAQMW::CreateDom createDom;
	m_msg = createDom.getStatus("Status", m_state);
	return 0;
}
int DaqOperator::command_log()
{
	log_procedure();
	DAQMW::CreateDom createDom;

	groupStatus groupStat;
	groupStatusList groupStatList;

	bool fatal_error = false;

	for (int i = 0; i < m_comp_num; i++)
	{

		RTC::ConnectorProfileList_var myprof = m_DaqServicePorts[i]->get_connector_profiles();
		groupStat.groupId = CORBA::string_dup(myprof[0].name);

		Status_var status = m_daqservices[i]->getStatus();

		groupStat.comp_status.comp_name = CORBA::string_dup(status->comp_name);
		groupStat.comp_status.state = status->state;
		groupStat.comp_status.event_size = status->event_size;
		groupStat.comp_status.comp_status = status->comp_status;

		if (groupStat.comp_status.comp_status == COMP_FATAL)
		{
			fatal_error = true;
		}

		groupStatList.emplace_back(groupStat);
	}

	if (fatal_error)
	{
		cerr << "### FATAL: command_log(): " << m_err_msg << '\n';
		m_msg = createDom.getLog("Log", groupStatList, m_err_msg);
		m_err_msg = "";
	}
	else
	{
		m_msg = createDom.getLog("Log", groupStatList);
	}

	for (unsigned int i = 0; i < groupStatList.size(); i++)
	{
		CORBA::string_free(groupStatList[i].groupId);
	}
	return 0;
}
int DaqOperator::command_pause()
{
	if (m_state != RUNNING)
	{
		createDom_ng("Pause");
		cerr << "   Bad Command\n";
		return 1;
	}

	pause_procedure();
	m_state = PAUSED;

	createDom_ok("Pause");
	return 0;
}
int DaqOperator::command_resume()
{
	if (m_state != PAUSED)
	{
		createDom_ng("Restart");
		cerr << "   Bad Command\n";
		return 1;
	}

	resume_procedure();
	m_state = RUNNING;

	createDom_ok("Restart");
	return 0;
}
int DaqOperator::command_stopparamsset()
{
	createDom_ok("StopParamsSet");
	return 0;
}
int DaqOperator::command_resetparams()
{
	createDom_ok("ResetParams");
	return 0;
}
int DaqOperator::command_save()
{
	createDom_ok("Save");
	return 0;
}
int DaqOperator::command_confirmconnection()
{
	createDom_ok("ConfirmConnection");
	return 0;
}
int DaqOperator::command_dummy()
{
	return 0;
}
void DaqOperator::createDom_ok(string name)
{
	DAQMW::CreateDom createDom;
	m_msg = createDom.getOK(name);
}
void DaqOperator::createDom_ng(string name)
{
	DAQMW::CreateDom createDom;
	string state = createDom.getState(m_state, false);
	m_msg = "";

	char str_e[128];
	sprintf(str_e, FORMAT_REQ_INV_IN_STS_E, state.c_str());
	char str_j[128];
	sprintf(str_j, FORMAT_REQ_INV_IN_STS_J, state.c_str());

	createDom_ng(name, RET_CODE_REQ_INV_IN_STS, str_e, str_j);
}
void DaqOperator::createDom_ng(string name, int code, char *str_e, char *str_j)
{
	DAQMW::CreateDom createDom;
	m_msg = createDom.getNG(name, code, name, str_e, str_j);
}
extern "C"
{
	void DaqOperatorInit(RTC::Manager *manager)
	{
		RTC::Properties profile(daqserviceconsumer_spec);
		manager->registerFactory(profile,
								 RTC::Create<DaqOperator>,
								 RTC::Delete<DaqOperator>);
	}
};
//- Add -----------------------------------------------------
int DaqOperator::reset_mytimer()
{
	mytimer->resetTimer();
	return 0;
}
int DaqOperator::output_performance(int command)
{
	struct timeval start_time;
	struct timezone tz;
	struct passwd *pw;
	uid_t uid;
	char fname[256];
	long gt[2];
	const string fout = "s4i-output";
	/* end time */
	gettimeofday(&start_time, &tz);
	gt[0] = start_time.tv_sec;
	gt[1] = start_time.tv_usec;
	uid = getuid();
	if ((pw = getpwuid(uid)))
	{
		sprintf(fname, "/home/%s/DAQ-Middleware/csv/s4i/%s.csv", pw->pw_name, fout.c_str());
	}
	/* File  */
	ofstream csv_file(fname, ios::app);
	switch (command)
	{
	case CMD_CONFIGURE:
		csv_file << "st,Configure," << gt[0] << ',' << gt[1] << '\n';
		break;
	case CMD_START:
		csv_file << "st,Start," << gt[0] << ',' << gt[1] << '\n';
		break;
	case CMD_STOP:
		csv_file << "st,Stop," << gt[0] << ',' << gt[1] << '\n';
		break;
	case CMD_UNCONFIGURE:
		csv_file << "st,Unconfigure," << gt[0] << ',' << gt[1] << '\n';
		break;
	case CMD_PAUSE:
		csv_file << "st,Pause," << gt[0] << ',' << gt[1] << '\n';
		break;
	case CMD_RESUME:
		csv_file << "st,Resume," << gt[0] << ',' << gt[1] << '\n';
		break;
	case CMD_RESTART:
		csv_file << "st,Restart," << gt[0] << ',' << gt[1] << '\n';
		break;
	}
	csv_file.close();
	return 0;
}
int DaqOperator::state_change_automation()
{
	static int state_management = 0;
	// command check
	if (state_management > 5)
		state_management = 0;
	if (state_management == 0)
	{
		output_performance(0);
		configure_procedure();
		sleep(2);
		state_management++;
	}

	if (state_management == 1)
	{
		output_performance(1);
		start_procedure();
		sleep(2);
		state_management++;
	}

	if (state_management == 2)
	{
		output_performance(4);
		pause_procedure();
		sleep(2);
		state_management++;
	}

	if (state_management == 3)
	{
		output_performance(5);
		resume_procedure();
		sleep(1);
		state_management++;
	}

	if (state_management == 4)
	{
		output_performance(2);
		stop_procedure();
		sleep(2);
		state_management++;
	}

	if (state_management == 5)
	{
		output_performance(3);
		unconfigure_procedure();
		sleep(1);
		state_management++;
	}
	return 0;
}
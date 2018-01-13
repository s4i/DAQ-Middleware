#ifndef GETTIME_H
#define GETTIME_H
#include <iostream>

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

#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <sys/select.h>
#include <sys/time.h>

#include "TimeServiceStub.h"

using namespace RTC;

struct timeServiceInfo {
    RTC::CorbaConsumer<TimeService> timeService;
};

typedef std::vector< serviceInfo > DaqServiceList;

class TimeService
    : public RTC::DataFlowComponentBase
{
public:
    static TimeService* Instance();

    TimeService(RTC::Manager* manager);
    ~TimeService();
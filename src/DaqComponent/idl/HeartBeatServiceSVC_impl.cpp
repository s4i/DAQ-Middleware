#include "HeartBeatServiceSVC_impl.h"

HeartBeatServiceSVC_impl::HeartBeatServiceSVC_impl()
    : m_oc('1'),
      m_co('1')
{

}

HeartBeatServiceSVC_impl::~HeartBeatServiceSVC_impl()
{

}

void HeartBeatServiceSVC_impl::setOperatorToComp(const CORBA::Char hb)
{
    m_oc = hb;
}

CORBA::Char HeartBeatServiceSVC_impl::getOperatorToComp()
{
    return m_oc;
}

void HeartBeatServiceSVC_impl::setCompToOperator(const CORBA::Char hb)
{
    m_co = hb;
}

CORBA::Char HeartBeatServiceSVC_impl::getCompToOperator()
{
    return m_co;
}
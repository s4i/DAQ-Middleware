// -*- C++ -*-
/*!
 * @file ComponentInfoContainer.h
 * @brief Daq Component information container class
 * @date
 * @author Kazuo Nakayoshi <kazuo.nakayoshi@kek.jp>
 *
 * Copyright (C) 2008
 *     Kazuo Nakayoshi
 *     Electronics System Group,
 *     KEK, Japan.
 *     All rights reserved.
 *
 */

#ifndef COMPONENTINFOCONTAINER_H
#define COMPONENTINFOCONTAINER_H

#include <string>
#include <vector>
#include <map>
#include "DAQServiceSVC_impl.h"

/**
 *  Component Parameter Container class
 *
 */

class ComponentParam
{
public:
    ComponentParam()
    { 
	m_nvList.release(); 
    }

    void setId(std::string id) 		
    {
	m_id = id;
    }

    std::string getId()			
    {
	return m_id;
    }

    void setList(NVList nvList)		
    {
	m_nvList = nvList;
    }

    NVList getList()			
    {
	return m_nvList;
    } 

private:
    std::string   m_id;
    NVList        m_nvList;
};

typedef std::vector<ComponentParam> ParamList;


/**
 *  Component Information Container class, except Parameters.
 *
 */
class ComponentInfoContainer
{
  public:
    ComponentInfoContainer()
    {

    }
    ~ComponentInfoContainer()
    {

    }
	
  public:
    void setId(std::string id) 		
    {
      m_id = id;
    }
    std::string getId()			
    {
      return m_id;
    }

    void setAddress(std::string address)	
    {
      m_address = address;
    }
    std::string getAddress()			
    {
      return m_address;
    }

    void setPort(std::string port) 		
    {
      m_port = port;
    }
    std::string getPort()			
    {
      return m_port;
    }

    void setName(std::string name) 		
    {
      m_name = name;
    }
    std::string getName()			
    {
      return m_name;
    }

    void setExec(std::string exec) 		
    {
      m_exec = exec;
    }
    std::string getExec()			
    {
      return m_exec;
    }

    void setConf(std::string conf) 		
    {
      m_conf = conf;
    }
    std::string getConf()			
    {
      return m_conf;
    }

    void setService(std::string service) 	
    {
      m_service.push_back(service);
      
    }
    std::vector<std::string> getService()			
    {
      return m_service;
    }

    void setStartupOrder(std::string startup_order) 	
    {
	  m_startup_order = startup_order;
    }
    std::string getStartupOrder()			
    {
      return m_startup_order;
    }

    void setInport(std::string inport) 	
    {
      m_inport.push_back(inport);
    }
    std::vector<std::string> getInport()			
    {
      return m_inport;
    }

    void setFromOutPort(std::string from) 	
    {
      m_from.push_back(from);
    }
    std::vector<std::string> getFromOutPort()		
    {
      return m_from;
    }
    void setBufferLength(std::string buffer_length)
    {
      m_buffer_length.push_back(buffer_length);
    }
    std::vector<std::string> getBufferLength()
    {
      return m_buffer_length;
    }
    void setBufferReadTimeout(std::string buffer_read_timeout)
    {
      m_buffer_read_timeout.push_back(buffer_read_timeout);
    }
    std::vector<std::string> getBufferReadTimeout()
    {
      return m_buffer_read_timeout;
    }
    void setBufferWriteTimeout(std::string buffer_write_timeout)
    {
      m_buffer_write_timeout.push_back(buffer_write_timeout);
    }
    std::vector<std::string> getBufferWriteTimeout()
    {
      return m_buffer_write_timeout;
    }
    void setBufferReadEmptyPolicy(std::string buffer_read_empty_policy)
    {
      m_buffer_read_empty_policy.push_back(buffer_read_empty_policy);
    }
    std::vector<std::string> getBufferReadEmptyPolicy()
    {
      return m_buffer_read_empty_policy;
    }
    void setBufferWriteFullPolicy(std::string buffer_write_full_policy)
    {
      m_buffer_write_full_policy.push_back(buffer_write_full_policy);
    }
    std::vector<std::string> getBufferWriteFullPolicy()
    {
      return m_buffer_write_full_policy;
    }

    void setOutport(std::string outport) 	
    {
      m_outport.push_back(outport);
    }
    std::vector<std::string> getOutport()			
    {
      return m_outport;
    }

  private:
    std::string m_id;
    std::string m_address;
    std::string m_port;
    std::string m_name;
    std::string m_exec;
    std::string m_conf;
    std::string m_startup_order;
    std::vector<std::string> m_service;
    std::vector<std::string> m_inport;
    std::vector<std::string> m_from;
    std::vector<std::string> m_buffer_length;
    std::vector<std::string> m_buffer_read_timeout;
    std::vector<std::string> m_buffer_write_timeout;
    std::vector<std::string> m_buffer_read_empty_policy;
    std::vector<std::string> m_buffer_write_full_policy;
    std::vector<std::string> m_outport;
};
typedef std::vector<ComponentInfoContainer> CompInfoList;

/**
 *  Component Group Container class.
 *  It includes CompInfoList.
 */
class ComponentGroup
{
public:
    ComponentGroup() {}
    ~ComponentGroup() {}

    void setGroupId(std::string gid) {
	m_gid = gid;
    }
    std::string getGroupId() {
	return m_gid;
    }

    CompInfoList getCompInfoList() {
	return m_compInfoList;
    }

    void setCompInfoList(CompInfoList& compInfoList) {
	m_compInfoList = compInfoList;
    }

private:
    std::string m_gid;
    CompInfoList m_compInfoList;
};    
typedef std::vector<ComponentGroup> CompGroupList;

struct groupStatus {
  char * groupId;
  Status comp_status;
};

typedef std::vector< groupStatus > groupStatusList;

#endif // COMPONENTINFOCONTAINER_H


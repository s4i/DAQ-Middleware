int DaqOperator::set_time()
{
	try {
		for (int i = 0; i < m_comp_num; i++) {
			try {
				set_gettime(m_daqservices[i]);
			}
			catch(...) {
				std::cerr << "### ERROR: set time: exception occured\n";
			}
		}
	}
	catch(...) {
		std::cerr << "### ERROR: DaqOperator: Failed to set Time.\n";
	}

	return 0;
}

int DaqOperator::set_gettime(RTC::CorbaConsumer<DAQService> daqservice)
{
	int status = 0;
	struct timeval start_time;
	struct timezone tz;
	gettimeofday(&start_time, &tz);
	try {
		status = daqservice->setTime(start_time.tv_usec);
	}
	catch(...) {
		std::cerr << "### ERROR: set time: exception occured\n";
	}
	return 0;
}
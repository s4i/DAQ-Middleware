DESTDIR =
prefix  = /usr

DIR = $(DESTDIR)$(prefix)/include/daqmw/idl
MODE = 0644

FILES += DAQService.idl
FILES += DAQServiceSVC_impl.cpp
FILES += DAQServiceSVC_impl.h

FILES += HeartBeatService.idl
FILES += HeartBeatServiceSVC_impl.cpp
FILES += HeartBeatServiceSVC_impl.h

FILES += TimeService.idl
FILES += TimeServiceSVC_impl.cpp
FILES += TimeServiceSVC_impl.h

all:

clean:

install:
	mkdir -p $(DIR)
	install -m $(MODE) $(FILES) $(DIR)

uninstall:
	@echo "---> uninstalling idl files."
	@for file in $(FILES); do rm -f $(DIR)/$${file}; done
	@echo "---> done"

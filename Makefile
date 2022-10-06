LIBS=-lz
ifneq "$(do_debug)" "0"
CXXFLAGS= -g -Wall $(INCLUDES) -DDO_DEBUG
else
CXXFLAGS= -g -Wall $(INCLUDES) 
endif


granul: main.c Makefile
	$(CXX) -o granul $< $(CXXFLAGS) $(LDFLAGS) $(LIBS)

clean:
	@rm -rf granul 

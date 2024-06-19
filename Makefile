TARGET = $(notdir $(CURDIR))
include ../../../config
C_FLAGS += -std=c++0x -Wall  -Werror=return-type -DWITH_HTTPS -DWITH_OPENSSL -DWITH_NO_C_LOCALE
ifeq ($(BETA_VERSION),true)
	C_FLAGS+=-DBETAVER
endif
CPP_FLAGS += -I../../../include -I./ -I../../ExtAppIpc/ -I$(ACCESS_DIR)/

LD_FLAGS += -lExtAppIpc -lPoco -lCommon -lNetComm  -lJson -lProto -lzmq -lsodium -lssl -lcrypto -L../../lib  -lpthread -lrt -ldl -Wl,-rpath=/app/lib
DEG_LD_FLAGS += -lExtAppIpcd -lPocod -lCommond -lNetCommd  -lJsond -lProtod -lprotobuf-lite -lzmq -lsodium -lssl  -lcrypto -L../../libd -lpthread -lrt -ldl -Wl,-rpath=/app/lib

COMPILE.c = $(CROSS_COMPILER)g++ $(C_FLAGS) $(CPP_FLAGS) -c
LINK.c = $(CROSS_COMPILER)g++

DBGTARGET = ./ext_$(TARGET)d.app
RELTARGET = ./ext_$(TARGET).app

DBGCFLAGS = -g -D__DEBUG
RELCFLAGS = -O2 -fno-strict-aliasing 

SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)


RELOBJFILES=$(addprefix release-$(CROSS_COMPILER)/,$(SOURCES:%.cpp=%.o))
DBGOBJFILES=$(addprefix debug-$(CROSS_COMPILER)/,$(SOURCES:%.cpp=%.o))


RELLDFLAGS = $(LD_FLAGS)
DBGLDFLAGS = $(DEG_LD_FLAGS)

.PHONY: clean debug release install

all:	debug release

install:
	install -d $(EXEC_DIR)
	install $(RELTARGET) $(EXEC_DIR)
	install -m 444 $(TARGET).txt $(EXEC_DIR)

release:	$(RELTARGET)

debug:		$(DBGTARGET)

$(RELTARGET):	$(RELOBJFILES)
	$(LINK.c) -fPIC -o $@ $^ $(RELLDFLAGS)
$(DBGTARGET):	$(DBGOBJFILES)
	$(LINK.c) -fPIC -o $@ $^ $(DBGLDFLAGS)


$(RELOBJFILES):	release-$(CROSS_COMPILER)/%.o: %.cpp  $(HEADERS)
	@mkdir -p release-$(CROSS_COMPILER)
	$(COMPILE.c) $(RELCFLAGS) -o $@ $<

$(DBGOBJFILES):	debug-$(CROSS_COMPILER)/%.o: %.cpp  $(HEADERS)
	@mkdir -p debug-$(CROSS_COMPILER)
	$(COMPILE.c) $(DBGCFLAGS) -o $@ $<

clean:
	-$(RM) -rf release* debug* *~ *.d .dep *.app 

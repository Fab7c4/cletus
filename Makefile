# What's the executable called?
PROJ =  run_controller run_actuators run_sensors run_function_test run_uart tests/test_uart tests/test_lisa_message tests/log_lisa sim_uart



# What C or C++ files must we compile to make the executable?
C_SRC = run_controller.c \
        run_actuators.c \
        run_uart.c \
        run_sensors.c \
        sensors.c \
        uart.c \
        controller.c \
	actuators.c \
        misc.c \
        zmq.c \
        run_function_test.c \
        tests/test_uart.c   \
        tests/test_lisa_message.c \
	tests/log_lisa.c \
	sim_uart.c


CXX_SRC = \
#	main.cpp \
#	parsing.cpp

LIBS = $(shell pkg-config --libs libzmq) $(shell pkg-config --libs libprotobuf-c)-lm -lrt -lprotobuf 

Q ?= @

PROTOS_CXX = protos_cpp/messages.pb.cc \
             protos_cpp/messages.pb.h

PROTOS_C = protos_c/messages.pb-c.c \
           protos_c/messages.pb-c.h

HS_PROTOS = hs/src/Messages.hs

# build in the autopilot
# this has a makefile format to make it easy to include
# into the paparazzi build system
UNAME := $(shell uname)

LDFLAGS = $(LIBS)
INCLUDES = -I./protos_c

#CFLAGS = -O3 -Wall -Wextra -Werror -std=gnu99 -Wimplicit -Wshadow -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Wcast-align -Wformat=2 -Wimplicit-function-declaration -Wredundant-decls -Wformat-security  -Werror -Os -march=native -ftree-vectorize -flto -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-all -fno-strict-overflow   -g -ftrapv

ifeq ($(UNAME),Darwin)
	LDFLAGS += -L/opt/local/lib
	INCLUDES += -I/opt/local/include
	INCLUDES += -isystem /usr/local/include
endif


OBJ = $(C_SRC:%.c=%.o) $(CXX_SRC:%.cpp=%.o) $(CXX_SRC:%.cc=%.o) protos_c/messages.pb-c.o

## Compile pedantically and save pain later
CXX_WARNINGFLAGS = -Wall -Wextra -Wshadow
C_WARNINGFLAGS =  -Wall -Wextra -Wshadow -Wstrict-prototypes
C_WARNINGFLAGS += -Wimplicit -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Wcast-align -Wformat=2 -Wimplicit-function-declaration -Wredundant-decls -Wformat-security -march=native -ftree-vectorize -flto -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-all -fno-strict-overflow -ftrapv

C_WARNINGFLAGS += -Werror
CXX_WARNINGFLAGS += -Werror
DEBUGFLAGS ?= -g -DDEBUG # -pg to generate profiling information

## Sensorflags for activating special sensor
#SENSORFLAGS ?= -DIMU
#SENSORFLAGS ?= -DGPS
#SENSORFLAGS ?= -DAIRSPEED
#SENSORFLAGS ?= -DAHRS
#SENSORFLAGS ?= -DRC
SENSORFLAGS ?= -DALL


OPTFLAGS = -O3

CFLAGS ?= $(C_WARNINGFLAGS) $(DEBUGFLAGS) $(FEATUREFLAGS) $(INCLUDES) $(OPTFLAGS) $(SENSORFLAGS) -std=gnu99
CXXFLAGS ?= $(CXX_WARNINGFLAGS) $(DEBUGFLAGS) $(FEATUREFLAGS) $(INCLUDES) $(OPTFLAGS) $(SENSORFLAGS) -std=gnu++0x
CC ?= gcc
CXX ?= g++

HS_STRUCTS = hs/src/Structs/Structures.hs hs/src/Structs/Structures.hsc

.PHONY: all
all: $(PROTOS_C) $(PROTOS_CXX)  protos_cpp/messages.pb.o protos_c/messages.pb-c.o $(PROJ)
hs: $(HS_PROTOS)

.SECONDEXPANSION:
$(PROJ): % : $$(findstring $$(*:%=%.o),$(OBJ)) $(filter-out $(PROJ:%=%.o),$(OBJ))
	@echo LD $@
ifneq (,$(CXX_SRC))
	$(Q)$(CXX) $(filter %.o %.a %.so, $^) $(LDFLAGS) -o $@
else
	$(Q)$(CC) $(filter %.o %.a %.so, $^) $(LDFLAGS) -o $@
endif

$(HS_PROTOS) : messages.proto
	@echo hprotoc $<
	$(Q)hprotoc --haskell_out=hs/src $<
	$(Q)touch hs/cletus.cabal

$(PROTOS_C) : messages.proto
	@echo protoc-c $<
	$(Q)protoc-c --c_out=protos_c $<

$(PROTOS_CXX) : messages.proto
	@echo protoc $<
	$(Q)protoc --cpp_out=protos_cpp $<

protos_cpp/messages.pb.o : protos_cpp/messages.pb.cc protos_cpp/messages.pb.h
	@echo CXX protos_cpp/messages.pb.cc
	$(Q)$(CXX) -O3 -Wall -Werror -lprotobuf -c protos_cpp/messages.pb.cc -o protos_cpp/messages.pb.o

protos_c/messages.pb-c.o : protos_c/messages.pb-c.c protos_c/messages.pb-c.h
	@echo CC protos_c/messages.pb-c.c
	$(Q)$(CC) -O3 -Wall -Werror -lprotobuf-c -c protos_c/messages.pb-c.c -o protos_c/messages.pb-c.o


sim/src/Structs/Structures.hsc : structures.h
	@echo c2hsc $@
	$(Q)cd sim/src/Structs && c2hsc --prefix=Structs ../../../$<

sim/src/Structs/Structures.hs : sim/src/Structs/Structures.hsc
	@echo hsc2hs $@
	$(Q)hsc2hs -Isim/src/Structs $< -o $@

%.o : %.c
	@echo CC $@
	$(Q)$(CC) $(CFLAGS) -c $< -o  $@

%.o : %.cpp
	@echo CXX $@
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(PROJ)
	rm -f $(PROTOS)
	rm -f hs/src/Messages.hs
	rm -f hs/src/Messages/*
	rm -f protos_cpp/messages.pb.*
	rm -f protos_c/messages.pb-c.*
	rm -f $(OBJ)
	rm -f $(HS_STRUCTS)

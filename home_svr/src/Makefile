CC = g++
WORKDIR = ..
SRCDIR = $(WORKDIR)/src
OBJDIR = $(WORKDIR)/obj
DEPDIR = $(WORKDIR)/obj
INCLUDEDIR = $(WORKDIR)/src
BINDIR = $(WORKDIR)/bin
TARGET = $(BINDIR)/libhome.so
#DATA_TARGET = $(BINDIR)/libdata.so
PROTODIR = $(WORKDIR)/../proto
PROTOBUF_CFLAGS = $(shell pkg-config --cflags protobuf)
PROTOBUF_LFLAGS = $(shell pkg-config --libs protobuf)

LIB_CLIENT_DIR = $(PROTODIR)/client
LIB_CLIENT_PROTO = $(LIB_CLIENT_DIR)/libclientproto.a
LIB_CLIENT_MAKEFILE_DIR = $(LIB_CLIENT_DIR)/define

LIB_HOME_DIR = $(PROTODIR)/home_svr
LIB_HOME_PROTO = $(LIB_HOME_DIR)/libhomeproto.a
LIB_HOME_MAKEFILE_DIR = $(LIB_HOME_DIR)/define

LIB_COMMON_DIR=$(PROTODIR)/client
LIB_COMMON_MAKEFILE_DIR=$(LIB_COMMON_DIR)/common_make
LIB_COMMON_PROTO=$(LIB_COMMON_DIR)/libcommonproto.a

LIB_SVR_COMMON_DIR=$(PROTODIR)/common
LIB_SVR_COMMON_MAKEFILE_DIR=$(LIB_SVR_COMMON_DIR)/define
LIB_SVR_COMMON_PROTO=$(LIB_SVR_COMMON_DIR)/libsvr_commonproto.a

LIB_SWITCH_DIR = $(PROTODIR)/switch
LIB_SWITCH_PROTO = $(LIB_SWITCH_DIR)/libswitchproto.a
LIB_SWITCH_MAKEFILE_DIR = $(LIB_SWITCH_DIR)/define

LIB_DB_DIR = $(PROTODIR)/db
LIB_DB_PROTO = $(LIB_DB_DIR)/libdbproto.a
LIB_DB_MAKEFILE_DIR = $(LIB_DB_DIR)/define

LIB_RANK_DIR = $(PROTODIR)/ranking
LIB_RANK_PROTO = $(LIB_RANK_DIR)/librankproto.a
LIB_RANK_MAKEFILE_DIR = $(LIB_RANK_DIR)/define

#LIB_BATTLE_DIR = $(PROTODIR)/battle
#LIB_BATTLE_PROTO = $(LIB_BATTLE_DIR)/libbattleproto.a
#LIB_BATTLE_MAKEFILE_DIR = $(LIB_BATTLE_DIR)/define

CFLAGS = -Wall -Werror -fPIC -ggdb \
	-Iinclude -I/usr/local/lib/glib-2.0/include/ -I/usr/local/include/glib-2.0 \
	-I/usr/lib/glib-2.0/include/ -I/usr/include/glib-2.0 \
	-I$(WORKDIR)/../ -I$(LIB_COMMON_DIR) -I$(LIB_SVR_COMMON_DIR) \
	-I/usr/include/libxml2 \
	-DUSE_TLOG -DENABLE_TRACE_LOG -I.

LFLAGS = -Wall -Werror -ggdb \
	-L$(LIB_HOME_DIR) -L$(LIB_SWITCH_DIR) -L$(LIB_DB_DIR) -L$(LIB_CLIENT_DIR) -L$(LIB_COMMON_DIR) -L$(LIB_SVR_COMMON_DIR) -L$(LIB_RANK_DIR)\
    -lcrypto -ltaomee /usr/lib/libprotobuf.a \
	-lhomeproto -ldbproto -lswitchproto -lclientproto -lcommonproto -lsvr_commonproto -lrankproto

HEADERS = $(wildcard $(INCLUDEDIR)/*.h)
#SRCS = $(shell ls -I "global_data*" $(SRCDIR)/*.cpp)
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(subst $(SRCDIR),$(OBJDIR),$(SRCS)))
DEPS := $(patsubst %.cpp,%.d,$(subst $(SRCDIR),$(DEPDIR),$(SRCS)))

all: client_proto home_proto db_proto switchproto commonproto rank_proto $(TARGET)


$(TARGET): $(OBJS) 
	g++ -shared -o $(TARGET) $(OBJS) $(CFLAGS) $(LFLAGS) /usr/lib/libprotobuf.a

commonproto:
	+make -C $(LIB_COMMON_MAKEFILE_DIR)

home_proto:
	+make -C $(LIB_HOME_MAKEFILE_DIR)

db_proto:
	+make -C $(LIB_DB_MAKEFILE_DIR)

switchproto:
	+make -C $(LIB_SWITCH_MAKEFILE_DIR)

client_proto: 
	+make -C $(LIB_CLIENT_MAKEFILE_DIR)

rank_proto:
	+make -C $(LIB_RANK_MAKEFILE_DIR)

#由于obj生成在$(OBJDIR)中，在依赖关系中加入目录
$(DEPDIR)/%.d: $(SRCDIR)/%.cpp  
	@echo "generate dependence $@"
	@printf "$(OBJDIR)/" > $@.tmp
	@$(CC) -MM $< $(CFLAGS) >> $@.tmp 2> /dev/null
	@mv $@.tmp $@
	
sinclude $(DEPS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp 
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	-rm $(OBJS) $(TARGET) $(DEPS)

clean-target:
	-rm $(TARGET) 

clean-all: clean
	make -C $(LIB_HOME_MAKEFILE_DIR) clean
	make -C $(LIB_DB_MAKEFILE_DIR) clean

#   make -C $(LIB_RANK_MAKEFILE_DIR) clean
	make -C $(LIB_CLIENT_MAKEFILE_DIR) clean
	make -C $(LIB_COMMON_MAKEFILE_DIR) clean
	make -C $(LIB_SVR_COMMON_MAKEFILE_DIR) clean
#	make -C $(LIB_BATTLE_MAKEFILE_DIR) clean

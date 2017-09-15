CC = g++
WORKDIR = ..
SRCDIR = $(WORKDIR)/src
OBJDIR = $(WORKDIR)/obj
DEPDIR = $(WORKDIR)/obj
INCLUDEDIR = $(WORKDIR)/src
BINDIR = $(WORKDIR)/bin
TARGET = $(BINDIR)/libbattle.so
DATA_TARGET = $(BINDIR)/libdata.so
PROTODIR = $(WORKDIR)/../proto
PROTOBUF_CFLAGS = $(shell pkg-config --cflags protobuf)
PROTOBUF_LFLAGS = $(shell pkg-config --libs protobuf)

LIB_CLIENT_DIR = $(PROTODIR)/client
LIB_CLIENT_PROTO = $(LIB_CLIENT_DIR)/libclientproto.a
LIB_CLIENT_MAKEFILE_DIR = $(LIB_CLIENT_DIR)/define

LIB_COMMON_DIR=$(PROTODIR)/client
LIB_COMMON_MAKEFILE_DIR=$(LIB_COMMON_DIR)/common_make
LIB_COMMON_PROTO=$(LIB_COMMON_DIR)/libcommonproto.a

LIB_SVR_COMMON_DIR=$(PROTODIR)/common
LIB_SVR_COMMON_MAKEFILE_DIR=$(LIB_SVR_COMMON_DIR)/define
LIB_SVR_COMMON_PROTO=$(LIB_SVR_COMMON_DIR)/libsvr_commonproto.a

LIB_BATTLE_DIR = $(PROTODIR)/battle
LIB_BATTLE_PROTO = $(LIB_BATTLE_DIR)/libbattleproto.a
LIB_BATTLE_MAKEFILE_DIR = $(LIB_BATTLE_DIR)/define

CFLAGS = -Wall -Werror -fPIC -ggdb \
	-Iinclude -I/usr/local/lib/glib-2.0/include/ -I/usr/local/include/glib-2.0 \
	-I/usr/lib/glib-2.0/include/ -I/usr/include/glib-2.0 \
	-I$(WORKDIR)/../ -I$(LIB_COMMON_DIR) -I$(LIB_SVR_COMMON_DIR) -I$(LIB_BATTLE_DIR) \
	-I/usr/include/libxml2 \
	-DUSE_TLOG -DENABLE_TRACE_LOG -I./
LFLAGS = -Wall -Werror -ggdb \
	-L$(LIB_CLIENT_DIR) \
	-L$(LIB_COMMON_DIR) -L$(LIB_SVR_COMMON_DIR) -L$(LIB_BATTLE_DIR) \
	-lcrypto -pthread -lpthread -lm -lxml2 \
	-lclientproto -lbattleproto \
	-ltaomee -ltaomee++ -lcommonproto -lsvr_commonproto

HEADERS = $(wildcard $(INCLUDEDIR)/*.h)
DATA_SRCS = $(SRCDIR)/global_data.cpp
SRCS = $(shell ls -I "global_data*" $(SRCDIR)/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(subst $(SRCDIR),$(OBJDIR),$(SRCS)))
OBJS := $(subst $(OBJDIR)/global_data.o,,$(OBJS))
DATA_OBJS := $(patsubst %.cpp,%.o,$(subst $(SRCDIR),$(OBJDIR),$(DATA_SRCS)))
DEPS := $(patsubst %.cpp,%.d,$(subst $(SRCDIR),$(DEPDIR),$(SRCS)))

all: commonproto svr_commonproto client_proto battle_proto \
	$(TARGET) $(DATA_TARGET)

$(DATA_TARGET): $(DATA_OBJS)
	g++ -shared -o $(DATA_TARGET) $(DATA_OBJS) $(LFLAGS)

$(TARGET): $(LIB_CLIENT_PROTO) $(LIB_DB_PROTO) $(LIB_SWITCH_PROTO) $(LIB_BATTLE_PROTO) $(OBJS) 
	g++ -shared -o $(TARGET) $(OBJS) $(LFLAGS) /usr/lib/libprotobuf.a

client_proto: 
	+make -C $(LIB_CLIENT_MAKEFILE_DIR)

commonproto:
	+make -C $(LIB_COMMON_MAKEFILE_DIR)

svr_commonproto:
	+make -C $(LIB_SVR_COMMON_MAKEFILE_DIR)

battle_proto:
	+make -C $(LIB_BATTLE_MAKEFILE_DIR)

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
	-rm $(OBJS) $(TARGET) $(DEPS) $(DATA_TARGET)

clean-target:
	-rm $(TARGET) 

clean-all: clean
	make -C $(LIB_CLIENT_MAKEFILE_DIR) clean
	make -C $(LIB_COMMON_MAKEFILE_DIR) clean
	make -C $(LIB_SVR_COMMON_MAKEFILE_DIR) clean
	make -C $(LIB_BATTLE_MAKEFILE_DIR) clean

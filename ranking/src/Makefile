CC = g++
WORKDIR = ../
SRCDIR = $(WORKDIR)/src
LIBDIR = $(WORKDIR)/lib
OBJDIR = $(WORKDIR)/obj
DEPDIR = $(WORKDIR)/obj
INCLUDEDIR = $(WORKDIR)/src
BINDIR = $(WORKDIR)/bin
TARGET = $(BINDIR)/librank.so
PROTODIR = $(WORKDIR)../proto

LIB_RANK_DIR = $(PROTODIR)/ranking
LIB_RANK_PROTO = $(LIB_RANK_DIR)/librankproto.a
LIB_RANK_MAKEFILE_DIR = $(LIB_RANK_DIR)/define

LIB_CLI_DIR = $(PROTODIR)/client
LIB_CLI_HEADER = $(LIB_CLI_DIR)/attr_type.h
LIB_CLI_MAKEFILE_DIR = $(LIB_CLI_DIR)/define

LIB_COMMON_DIR=$(PROTODIR)/client
LIB_COMMON_MAKEFILE_DIR=$(LIB_COMMON_DIR)/common_make
LIB_COMMON_PROTO=$(LIB_COMMON_DIR)/libcommonproto.a

LIB_SVR_COMMON_DIR=$(PROTODIR)/common
LIB_SVR_COMMON_MAKEFILE_DIR=$(LIB_SVR_COMMON_DIR)/define
LIB_SVR_COMMON_PROTO=$(LIB_SVR_COMMON_DIR)/libsvr_commonproto.a

REDISCLIENT_DIR=$(WORKDIR)/redisclient

CFLAGS = -Wall -DUSE_TLOG -DENABLE_TRACE_LOG -Werror -fPIC -ggdb \
	-Iinclude -I/usr/lib/glib-2.0/include/ -I/usr/include/glib-2.0 \
	-I$(WORKDIR)/../ -I/usr/include/mysql \
	-I$(LIB_COMMON_DIR) -I$(LIB_SVR_COMMON_DIR) \
	-I/usr/include/dbser -I../../proto/client -I../include -I/usr/local/boost_1_52_0 -I../ \

LFLAGS = -Wall -Werror -ggdb \
	-L$(LIBDIR) -L$(LIB_RANK_DIR) \
	-L$(LIB_COMMON_DIR) -L$(LIB_SVR_COMMON_DIR) \
	-L/usr/local/lib \
	-L../redisclient/ -ldl -lredisclient -lboost_system -L/usr/local/boost_1_52_0/stage/lib \
	-lcrypto \
	-lrankproto -ltaomee++ -lmysqlclient -ldbser -ltaomee -lcommonproto -lsvr_commonproto \
	/usr/lib/libprotobuf.a

SRCS = $(wildcard $(SRCDIR)/*.cpp)
HEADERS = $(wildcard $(INCLUDEDIR)/*.h)
OBJS := $(patsubst %.cpp,%.o,$(subst $(SRCDIR),$(OBJDIR),$(SRCS)))
DEPS := $(patsubst %.cpp,%.d,$(subst $(SRCDIR),$(DEPDIR),$(SRCS)))

all: commonproto svr_commonproto rank_proto cli_header redisclient $(TARGET)

$(TARGET): $(LIB_RANK_PROTO) $(LIB_CLI_HEADER) $(OBJS) 
	g++ -shared -o $(TARGET) $(OBJS) $(LFLAGS)

commonproto:
	make -C $(LIB_COMMON_MAKEFILE_DIR)

svr_commonproto:
	make -C $(LIB_SVR_COMMON_MAKEFILE_DIR)

rank_proto: 
	make -C $(LIB_RANK_MAKEFILE_DIR)

cli_header:
	make -C $(LIB_CLI_MAKEFILE_DIR)

redisclient:
	+make -C $(REDISCLIENT_DIR)

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

clean-all:
	make -C $(LIB_RANK_MAKEFILE_DIR) clean

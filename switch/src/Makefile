CC = g++
WORKDIR = ../
SRCDIR = $(WORKDIR)/src
LIBDIR = $(WORKDIR)/lib
OBJDIR = $(WORKDIR)/obj
DEPDIR = $(WORKDIR)/obj
INCLUDEDIR = $(WORKDIR)/src
BINDIR = $(WORKDIR)/bin
TARGET = $(BINDIR)/libswitch.so
PROTODIR = $(WORKDIR)/../proto

LIB_DB_DIR = $(PROTODIR)/db
LIB_DB_PROTO = $(LIB_DB_DIR)/libdbproto.a
LIB_DB_MAKEFILE_DIR = $(LIB_DB_DIR)/define

LIB_COMMON_DIR=$(PROTODIR)/client
LIB_COMMON_MAKEFILE_DIR=$(LIB_COMMON_DIR)/common_make
LIB_COMMON_PROTO=$(LIB_COMMON_DIR)/libcommonproto.a

LIB_SVR_COMMON_DIR=$(PROTODIR)/common
LIB_SVR_COMMON_MAKEFILE_DIR=$(LIB_SVR_COMMON_DIR)/define
LIB_SVR_COMMON_PROTO=$(LIB_SVR_COMMON_DIR)/libsvr_commonproto.a

LIB_SWITCH_DIR = $(PROTODIR)/switch
LIB_SWITCH_PROTO = $(LIB_SWITCH_DIR)/libswitchproto.a
LIB_SWITCH_MAKEFILE_DIR = $(LIB_SWITCH_DIR)/define

STATLOGGER_DIR = $(WORKDIR)/../statlogger

CFLAGS = -Wall -Werror -fPIC -ggdb  \
	-Iinclude -I/usr/lib/glib-2.0/include/ -I/usr/include/glib-2.0 -I/usr/include/uuid \
	-I/usr/local/lib/glib-2.0/include/ -I/usr/local/include/glib-2.0 \
	-I$(WORKDIR)/../  -I$(LIB_COMMON_DIR) -I$(LIB_SVR_COMMON_DIR) -DOL_DEBUG -I/usr/include/libxml2 \
	-DOL_DEBUG -DUSE_TLOG -DENABLE_TRACE_LOG

LFLAGS = -Wall -Werror -ggdb \
	-L$(LIBDIR) -L$(LIB_SWITCH_DIR) -L$(LIB_COMMON_DIR) -L$(LIB_SVR_COMMON_DIR) -L$(LIB_DB_DIR) -L$(STATLOGGER_DIR) \
	-lcrypto -luuid -ltaomee /usr/lib/libprotobuf.a -lstatlogger \
	-ltaomee++ -lswitchproto -lcommonproto -lsvr_commonproto -ldbproto

SRCS = $(wildcard $(SRCDIR)/*.cpp)
HEADERS = $(wildcard $(INCLUDEDIR)/*.h)
OBJS := $(patsubst %.cpp,%.o,$(subst $(SRCDIR),$(OBJDIR),$(SRCS)))
DEPS := $(patsubst %.cpp,%.d,$(subst $(SRCDIR),$(DEPDIR),$(SRCS)))

all: switchproto commonproto svr_commonproto db_proto $(TARGET)

$(TARGET): $(LIB_SWITCH_PROTO) $(OBJS)
	$(CC) -shared -o $(TARGET) $(OBJS) $(LFLAGS)


switchproto: 
	+make -C $(LIB_SWITCH_MAKEFILE_DIR)

commonproto:
	+make -C $(LIB_COMMON_MAKEFILE_DIR)

svr_commonproto:
	+make -C $(LIB_SVR_COMMON_MAKEFILE_DIR)

db_proto:
	+make -C $(LIB_DB_MAKEFILE_DIR)

#由于obj生成在$(OBJDIR)中，在依赖关系中加入目录
$(DEPDIR)/%.d: $(SRCDIR)/%.cpp  
	@echo "generate $@"
	@printf "$(OBJDIR)/" > $@.tmp
	@$(CC) -MM $< $(CFLAGS) >> $@.tmp
	@mv $@.tmp $@
	
sinclude $(DEPS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp 
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	-rm $(OBJS) $(TARGET) $(DEPS)

clean-target:
	-rm $(TARGET)

clean-all: clean
	make -C $(LIB_SWITCH_MAKEFILE_DIR) clean
	make -C $(LIB_COMMON_MAKEFILE_DIR) clean
	make -C $(LIB_SVR_COMMON_MAKEFILE_DIR) clean
	make -C $(LIB_DB_MAKEFILE_DIR) clean


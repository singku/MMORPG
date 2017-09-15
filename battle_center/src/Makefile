CC = g++
WORKDIR = ../
SRCDIR = $(WORKDIR)/src
LIBDIR = $(WORKDIR)/lib
OBJDIR = $(WORKDIR)/obj
DEPDIR = $(WORKDIR)/obj
INCLUDEDIR = $(WORKDIR)/src
BINDIR = $(WORKDIR)/bin
TARGET = $(BINDIR)/libbtlcenter.so
PROTODIR = $(WORKDIR)/../proto

LIB_CLIENT_DIR = $(PROTODIR)/client
LIB_CLIENT_PROTO = $(LIB_CLIENT_DIR)/libclientproto.a
LIB_CLIENT_MAKEFILE_DIR = $(LIB_CLIENT_DIR)/define

LIB_BTL_DIR = $(PROTODIR)/battle
LIB_BTL_PROTO = $(LIB_DB_DIR)/libbattleproto.a
LIB_BTL_MAKEFILE_DIR = $(LIB_BTL_DIR)/define

LIB_COMMON_DIR=$(PROTODIR)/client
LIB_COMMON_MAKEFILE_DIR=$(LIB_COMMON_DIR)/common_make
LIB_COMMON_PROTO=$(LIB_COMMON_DIR)/libcommonproto.a

LIB_SVR_COMMON_DIR=$(PROTODIR)/common
LIB_SVR_COMMON_MAKEFILE_DIR=$(LIB_SVR_COMMON_DIR)/define
LIB_SVR_COMMON_PROTO=$(LIB_SVR_COMMON_DIR)/libsvr_commonproto.a

CFLAGS = -Wall -Werror -fPIC -ggdb  \
	-Iinclude -I/usr/lib/glib-2.0/include/ -I/usr/include/glib-2.0 -I/usr/include/uuid \
	-I/usr/local/lib/glib-2.0/include/ -I/usr/local/include/glib-2.0 \
	-I$(WORKDIR)/../  -I$(LIB_BTL_DIR) -I$(LIB_COMMON_DIR) -I$(LIB_SVR_COMMON_DIR) -I$(LIB_CLIENT_DIR) \
	-I/usr/include/libxml2 \
	-DOL_DEBUG -DUSE_TLOG -DENABLE_TRACE_LOG

LFLAGS = -Wall -Werror -ggdb \
	-L$(LIBDIR) -L$(LIB_BTL_DIR) -L$(LIB_COMMON_DIR) -L$(LIB_SVR_COMMON_DIR) -L$(LIB_CLIENT_DIR)\
	-lcrypto -luuid -ltaomee /usr/lib/libprotobuf.a \
	-ltaomee++ -lclientproto -lbattleproto -lcommonproto -lsvr_commonproto

SRCS = $(wildcard $(SRCDIR)/*.cpp)
HEADERS = $(wildcard $(INCLUDEDIR)/*.h)
OBJS := $(patsubst %.cpp,%.o,$(subst $(SRCDIR),$(OBJDIR),$(SRCS)))
DEPS := $(patsubst %.cpp,%.d,$(subst $(SRCDIR),$(DEPDIR),$(SRCS)))

all: battleproto commonproto svr_commonproto clientproto $(TARGET)

$(TARGET): $(LIB_SWITCH_PROTO) $(OBJS)
	$(CC) -shared -o $(TARGET) $(OBJS) $(LFLAGS)

clientproto:
	+make -C $(LIB_CLIENT_MAKEFILE_DIR)

battleproto: 
	+make -C $(LIB_BTL_MAKEFILE_DIR)

commonproto:
	+make -C $(LIB_COMMON_MAKEFILE_DIR)

svr_commonproto:
	+make -C $(LIB_SVR_COMMON_MAKEFILE_DIR)

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
	make -C $(LIB_CLIENT_MAKEFILE_DIR) clean
	make -C $(LIB_COMMON_MAKEFILE_DIR) clean
	make -C $(LIB_SVR_COMMON_MAKEFILE_DIR) clean
	make -C $(LIB_BTL_MAKEFILE_DIR) clean


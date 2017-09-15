CC = g++
WORKDIR = ../
SRCDIR = $(WORKDIR)/src
LIBDIR = $(WORKDIR)/lib
OBJDIR = $(WORKDIR)/obj
DEPDIR = $(WORKDIR)/obj
INCLUDEDIR = $(WORKDIR)/src
BINDIR = $(WORKDIR)/bin
TARGET = $(BINDIR)/libcache_svr.so
PROTODIR = $(WORKDIR)/../proto

LIB_COMMON_DIR=$(PROTODIR)/client
LIB_COMMON_MAKEFILE_DIR=$(LIB_COMMON_DIR)/common_make
LIB_COMMON_PROTO=$(LIB_COMMON_DIR)/libcommonproto.a

LIB_CACHE_DIR = $(PROTODIR)/cache
LIB_CACHE_PROTO = $(LIB_CACHE_DIR)/libcacheproto.a
LIB_CACHE_MAKEFILE_DIR = $(LIB_CACHE_DIR)/define
LIB_DB_DIR = $(PROTODIR)/db
LIB_DB_PROTO = $(LIB_DB_DIR)/libdbproto.a
LIB_DB_MAKEFILE_DIR = $(LIB_DB_DIR)/define
LIB_REDIS = ./hiredis/libhiredis.a

CFLAGS = -Wall -Werror -fPIC -ggdb  \
	-Iinclude -I/usr/lib/glib-2.0/include/ -I/usr/include/glib-2.0 -I./hiredis \
	-I/usr/local/lib/glib-2.0/include/ -I/usr/local/include/glib-2.0 \
	-I$(WORKDIR)/../ -I$(LIB_COMMON_DIR) \
	-DUSE_TLOG -DENABLE_TRACE_LOG
LFLAGS = -Wall -Werror -ggdb \
	-L$(LIBDIR) -L$(LIB_COMMON_DIR) -L$(LIB_CACHE_DIR) -L$(LIB_DB_DIR) \
 	-lcrypto -ltaomee /usr/lib/libprotobuf.a \
	-lclientproto -lcacheproto -ltaomee++ -ldbproto

SRCS = $(wildcard $(SRCDIR)/*.cpp)
HEADERS = $(wildcard $(INCLUDEDIR)/*.h)
OBJS := $(patsubst %.cpp,%.o,$(subst $(SRCDIR),$(OBJDIR),$(SRCS)))
DEPS := $(patsubst %.cpp,%.d,$(subst $(SRCDIR),$(DEPDIR),$(SRCS)))

all: common_proto redis cache_proto db_proto $(TARGET)

$(TARGET): $(LIB_COMMON_PROTO) $(LIB_CACHE_PROTO) $(LIB_DB_PROTO) $(LIB_REDIS) $(OBJS)
	$(CC) -shared -o $(TARGET) $(OBJS) $(LFLAGS) $(LIB_REDIS)

common_proto:
	+make -C $(LIB_COMMON_MAKEFILE_DIR)

redis:
	+make -C ./hiredis

cache_proto: 
	+make -C $(LIB_CACHE_MAKEFILE_DIR)

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
	-rm $(OBJS) $(TARGET)

clean-target:
	-rm $(TARGET)

clean-all: clean
	make -C $(LIB_CACHE_MAKEFILE_DIR) clean
	make -C $(LIB_DB_MAKEFILE_DIR) clean

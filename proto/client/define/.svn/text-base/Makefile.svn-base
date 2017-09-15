CC=g++
PROTO_FILES=$(wildcard *.proto)
SRC_FILES=$(PROTO_FILES:%.proto=../%.pb.cc)
INC_FILES=$(PROTO_FILES:%.proto=../%.pb.h)
OBJ_FILES=$(PROTO_FILES:%.proto=../%.pb.o)
DEP_FILES=$(PROTO_FILES:%.proto=../%.pb.d)
PROTOBUF_CFLAGS=$(shell pkg-config --cflags protobuf)
PROTOBUF_LFLAGS=$(shell pkg-config --libs protobuf)
CFLAGS=-I./ -Wall -Werror -fPIC -ggdb $(PROTOBUF_CFLAGS)
LFLAGS=$(PROTOBUF_LFLAGS)
ERRNO_INC_FILE=../cli_errno.h
ERRNO_XML=errno.txt
CMD_INC_FILE=../cli_cmd.h
CMD_DIR=.
CMD_MSG_CPP_FILE=../cmd_msg.cpp
ATTR_INC_FILE=../attr_type.h
ATTR_XML=../../../conf/attribute.xml
TARGET=../libclientproto.a

$(TARGET): $(SRC_FILES) $(OBJ_FILES) $(ERRNO_INC_FILE) $(CMD_INC_FILE) $(ATTR_INC_FILE) $(CMD_MSG_CPP_FILE)
	ar r $(TARGET) $(OBJ_FILES)

$(ERRNO_INC_FILE): $(ERRNO_XML)
	php tools/gen_errno.php $< > $@

$(CMD_INC_FILE): $(CMD_DIR)
	php tools/gen_cmd.php $< > $@

$(CMD_MSG_CPP_FILE): $(CMD_DIR)
	php tools/gen_cmd_msg.php $< > $@

$(ATTR_XML):
	mkdir -p ../../../conf
	svn co --force http://svn.taomee.com/dolphin/client/trunk/design/config ../../../conf

$(ATTR_INC_FILE): $(ATTR_XML)
	php tools/gen_attr.php $< > $@

#由于obj生成在$(OBJDIR)中，在依赖关系中加入目录
$(DEP_FILES): ../%.pb.d : ../%.pb.cc 
	@echo "generate dependence $@"
	@printf "../" > $@.tmp
	@$(CC) -MM $< $(CFLAGS) >> $@.tmp 2> /dev/null
	@mv $@.tmp $@

$(OBJ_FILES) : ../%.pb.o : ../%.pb.cc
	$(CC) -o $@ -c $< $(CFLAGS)

$(SRC_FILES) : ../%.pb.cc : %.proto
	protoc --cpp_out=../ --proto_path=./ $<	

clean:
	-rm $(SRC_FILES) $(INC_FILES) $(OBJ_FILES) $(CMD_INC_FILE) $(ERRNO_INC_FILE) $(TARGET) $(DEP_FILES) $(ATTR_INC_FILE)

sinclude $(DEP_FILES)

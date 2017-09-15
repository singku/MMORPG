PROTO_FILES=$(wildcard *.proto)
SRC_FILES=$(PROTO_FILES:%.proto=../%.pb.cc)
INC_FILES=$(PROTO_FILES:%.proto=../%.pb.h)
OBJ_FILES=$(PROTO_FILES:%.proto=../%.pb.o)
DEP_FILES=$(PROTO_FILES:%.proto=../%.pb.d)
PROTOBUF_CFLAGS=$(shell pkg-config --cflags protobuf)
PROTOBUF_LFLAGS=$(shell pkg-config --libs protobuf)
CFLAGS=-I./ -Wall -Werror -fPIC -ggdb $(PROTOBUF_CFLAGS)
LFLAGS=$(PROTOBUF_LFLAGS)
TARGET=../libsvr_commonproto.a

$(TARGET): $(SRC_FILES) $(OBJ_FILES) 
	ar r $(TARGET) $(OBJ_FILES)

$(SRC_FILES) : ../%.pb.cc : %.proto
	protoc --cpp_out=../ --proto_path=./ $<	

$(OBJ_FILES) : ../%.pb.o : ../%.pb.cc
	g++ -o $@ -c $< $(CFLAGS)

#由于obj生成在$(OBJDIR)中，在依赖关系中加入目录
$(DEP_FILES): ../%.pb.d : ../%.pb.cc 
	@echo "generate dependence $@"
	@printf "../" > $@.tmp
	@$(CC) -MM $< $(CFLAGS) >> $@.tmp 2> /dev/null
	@mv $@.tmp $@

clean:
	-rm $(SRC_FILES) $(INC_FILES) $(OBJ_FILES) $(TARGET) $(DEP_FILES)

sinclude $(DEP_FILES)

CC=g++
PROTO_FILE=../define/common.proto
SRC_FILE=../common.pb.cc
OBJ_FILE=$(SRC_FILE:%.cc=%.o)
DEP_FILE=$(SRC_FILE:%.cc=%.d)
PROTOBUF_CFLAGS=$(shell pkg-config --cflags protobuf)
PROTOBUF_LFLAGS=$(shell pkg-config --libs protobuf)

CFLAGS=-I./ -Wall -Werror -fPIC -ggdb $(PROTOBUF_CFLAGS)
TARGET=../libcommonproto.a

$(TARGET): $(SRC_FILE) $(OBJ_FILE) 
	ar r $(TARGET) $(OBJ_FILE)

#由于obj生成在$(OBJDIR)中，在依赖关系中加入目录
$(DEP_FILE): %.pb.d : %.pb.cc 
	@echo "generate dependence $@"
	@printf "./" > $@.tmp
	@$(CC) -MM $< $(CFLAGS) >> $@.tmp 2> /dev/null
	@mv $@.tmp $@

$(OBJ_FILE):
	$(CC) -o $(OBJ_FILE) -c $(SRC_FILE) $(CFLAGS)

$(SRC_FILE):
	protoc --cpp_out=../ --proto_path=../define $(PROTO_FILE)

clean:
	-rm $(SRC_FILES) $(INC_FILES) $(OBJ_FILES)  $(TARGET) $(DEP_FILES) 

sinclude $(DEP_FILES)

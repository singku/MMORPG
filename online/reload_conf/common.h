#ifndef __COMMON_H__
#define __COMMON_H__


// c include files
extern "C" {
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>
#include <libtaomee/conf_parser/config.h>
}


// cpp include files
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include <set>
#include <list>

// libtaomee++ include files

// protobuf common include files
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/message.h> 
#include <gflags/gflags.h>

//=========================================================================
using google::protobuf::Message;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::ZeroCopyOutputStream;
using google::protobuf::TextFormat;
using google::protobuf::compiler::Importer;
using google::protobuf::compiler::DiskSourceTree;
using google::protobuf::DynamicMessageFactory;
using google::protobuf::FieldDescriptorProto;
using google::protobuf::Reflection;
using google::protobuf::Descriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::MessageFactory;
using google::protobuf::FieldDescriptorProto_Label;
using google::protobuf::compiler::MultiFileErrorCollector;

//=========================================================================
// enums
//=========================================================================

// 最大包长
enum pkglen_t {
	/* cs 协议最大包长 */
	max_cs_pkglen		= (32 * 1024), // 32KB

	/* server间最大包长 */
	max_svr_pkglen		= (128 * 1024), // 128KB
};



//=========================================================================

using namespace std;


#endif //__COMMON_H__

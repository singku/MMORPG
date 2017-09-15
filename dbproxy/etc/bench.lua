--程序名称
local program_name= string.match( get_program_name() ,"([^/]+)$" )
--内网ip
local ip=get_ip(1) ;
--公网ip
local public_ip=get_ip(2) ;

-- dir to store logs
log_dir = "./log/" 
-- log level
log_level = "7" 
-- max size of each log file
log_size = "104857600" 
-- max number of log files per log level
max_log_files = "100" 

-- size of the buffer to hold incoming/outcoming packets
shmq_length = "8388608" 
-- running mode of the program
run_mode = "background" 

-- mcast address
mcast_ip = "239.0.1.9" 
-- mcast port
mcast_port = "5757" 
-- interface on which arriving multicast datagrams will be received
mcast_incoming_if = "eth0" 
-- interface for outgoing multicast datagrams
mcast_outgoing_if = "10.1.1.154" 
incoming_packet_max_size = "3276800" 

-- dll file
dll_file = "./bin/libproxy.so" 

-- bind file
-- 关闭:2
LOG_PROTO_FLAG = "0" 
route_file = "./etc/route.xml" 

------加载其它模块列表列表 以","分开-----begin-----
--DLL_FILE_LIST		= 	 "./bin/liblogin_test.so"
------加载其它模块列表------end----


--# mcast config for synchronize the name and ip address of a given service
addr_mcast_ip = "239.0.0.1" 
addr_mcast_port = "5858" 
--### interface on which arriving multicast datagrams will be received
addr_mcast_incoming_if = "eth0" 
--
addr_mcast_outgoing_if = "10.1.1.154"

--------------------------------
--bind
-- item 格式 { id, "server.name", "ip", port  } 
async_server_bind_map={ }
async_server_bind_map[0]={220, "dp_dbpr_sk",	ip, 20220 }

tm_dirty_use_dirty_logical ="0"

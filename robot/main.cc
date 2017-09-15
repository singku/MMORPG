#include "common.h"
#include "config.h"
#include "client.h"
#include "robot.h"


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("\tUsage: %s config_full_path(\"./proto/robot.pbconf\")\n", 
                argv[0]);
        return -1;
    }
    if (argc == 3) {
        echo_msg = atoi(argv[2]);
    }
    FILE *fp = fopen("./road_map", "rb");
    if (fp) {
        uint32_t size = 0;
        char str[4096];
        while (!feof(fp)) {
            memset(str, 0, sizeof(str));
            fread(&size, sizeof(uint32_t), 1, fp);
            fread(str, size, 1, fp);
            string tmp(str, size);
            onlineproto::cs_0x0106_player_change_state msg;
            if (!msg.ParsePartialFromString(tmp)) {
                continue;
            }
            //cout << size <<": "<< msg.Utf8DebugString() <<endl;
            walk_vec.push_back(tmp);
        }
        fclose(fp);
        //exit(0);
    }

    config_full_path.assign(argv[1]);
    g_thread_init(NULL);
    srand(time(0));
	google::InitGoogleLogging(argv[0]);
	if (!init_robot_config()) {
		return -1;
	}
	RunRobots(cfg_root);

	return 0;
}

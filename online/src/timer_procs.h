#ifndef TIMER_PROCS_H
#define TIMER_PROCS_H

#define VP_ADD_GAP  (720)
#define VP_ADD_VAL (2)

enum timer_type_t
{
    kTimerTypeReconnectServiceTimely = 1, // 重连service服务器
    kTimerTypeSyncClientTimeTimely = 2, // 同步客户端时间
    kTimerTypeCheckSvrTimeout = 3, // 服务器超时
    kTimerTypeDailyOperation = 4,      // 每日操作
    kTimerTypeCleanExpiredItemsTimely = 5, // 定期清理用户过期道具
    kTimerTypeSysNotiOffline = 6, //系统定时在23:50分发布服务器即将关闭的通知
    kTimerTypeCheckDbDiamondTimely = 7, //检测钻石的DB请求是否正常返回
    kTimerTypeSwRegTimely = 8, //即时注册到switch
    kTimerTypeVpFlush = 9, //及时恢复体力
    kTimerTypeShopFlush = 10, //及时刷新商店物品
	kTimerTypeTestEscortFinish = 11,	//探测本服运宝系统是否有玩家完成运宝
    kTimerTypeClearDive = 12, //强制从跳水队列中清除
    kTimerTypeVpAdd = 13, //体力恢复定时器
	kTimerTypeSendArenaWeeklyReward = 14, //邮件发送竞技场周排名奖励
	kTimerTypeExercisePetAddExp = 15,	//锻炼的精灵加经验(已不使用)
    kTimerTypeWorldBossCheck    = 16,   // 世界boss阶段控制定时器
    kTimerTypeDailyKickOff = 17, //每日定点T下线的定时器操作
    kTimerTypeDailyResetDive = 18, //每日0点清理跳水排行
	kTimerTypeSendArenaDailyReward = 19,	//邮件发送竞技场日排名奖励
	kTimerTypeSendDiamondRechargeReward = 20,	//邮件发送冲钻排名奖励
	kTimerTypeSendOpenSrvPowerReward = 22,		//邮件发送限时战力榜奖励
	kTimerTypeSendGoldConsumeReward = 23,		//邮件发送限时金币消耗榜奖励
	kTimerTypeWeeklyActivityRankReward = 24,		//邮件发送限时金币消耗榜奖励
    kTimerTypeDaily21DumpRank = 25, //每日21点dump排名服
};

enum timer_interval_t
{
    kTimerIntervalReconnectServiceTimely = 1, // 重连service服务器间隔
    kTimerIntervalSyncClientTimeTimely = 30, // 同步客户端时间
    kTimerIntervalCheckSvrTimeout = 10, // Svr超时时间
    kTimerIntervalCleanExpiredItems = 10 * 60, // 定期清理用户过期道具
    kTimerIntervalDbDiamondTimely = 5, //钻石超时时间
    kTimerIntervalSwRegTimely = 3, //switch注册的超时时间
	kTimerIntervalTestEscort = 10, //每隔10秒探测一次运宝
	kTimerIntervalExercPetAddExp = 2 * 60,	//锻炼着的精灵每两分钟加一次经验
    kTimerIntervalWorldBoss     =   15,     // 世界boss阶段检查间隔
};

int register_timers();

int daily_00_dump_rank_utils();

int weekly_00_dump_rank_utils();

#endif

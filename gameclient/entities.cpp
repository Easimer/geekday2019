#include "entities.h"
#include <vector>
#include <mutex>

#define DEFINE_TABLE(type) static std::vector< Entity_##type > g##type##Data
#define DEFINE_TABLE_GETTER(type) Entity_##type * Get##type##Idx(int idx) { \
    return (idx < g##type##Data.size()) ? &g##type##Data[idx] : (Entity_##type *)NULL; \
}

static std::vector<Entity_Common> gCommonData;
DEFINE_TABLE(Player);
DEFINE_TABLE(Rocket);
DEFINE_TABLE(Mine);
DEFINE_TABLE(Pickup);

DEFINE_TABLE_GETTER(Player);
DEFINE_TABLE_GETTER(Rocket);
DEFINE_TABLE_GETTER(Mine);
DEFINE_TABLE_GETTER(Pickup);

static std::mutex gLock;

void LockEntitySystem() {
    gLock.lock();
}

void UnlockEntitySystem() {
    gLock.unlock();
}

Entity_Player* GetPlayerByID(int id) {
    Entity_Player* ret = NULL;

    for (auto& p : gPlayerData) {
        if (p.common.playerID == id) {
            ret = &p;
        }
    }

    return ret;
}

int CreatePlayer() {
    int ret = gPlayerData.size();
    Entity_Player p;

    memset(&p, 0, sizeof(p));
    gPlayerData.push_back(p);

    return ret;
}

void ClearPrimaryEntities() {
    gPlayerData.clear();
    gMineData.clear();
    gRocketData.clear();
}

void ClearSecondaryEntities() {
    gPickupData.clear();
}


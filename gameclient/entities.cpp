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


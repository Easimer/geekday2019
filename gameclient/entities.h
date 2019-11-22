#ifndef GAMECLIENT_ENTITIES_H
#define GAMECLIENT_ENTITIES_H

using Entity_ID = int;

struct Entity_Common {
    unsigned posX, posY;
    int playerID;
    int speed;
};

struct Entity_Player {
    Entity_Common common;
    int nextCheckpointID;
    int healthPoints;
    int countMines;
    int countRockets;
    int angle;
    int desiredAngle;
};

struct Entity_Rocket {
    Entity_Common common;
};

struct Entity_Mine {
    Entity_Common common;
};

enum Pickup_Kind {
    k_PickupInvalid = 0,
    k_PickupMine = 1,
    k_PickupRocket = 2,
    k_PickupHealth = 3,
    k_PickupLast
};

struct Entity_Pickup {
    Entity_Common common;
    Pickup_Kind kind;
};

void LockEntitySystem();
void UnlockEntitySystem();

#define DECLARE_ENTITY_GETTER(type) Entity_##type * Get##type##Idx(int idx)
#define DECLARE_ENTITY_CREATE(type) int Create##type()
#define DECLARE_ENTITY_DELETE(type) void Delete##type##Idx(int idx)

#define DECLARE_ENTITY_OPS(type) \
DECLARE_ENTITY_GETTER(type); \
DECLARE_ENTITY_CREATE(type); \
DECLARE_ENTITY_DELETE(type); \

DECLARE_ENTITY_OPS(Player);
DECLARE_ENTITY_OPS(Rocket);
DECLARE_ENTITY_OPS(Mine);
DECLARE_ENTITY_OPS(Pickup);

#endif /* GAMECLIENT_ENTITIES_H */

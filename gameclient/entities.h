#ifndef GAMECLIENT_ENTITIES_H
#define GAMECLIENT_ENTITIES_H

using Entity_ID = int;

struct Entity_Common {
    bool used;
    unsigned posX, posY;
};

struct Entity_Player {
    int nextCheckpointID;
    int healthPoints;
    int speed;
    int countMines;
    int countRockets;
    int angle;
    int desiredAngle;
};


struct Entity_Rocket {
    Entity_ID id;
};

struct Entity_Mine {
    Entity_ID id;
};

enum Pickup_Kind {
    k_PickupInvalid = 0,
    k_PickupMine = 1,
    k_PickupRocket = 2,
    k_PickupHealth = 3,
    k_PickupLast
};

struct Entity_Pickup {
    Entity_ID id;
    Pickup_Kind kind;
};

void LockEntitySystem();
void UnlockEntitySystem();

Entity_Common* GetEntity(Entity_ID entity);

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

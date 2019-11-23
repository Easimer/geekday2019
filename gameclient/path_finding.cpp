#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <assert.h>
#include "path_finding.h"

#define WALLDIST_COST (20)

using PFPoint = std::tuple<int, int>;

struct PFPointHash {
    std::size_t operator()(const PFPoint& p) const {
        return std::get<0>(p) ^ std::get<1>(p);
    }
};

using PFSet = std::unordered_set<PFPoint, PFPointHash>;
using PFPointMap = std::unordered_map<PFPoint, PFPoint, PFPointHash>;
using PFPointCostMap = std::unordered_map<PFPoint, int, PFPointHash>;

static int& CheapestPathCostTo(PFPointCostMap& map, const PFPoint& point) {
    if (!map.count(point)) {
        map[point] = INT_MAX;
    }
    return map[point];
}


static std::vector<PFPoint> Neighbors(const Level_Mask* pLevel, const PFPoint& point) {
    std::vector<PFPoint> ret;
    auto pX = std::get<0>(point);
    auto pY = std::get<1>(point);
    if(LevelMaskInBounds(pLevel, pX + 1, pY))
        ret.push_back({ pX + 1, pY });
    if(LevelMaskInBounds(pLevel, pX - 1, pY))
        ret.push_back({ pX - 1, pY });
    if(LevelMaskInBounds(pLevel, pX, pY + 1))
        ret.push_back({ pX, pY + 1 });
    if(LevelMaskInBounds(pLevel, pX, pY - 1))
        ret.push_back({ pX, pY - 1 });
    return ret;
}

static std::vector<PFPoint> Neighbors(const LevelBlocks* pLevel, const PFPoint& point, bool useDarkNodes) {
    std::vector<PFPoint> ret;
    auto pX = std::get<0>(point);
    auto pY = std::get<1>(point);
	if (useDarkNodes) {
		if (LevelBlocksInStrictBounds(pLevel, pX + 1, pY))
		ret.push_back({ pX + 1, pY });
		if (LevelBlocksInStrictBounds(pLevel, pX - 1, pY))
		ret.push_back({ pX - 1, pY });
		if (LevelBlocksInStrictBounds(pLevel, pX, pY + 1))
		ret.push_back({ pX, pY + 1 });
		if (LevelBlocksInStrictBounds(pLevel, pX, pY - 1))
		ret.push_back({ pX, pY - 1 });
	}
	else {
		if (LevelBlocksInBounds(pLevel, pX + 1, pY))
		ret.push_back({ pX + 1, pY });
		if (LevelBlocksInBounds(pLevel, pX - 1, pY))
		ret.push_back({ pX - 1, pY });
		if (LevelBlocksInBounds(pLevel, pX, pY + 1))
		ret.push_back({ pX, pY + 1 });
		if (LevelBlocksInBounds(pLevel, pX, pY - 1))
		ret.push_back({ pX, pY - 1 });
	}
    return ret;
}

static PFPoint LowestFScoreInOpenset(PFPointCostMap& fScore, const PFSet& openSet) {
    PFPoint ret;
    int minCost = INT_MAX;
    int D = 0;

    for (auto& point : openSet) {
        int cost = CheapestPathCostTo(fScore, point);
        if (cost < minCost) {
            ret = point;
            minCost = cost;
            D++;
        }
    }
    assert(D > 0);

    return ret;
}

static Path_Node* NodeFrom(const PFPoint& p) {
    auto ret = new Path_Node;

    ret->x = std::get<0>(p);
    ret->y = std::get<1>(p);
    ret->next = NULL;

    return ret;
}

static Path_Node* PrependToPath(Path_Node* path, const PFPoint& current) {
    Path_Node* nu = NodeFrom(current);

    nu->next = path;

    return nu;
}

PFPoint Scale(const PFPoint& p, int v) {
    return { std::get<0>(p) * v, std::get<1>(p) * v };
}

Path_Node* ReconstructPath(const PFPointMap& cameFrom, PFPoint current) {
    Path_Node* ret = NodeFrom(Scale(current, 20)); // TODO: clusterscale
    while (cameFrom.count(current)) {
        current = cameFrom.at(current);
        ret = PrependToPath(ret, Scale(current, 20)); // TODO: clusterscale
    }
    return ret;
}

static int DistanceSquared(int x0, int y0, int x1, int y1) {
    return (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
}

Path_Node* CalculatePath(
    const LevelBlocks* level,
    int startX, int startY,
    int destX, int destY,
    bool calcDarkCost) {
    assert(level);

    auto H = [&](const PFPoint& p, int destX, int destY) {
		int deltaX = std::get<0>(p) - destX;
		int deltaY = std::get<1>(p) - destY;
        return sqrtf(deltaX * deltaX + deltaY * deltaY) / 10;
    };
    startX /= 20; // TODO: clusterscale
    startY /= 20; // TODO: clusterscale
    destX /= 20; // TODO: clusterscale
    destY /= 20; // TODO: clusterscale

    PFPoint start = { startX, startY };
    PFPoint goal = { destX, destY };

    PFSet openSet = { start };
    PFPointMap cameFrom;
    PFPointCostMap gScore;
    PFPointCostMap fScore;

    gScore[start] = 0;
    fScore[start] = H(start, destX, destY);

    while (openSet.size() > 0) {
        auto current = LowestFScoreInOpenset(fScore, openSet);
        if (current == goal) {
            return ReconstructPath(cameFrom, current);
        }

        openSet.erase(current);
        int curX = std::get<0>(current);
        int curY = std::get<1>(current);

        for (auto neighbor : Neighbors(level, current, calcDarkCost)) {
            int tentativeGScore = CheapestPathCostTo(gScore, current) + (WALLDIST_COST - LevelBlockDistanceFromWall(level, curX, curY)) * 2;
            if (calcDarkCost) {
                if (!LevelBlocksInBounds(level, curX, curY)) {
                    tentativeGScore += 60;
                }
            }
            //int tentativeGScore = CheapestPathCostTo(gScore, current) + 1;
            if (tentativeGScore < CheapestPathCostTo(gScore, neighbor)) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeGScore;
                fScore[neighbor] = tentativeGScore + H(neighbor, destX, destY);
                if (openSet.count(neighbor) == 0) {
                    openSet.insert(neighbor);
                }
            }
        }
    }

    return NULL;
}

bool IsPointBehindMe(int myX, int myY, int dirX, int dirY, int pX, int pY) {
    int dot = dirX * (pX - myX) + dirY * (pY - myX);
    return dot < 0;
}

Path_Node* PathNodeFreeSingle(Path_Node* node) {
    Path_Node* ret = NULL;
    assert(node);

    ret = node->next;
    delete node;

    return ret;
}

void PathNodeFree(Path_Node* node) {
    auto cur = node;
    auto next = cur;
    while (cur) {
        next = cur->next;
        delete cur;
        cur = next;
    }
}
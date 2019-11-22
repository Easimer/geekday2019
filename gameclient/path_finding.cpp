#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <assert.h>
#include "path_finding.h"

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


static std::vector<PFPoint> Neighbors(const PFPoint& point) {
    std::vector<PFPoint> ret;
    ret.push_back({ std::get<0>(point) + 1, std::get<1>(point) });
    ret.push_back({ std::get<0>(point) - 1, std::get<1>(point) });
    ret.push_back({ std::get<0>(point), std::get<1>(point) + 1 });
    ret.push_back({ std::get<0>(point), std::get<1>(point) - 1 });
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

Path_Node* ReconstructPath(const PFPointMap& cameFrom, const PFPoint& current) {
    Path_Node* ret = NodeFrom(current);
    while (cameFrom.count(current)) {

    }
}

Path_Node* CalculatePath(
    const Level_Mask* level,
    int startX, int startY,
    int destX, int destY) {
    assert(level);

    auto H = [&](const PFPoint& p) {
        return 0;
    };

    PFPoint start = { startX, startY };
    PFPoint goal = { destX, destY };

    PFSet openSet = { start };
    PFPointMap cameFrom;
    PFPointCostMap gScore;
    PFPointCostMap fScore;

    gScore[start] = 0;
    fScore[start] = H(start);

    while (openSet.size() > 0) {
        auto current = LowestFScoreInOpenset(fScore, openSet);
        if (current == goal) {
            return ReconstructPath(cameFrom, current);
        }

        openSet.erase(current);

        for (auto neighbor : Neighbors(current)) {
            int tentativeGScore = CheapestPathCostTo(gScore, current) + 1;
            if (tentativeGScore < CheapestPathCostTo(gScore, neighbor)) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeGScore;
                fScore[neighbor] = tentativeGScore + H(neighbor);
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
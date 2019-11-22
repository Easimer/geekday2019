#ifndef GAMECLIENT_SOAP_H
#define GAMECLIENT_SOAP_H

#include <vector>
#include <string>

struct GetCheckpointsResponse {
    struct Line {
        float x0, y0;
        float x1, y1;
    };
    std::vector<Line> lines;
};

GetCheckpointsResponse GetCheckpoints(const std::string& trackID);

#endif /* GAMECLIENT_SOAP_H */

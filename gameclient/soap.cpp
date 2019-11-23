#include <assert.h>
#include <string.h>
#include <string>

#include <httplib.h>
#include "tinyxml2.h"
using namespace tinyxml2;

#include "soap.h"

const char* pchGetCheckpointsTemplate = " \
<s11:Envelope xmlns:s11 = 'http://schemas.xmlsoap.org/soap/envelope/'> \
<s11:Body> \
<ns1:GetCheckpoints xmlns:ns1='http://tempuri.org/'> \
<ns1:levelId>%s</ns1:levelId> \
</ns1:GetCheckpoints> \
</s11:Body> \
</s11:Envelope> \
";

static std::string GenerateSOAPRequest(const std::string& trackID) {
    int res;
    char buffer[512];

    res = snprintf(buffer, 512, pchGetCheckpointsTemplate, trackID.c_str());
    assert(res <= 511);

    return std::string(buffer);
}

static const XMLNode* FindResponseInXML(XMLNode* node, tinyxml2::XMLDocument* pDoc) {
    const XMLNode* ret = NULL;

    if (node) {
        ret = node->FirstChildElement("s:Body");
        if (ret) {
            ret = ret->FirstChildElement("GetCheckpointsResponse");
            if (ret) {
                ret = ret->FirstChildElement("GetCheckpointsResult");
                if (ret) {
                    auto elem = ret->ToElement();
                    auto pFaszom = elem->GetText();
                    pDoc->Parse(pFaszom, strlen(pFaszom));
                } else {
                    ret = NULL;
                }
            } else {
                ret = NULL;
            }
        } else {
            ret = NULL;
        }
    }

    return ret;
}

static std::vector<GetCheckpointsResponse::Line>
GetLines(const std::string& xml) {
    std::vector<GetCheckpointsResponse::Line> ret;
    tinyxml2::XMLError res;

    tinyxml2::XMLDocument doc, docInner;

    res = doc.Parse(xml.c_str(), xml.size());

    FindResponseInXML(doc.FirstChildElement("s:Envelope"), &docInner);
    auto pLine = docInner.RootElement()->FirstChildElement("line");
    while (pLine) {
        float x0 = std::stof(pLine->Attribute("x1"));
        float y0 = std::stof(pLine->Attribute("y1"));
        float x1 = std::stof(pLine->Attribute("x2"));
        float y1 = std::stof(pLine->Attribute("y2"));
        //fprintf(stderr, "%f %f %f %f\n", x0, y0, x1, y1);

        ret.push_back({x0, y0, x1, y1});
        pLine = pLine->NextSiblingElement("line");
    }

    return ret;
}

GetCheckpointsResponse GetCheckpoints(const std::string& trackID) {
    GetCheckpointsResponse ret;
    httplib::Client cli("192.168.1.20", 8888);
    httplib::Headers hdrs = {
        {"SOAPAction", "http://tempuri.org/ICarService/GetCheckpoints"},
    };

    auto content = GenerateSOAPRequest(trackID);

    auto res = cli.Post("/DrService/", hdrs, content, "text/xml");
    if (res) {
        assert(res->status == 200);

        auto& xml = res->body;

        ret.lines = std::move(GetLines(xml));
    }

    return ret;
}

#include <httpsrv/srv.h>

int main(int argc, char** argv) {
    auto hHTTPSrv = HTTPServer_Create(8000);

    while (true);

    HTTPServer_Shutdown(hHTTPSrv);
    return 0;
}
#include "web_server/web_server.h"
#include <mysql/mysql.h>
#include "database/mysql_conn.h"

int main(){
    tiny_web_server server;
    server.run();


    return 0;
}
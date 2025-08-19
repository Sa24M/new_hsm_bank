#include <iostream>
#include <mysql/mysql.h>
#include <string>
#include <cstdlib>
using namespace std;

int main() {
    const char* qs = getenv("QUERY_STRING");
    if (!qs) return 0;
    string qsstr = qs;
    size_t pos = qsstr.find("accno=");
    if (pos == string::npos) return 0;
    int accno = stoi(qsstr.substr(pos+6));

    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "127.0.0.1", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
        return 0;
    }
    string query = "SELECT Balance FROM balance WHERE AccNo=" + to_string(accno);
    mysql_query(conn, query.c_str());
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    if ((row = mysql_fetch_row(res))) {
        cout << "Content-Type: text/plain\r\n\r\n";
        cout << (row[0] ? row[0] : "0");
    }
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

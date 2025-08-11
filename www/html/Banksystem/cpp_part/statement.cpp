#include <iostream>
#include <cstdlib>
#include <string>
#include <mysql/mysql.h>
using namespace std;

int main() {
    cout << "Content-Type: application/json\r\n\r\n";

    const char* qs = getenv("QUERY_STRING");
    if (!qs) { cout << "[]"; return 0; }
    string querystr = qs;
    size_t pos = querystr.find("accno=");
    if (pos == string::npos) { cout << "[]"; return 0; }
    string accno_str = querystr.substr(pos + 6);
    if (accno_str.empty()) { cout << "[]"; return 0; }
    int accno = stoi(accno_str);

    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "127.0.0.1", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
        cout << "[]";
        return 0;
    }

    // Query transactions where user is sender OR receiver, newest first
    string q = "SELECT Sender, Receiver, Amount, Remarks, DATE_FORMAT(DateTime, '%Y-%m-%d %H:%i:%s') as DT, SenBalance, RecBalance FROM transactions "
               "WHERE Sender=" + to_string(accno) + " OR Receiver=" + to_string(accno) + 
               " ORDER BY DateTime DESC";

    if (mysql_query(conn, q.c_str()) != 0) {
        cout << "[]";
        mysql_close(conn);
        return 0;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    cout << "[";
    bool first = true;
    while ((row = mysql_fetch_row(res))) {
        if (!first) cout << ",";
        cout << "{";
        cout << "\"Sender\":\"" << (row[0] ? row[0] : "") << "\",";
        cout << "\"Receiver\":\"" << (row[1] ? row[1] : "") << "\",";
        cout << "\"Amount\":\"" << (row[2] ? row[2] : "0") << "\",";
        cout << "\"Remarks\":\"" << (row[3] ? row[3] : "") << "\",";
        cout << "\"DateTime\":\"" << (row[4] ? row[4] : "") << "\",";
        cout << "\"SenBalance\":\"" << (row[5] ? row[5] : "0") << "\",";
        cout << "\"RecBalance\":\"" << (row[6] ? row[6] : "0") << "\"";
        cout << "}";
        first = false;
    }
    cout << "]";
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

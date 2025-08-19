#include <iostream>
#include <mysql/mysql.h>
#include <string>
#include <cstdlib>
using namespace std;

int main() {
    cout << "Content-Type: application/json\r\n\r\n";

    const char* qs = getenv("QUERY_STRING");
    if (!qs) { cout << "[]"; return 0; }
    string qsstr = qs;
    size_t pos = qsstr.find("accno=");
    if (pos == string::npos) { cout << "[]"; return 0; }
    int accno = stoi(qsstr.substr(pos+6));

    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "127.0.0.1", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
        cout << "[]"; return 0;
    }

    string query = "SELECT DateTime, Remarks, Amount, Sender, Receiver "
                   "FROM transactions WHERE Sender=" + to_string(accno) + 
                   " OR Receiver=" + to_string(accno) +
                   " ORDER BY DateTime DESC LIMIT 10";
    mysql_query(conn, query.c_str());
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    cout << "[";
    bool first = true;
    while ((row = mysql_fetch_row(res))) {
        if (!first) cout << ",";
        first = false;
        string date = row[0] ? row[0] : "";
        string remarks = row[1] ? row[1] : "";
        double amount = row[2] ? atof(row[2]) : 0;
        int sender = row[3] ? atoi(row[3]) : 0;
        int receiver = row[4] ? atoi(row[4]) : 0;

        cout << "{";
        cout << "\"DateTime\":\"" << date << "\",";
        cout << "\"Remarks\":\"" << remarks << "\",";
        cout << "\"Amount\":" << amount << ",";
        cout << "\"Sender\":" << sender << ",";
        cout << "\"Receiver\":" << receiver;
        cout << "}";
    }
    cout << "]";
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

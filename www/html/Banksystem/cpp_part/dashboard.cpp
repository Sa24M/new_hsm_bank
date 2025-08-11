#include <iostream>
#include <mysql/mysql.h>
#include <sstream>
#include <string>
#include <cstdlib>
using namespace std;

int main() {
    cout << "Content-Type: application/json\r\n\r\n";

    const char* qs = getenv("QUERY_STRING");
    if (!qs) { cout << "{}"; return 0; }

    string accno_str;
    string querystr = qs;
    size_t pos = querystr.find("accno=");
    if (pos != string::npos) accno_str = querystr.substr(pos + 6);
    if (accno_str.empty()) { cout << "{}"; return 0; }
    int accno = stoi(accno_str);

    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "127.0.0.1", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
        cout << "{}"; return 0;
    }

    string name;
    double balance=0, interest=0, credited=0, debited=0;

    MYSQL_RES *res;
    MYSQL_ROW row;
    // Get name, balance, interest
    {
        string q = "SELECT u.Name,b.Balance,b.Interest FROM userinfo u "
                   "JOIN balance b ON u.AccNo=b.AccNo WHERE u.AccNo=" + to_string(accno);
        mysql_query(conn, q.c_str());
        res = mysql_store_result(conn);
        if ((row = mysql_fetch_row(res))) {
            name = row[0] ? row[0] : "";
            balance = row[1] ? atof(row[1]) : 0;
            interest = row[2] ? atof(row[2]) : 0;
        }
        mysql_free_result(res);
    }
    // Total credited
    {
        string q = "SELECT IFNULL(SUM(Amount),0) FROM transactions WHERE Receiver=" + to_string(accno);
        mysql_query(conn, q.c_str());
        res = mysql_store_result(conn);
        if ((row = mysql_fetch_row(res))) credited = row[0] ? atof(row[0]) : 0;
        mysql_free_result(res);
    }
    // Total debited
    {
        string q = "SELECT IFNULL(SUM(Amount),0) FROM transactions WHERE Sender=" + to_string(accno);
        mysql_query(conn, q.c_str());
        res = mysql_store_result(conn);
        if ((row = mysql_fetch_row(res))) debited = row[0] ? atof(row[0]) : 0;
        mysql_free_result(res);
    }

    mysql_close(conn);

    cout << "{";
    cout << "\"name\":\"" << name << "\",";
    cout << "\"balance\":" << balance << ",";
    cout << "\"interest\":" << interest << ",";
    cout << "\"credited\":" << credited << ",";
    cout << "\"debited\":" << debited;
    cout << "}";
    return 0;
}

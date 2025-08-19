#include <iostream>
#include <mysql/mysql.h>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <iomanip>

using namespace std;

// URL decode function for safer parsing
string url_decode(const string &s) {
    string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') out.push_back(' ');
        else if (s[i] == '%' && i + 2 < s.size()) {
            char hex[3] = {s[i+1], s[i+2], '\0'};
            out.push_back((char)strtol(hex, nullptr, 16));
            i += 2;
        } else out.push_back(s[i]);
    }
    return out;
}

int main() {
    cout << "Content-Type: application/json\r\n\r\n";

    const char* qs = getenv("QUERY_STRING");
    if (!qs) {
        cout << "{}";
        return 0;
    }

    string accno_str;
    string querystr = qs;
    size_t pos = querystr.find("accno=");
    if (pos != string::npos) {
        accno_str = url_decode(querystr.substr(pos + 6));
    }

    if (accno_str.empty()) {
        cout << "{}";
        return 0;
    }

    int accno;
    try {
        accno = stoi(accno_str);
    } catch (...) {
        cout << "{}";
        return 0;
    }

        MYSQL *conn = mysql_init(NULL);
        if (!mysql_real_connect(conn, "127.0.0.1", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
            cout << "{}";
            return 0;
        }

    // Initialize all variables to a default value
    string name = "";
    double balance = 0, interest = 0, credited = 0, debited = 0;

    MYSQL_STMT *stmt;
    MYSQL_BIND bind[1];
    MYSQL_BIND result_bind[3];
    MYSQL_RES *res_meta;

    // Get name, balance, interest using a prepared statement
    stmt = mysql_stmt_init(conn);
    if (stmt) {
        const char *query = "SELECT u.Name, b.Balance, b.Interest FROM userinfo u "
                            "JOIN balance b ON u.AccNo = b.AccNo WHERE u.AccNo = ?";
        if (mysql_stmt_prepare(stmt, query, strlen(query)) == 0) {
            memset(bind, 0, sizeof(bind));
            bind[0].buffer_type = MYSQL_TYPE_LONG;
            bind[0].buffer = (char *)&accno;
            mysql_stmt_bind_param(stmt, bind);
            mysql_stmt_execute(stmt);
            
            res_meta = mysql_stmt_result_metadata(stmt);
            if (res_meta) {
                memset(result_bind, 0, sizeof(result_bind));
                char name_buffer[256];
                double balance_buffer, interest_buffer;
                unsigned long name_len;
                
                result_bind[0].buffer_type = MYSQL_TYPE_STRING;
                result_bind[0].buffer = (char *)name_buffer;
                result_bind[0].buffer_length = sizeof(name_buffer);
                result_bind[0].length = &name_len;
                
                result_bind[1].buffer_type = MYSQL_TYPE_DOUBLE;
                result_bind[1].buffer = (char *)&balance_buffer;
                
                result_bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
                result_bind[2].buffer = (char *)&interest_buffer;
                
                mysql_stmt_bind_result(stmt, result_bind);
                
                if (mysql_stmt_fetch(stmt) == 0) {
                    name.assign(name_buffer, name_len);
                    balance = balance_buffer;
                    interest = interest_buffer;
                }
                mysql_free_result(res_meta);
            }
        }
        mysql_stmt_close(stmt);
    }

    // Total credited using a prepared statement
    stmt = mysql_stmt_init(conn);
    if (stmt) {
        const char *query = "SELECT IFNULL(SUM(Amount),0) FROM transactions WHERE Receiver = ?";
        if (mysql_stmt_prepare(stmt, query, strlen(query)) == 0) {
            memset(bind, 0, sizeof(bind));
            bind[0].buffer_type = MYSQL_TYPE_LONG;
            bind[0].buffer = (char *)&accno;
            mysql_stmt_bind_param(stmt, bind);
            mysql_stmt_execute(stmt);
            
            double credited_buffer;
            memset(result_bind, 0, sizeof(result_bind));
            result_bind[0].buffer_type = MYSQL_TYPE_DOUBLE;
            result_bind[0].buffer = (char *)&credited_buffer;
            mysql_stmt_bind_result(stmt, result_bind);
            if (mysql_stmt_fetch(stmt) == 0) {
                credited = credited_buffer;
            }
        }
        mysql_stmt_close(stmt);
    }

    // Total debited using a prepared statement
    stmt = mysql_stmt_init(conn);
    if (stmt) {
        const char *query = "SELECT IFNULL(SUM(Amount),0) FROM transactions WHERE Sender = ?";
        if (mysql_stmt_prepare(stmt, query, strlen(query)) == 0) {
            memset(bind, 0, sizeof(bind));
            bind[0].buffer_type = MYSQL_TYPE_LONG;
            bind[0].buffer = (char *)&accno;
            mysql_stmt_bind_param(stmt, bind);
            mysql_stmt_execute(stmt);
            
            double debited_buffer;
            memset(result_bind, 0, sizeof(result_bind));
            result_bind[0].buffer_type = MYSQL_TYPE_DOUBLE;
            result_bind[0].buffer = (char *)&debited_buffer;
            mysql_stmt_bind_result(stmt, result_bind);
            if (mysql_stmt_fetch(stmt) == 0) {
                debited = debited_buffer;
            }
        }
        mysql_stmt_close(stmt);
    }

    mysql_close(conn);

    cout << "{";
    cout << "\"name\":\"" << name << "\",";
    cout << "\"balance\":" << fixed << setprecision(2) << balance << ",";
    cout << "\"interest\":" << fixed << setprecision(2) << interest << ",";
    cout << "\"credited\":" << fixed << setprecision(2) << credited << ",";
    cout << "\"debited\":" << fixed << setprecision(2) << debited;
    cout << "}";

    return 0;
}
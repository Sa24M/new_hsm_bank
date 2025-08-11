#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <openssl/sha.h>
#include <mysql/mysql.h>
using namespace std;

void trim(string &str) {
    str.erase(str.begin(), find_if(str.begin(), str.end(),
        [](unsigned char ch) { return !isspace(ch); }));
    str.erase(find_if(str.rbegin(), str.rend(),
        [](unsigned char ch) { return !isspace(ch); }).base(), str.end());
}

string url_decode(const string &s) {
    string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') out.push_back(' ');
        else if (s[i] == '%' && i + 2 < s.size()) {
            char hex[3] = { s[i+1], s[i+2], '\0' };
            out.push_back((char)strtol(hex, nullptr, 16));
            i += 2;
        } else out.push_back(s[i]);
    }
    return out;
}

bool read_post(string &body) {
    const char *len_str = getenv("CONTENT_LENGTH");
    if (!len_str) return false;
    int len = atoi(len_str);
    if (len <= 0) return false;
    body.resize(len);
    cin.read(&body[0], len);
    return true;
}

string sha256hex(const string &input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.size(), hash);
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << setw(2) << (int)hash[i];
    return ss.str();
}

string urlencode_msg(const string &s) {
    ostringstream oss;
    oss << hex << setfill('0');
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            oss << c;
        else if (c == ' ')
            oss << '+';
        else
            oss << '%' << setw(2) << (int)c;
    }
    return oss.str();
}

int main() {
    const string REGISTER_PAGE = "/Banksystem/htmlpart/register.html";
    const string LOGIN_PAGE = "/Banksystem/htmlpart/login.html";

    string body;
    if (!read_post(body)) {
        cout << "Status: 303 See Other\r\nLocation: " << REGISTER_PAGE 
             << "?msg=" << urlencode_msg("No data received") << "\r\n\r\n";
        return 0;
    }

    string name, address, email, password, confirm_password, deposit_str;
    size_t pos = 0;
    while (pos < body.size()) {
        size_t amp = body.find('&', pos);
        string token = (amp == string::npos) ? body.substr(pos) : body.substr(pos, amp - pos);
        size_t eq = token.find('=');
        if (eq != string::npos) {
            string key = url_decode(token.substr(0, eq));
            string val = url_decode(token.substr(eq + 1));
            for (char &c : key) c = tolower((unsigned char)c);
            if (key == "fullname") name = val;
            else if (key == "address") address = val;
            else if (key == "email") email = val;
            else if (key == "password") password = val;
            else if (key == "confirm_password") confirm_password = val;
            else if (key == "initial_deposit") deposit_str = val;
        }
        if (amp == string::npos) break;
        pos = amp + 1;
    }

    trim(name); trim(address); trim(email);
    trim(password); trim(confirm_password); trim(deposit_str);

    if (name.empty() || address.empty() || email.empty() || password.empty() || confirm_password.empty() || deposit_str.empty()) {
        cout << "Status: 303 See Other\r\nLocation: " << REGISTER_PAGE 
             << "?msg=" << urlencode_msg("Missing fields") << "\r\n\r\n";
        return 0;
    }
    if (password != confirm_password) {
        cout << "Status: 303 See Other\r\nLocation: " << REGISTER_PAGE 
             << "?msg=" << urlencode_msg("Passwords do not match") << "\r\n\r\n";
        return 0;
    }

    double initial_deposit = stod(deposit_str);
    if (initial_deposit < 0) initial_deposit = 0;
    string hashed_pass = sha256hex(password);

    MYSQL *conn = mysql_init(nullptr);
    if (!conn || !mysql_real_connect(conn, "127.0.0.1", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
        if (conn) mysql_close(conn);
        cout << "Status: 303 See Other\r\nLocation: " << REGISTER_PAGE 
             << "?msg=" << urlencode_msg("DB connection failed") << "\r\n\r\n";
        return 0;
    }

    mysql_query(conn, "START TRANSACTION");

    // credentials
    MYSQL_STMT *stmt1 = mysql_stmt_init(conn);
    const char *sql1 = "INSERT INTO credentials (Pass) VALUES (?)";
    mysql_stmt_prepare(stmt1, sql1, strlen(sql1));
    MYSQL_BIND bind1[1] = {};
    bind1[0].buffer_type = MYSQL_TYPE_STRING;
    bind1[0].buffer = (char*)hashed_pass.c_str();
    unsigned long pass_len = hashed_pass.size();
    bind1[0].length = &pass_len;
    mysql_stmt_bind_param(stmt1, bind1);
    if (mysql_stmt_execute(stmt1) != 0) {
        mysql_query(conn, "ROLLBACK");
        mysql_stmt_close(stmt1);
        mysql_close(conn);
        return 0;
    }
    int accno = mysql_insert_id(conn);
    mysql_stmt_close(stmt1);

    // userinfo
    MYSQL_STMT *stmt2 = mysql_stmt_init(conn);
    const char *sql2 = "INSERT INTO userinfo (AccNo, Name, Address, Email) VALUES (?, ?, ?, ?)";
    mysql_stmt_prepare(stmt2, sql2, strlen(sql2));
    MYSQL_BIND bind2[4] = {};
    bind2[0].buffer_type = MYSQL_TYPE_LONG; bind2[0].buffer = &accno;
    bind2[1].buffer_type = MYSQL_TYPE_STRING; bind2[1].buffer = (char*)name.c_str(); unsigned long nlen = name.size(); bind2[1].length = &nlen;
    bind2[2].buffer_type = MYSQL_TYPE_STRING; bind2[2].buffer = (char*)address.c_str(); unsigned long alen = address.size(); bind2[2].length = &alen;
    bind2[3].buffer_type = MYSQL_TYPE_STRING; bind2[3].buffer = (char*)email.c_str(); unsigned long elen = email.size(); bind2[3].length = &elen;
    mysql_stmt_bind_param(stmt2, bind2);
    if (mysql_stmt_execute(stmt2) != 0) {
        mysql_query(conn, "ROLLBACK");
        mysql_stmt_close(stmt2);
        mysql_close(conn);
        return 0;
    }
    mysql_stmt_close(stmt2);

    // balance
    MYSQL_STMT *stmt3 = mysql_stmt_init(conn);
    const char *sql3 = "INSERT INTO balance (AccNo, Balance, Interest) VALUES (?, ?, 0)";
    mysql_stmt_prepare(stmt3, sql3, strlen(sql3));
    MYSQL_BIND bind3[2] = {};
    bind3[0].buffer_type = MYSQL_TYPE_LONG; bind3[0].buffer = &accno;
    bind3[1].buffer_type = MYSQL_TYPE_DOUBLE; bind3[1].buffer = &initial_deposit;
    mysql_stmt_bind_param(stmt3, bind3);
    if (mysql_stmt_execute(stmt3) != 0) {
        mysql_query(conn, "ROLLBACK");
        mysql_stmt_close(stmt3);
        mysql_close(conn);
        return 0;
    }
    mysql_stmt_close(stmt3);

    // Signup Bonus transaction
    MYSQL_STMT *stmt4 = mysql_stmt_init(conn);
    const char *sql4 = "INSERT INTO transactions (Sender, Receiver, Amount, Remarks, SenBalance, RecBalance) VALUES (?, ?, ?, ?, ?, ?)";
    mysql_stmt_prepare(stmt4, sql4, strlen(sql4));
    MYSQL_BIND bind4[6] = {};
    int sender = 0;
    double zero = 0;
    string remarks = "Signup Bonus";
    bind4[0].buffer_type = MYSQL_TYPE_LONG; bind4[0].buffer = &sender;
    bind4[1].buffer_type = MYSQL_TYPE_LONG; bind4[1].buffer = &accno;
    bind4[2].buffer_type = MYSQL_TYPE_DOUBLE; bind4[2].buffer = &initial_deposit;
    bind4[3].buffer_type = MYSQL_TYPE_STRING; bind4[3].buffer = (char*)remarks.c_str(); unsigned long rlen = remarks.size(); bind4[3].length = &rlen;
    bind4[4].buffer_type = MYSQL_TYPE_DOUBLE; bind4[4].buffer = &zero;
    bind4[5].buffer_type = MYSQL_TYPE_DOUBLE; bind4[5].buffer = &initial_deposit;
    mysql_stmt_bind_param(stmt4, bind4);
    if (mysql_stmt_execute(stmt4) != 0) {
        mysql_query(conn, "ROLLBACK");
        mysql_stmt_close(stmt4);
        mysql_close(conn);
        return 0;
    }
    mysql_stmt_close(stmt4);

    mysql_query(conn, "COMMIT");
    mysql_close(conn);

    // Output confirmation with account number and login button
    cout << "Content-Type: text/html\r\n\r\n";
    cout << "<!DOCTYPE html><html><head><title>Account Created</title></head><body>";
    cout << "<h2>Account Created Successfully</h2>";
    cout << "<p>Your Account Number is: <strong>" << accno << "</strong></p>";
    cout << "<p>Please note this account number to login.</p>";
    cout << "<a href='" << LOGIN_PAGE << "'><button>Login</button></a>";
    cout << "</body></html>";

    return 0;
}

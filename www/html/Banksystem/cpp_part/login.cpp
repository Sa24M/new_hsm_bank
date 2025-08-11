#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <openssl/sha.h>
#include <mysql/mysql.h>
using namespace std;

string url_decode(const string &s) {
    string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '+') out.push_back(' ');
        else if (c == '%' && i + 2 < s.size()) {
            string hex = s.substr(i+1, 2);
            char decoded = static_cast<char>(strtol(hex.c_str(), nullptr, 16));
            out.push_back(decoded);
            i += 2;
        } else out.push_back(c);
    }
    return out;
}

bool read_post(string &body) {
    const char *cl = getenv("CONTENT_LENGTH");
    if (!cl) return false;
    int len = atoi(cl);
    if (len <= 0) return false;
    body.resize(len);
    cin.read(&body[0], len);
    return true;
}

string sha256hex(const string &input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) ss << setw(2) << (int)hash[i];
    return ss.str();
}

string urlencode_msg(const string &s) {
    ostringstream oss;
    oss << hex << setfill('0');
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') oss << c;
        else if (c == ' ') oss << '+';
        else oss << '%' << setw(2) << (int)c;
    }
    return oss.str();
}

static inline string to_lower(string s) {
    for (char &c : s) c = (char)tolower((unsigned char)c);
    return s;
}

int main() {
    const string LOGIN_PAGE = "/Banksystem/htmlpart/login.html";
    const string DASHBOARD_PAGE = "/Banksystem/htmlpart/dashboard.html";

    string body;
    if (!read_post(body)) {
        cout << "Status: 303 See Other\r\nLocation: " << LOGIN_PAGE 
             << "?msg=" << urlencode_msg("Missing credentials") << "\r\n\r\n";
        return 0;
    }

    string accno_s, password;
    size_t pos = 0;
    while (pos < body.size()) {
        size_t amp = body.find('&', pos);
        string token = (amp == string::npos) ? body.substr(pos) : body.substr(pos, amp - pos);
        size_t eq = token.find('=');
        if (eq != string::npos) {
            string key = to_lower(url_decode(token.substr(0, eq)));
            string val = url_decode(token.substr(eq + 1));
            if (key == "accountnumber" || key == "accno" || key == "account") accno_s = val;
            else if (key == "password") password = val;
        }
        if (amp == string::npos) break;
        pos = amp + 1;
    }

    if (accno_s.empty() || password.empty()) {
        cout << "Status: 303 See Other\r\nLocation: " << LOGIN_PAGE 
             << "?msg=" << urlencode_msg("Missing credentials") << "\r\n\r\n";
        return 0;
    }

    int accno = 0;
    try { accno = stoi(accno_s); } catch (...) {
        cout << "Status: 303 See Other\r\nLocation: " << LOGIN_PAGE 
             << "?msg=" << urlencode_msg("Invalid account number") << "\r\n\r\n";
        return 0;
    }

    MYSQL *conn = mysql_init(nullptr);
    if (!mysql_real_connect(conn, "127.0.0.1", "bankuser", "bankpass", "bms", 3306, nullptr, 0)) {
        mysql_close(conn);
        cout << "Status: 303 See Other\r\nLocation: " << LOGIN_PAGE 
             << "?msg=" << urlencode_msg("DB connection failed") << "\r\n\r\n";
        return 0;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *qry = "SELECT Pass FROM credentials WHERE AccNo = ?";
    mysql_stmt_prepare(stmt, qry, strlen(qry));

    MYSQL_BIND bind_param[1];
    memset(bind_param, 0, sizeof(bind_param));
    bind_param[0].buffer_type = MYSQL_TYPE_LONG;
    bind_param[0].buffer = (char*)&accno;
    bind_param[0].buffer_length = sizeof(accno);

    mysql_stmt_bind_param(stmt, bind_param);
    mysql_stmt_execute(stmt);

    char passbuf[65] = {0};
    unsigned long pass_len = 0;
    bool is_null = 0;

    MYSQL_BIND bind_result[1];
    memset(bind_result, 0, sizeof(bind_result));
    bind_result[0].buffer_type = MYSQL_TYPE_STRING;
    bind_result[0].buffer = passbuf;
    bind_result[0].buffer_length = sizeof(passbuf);
    bind_result[0].length = &pass_len;
    bind_result[0].is_null = &is_null;

    mysql_stmt_bind_result(stmt, bind_result);
    mysql_stmt_store_result(stmt);

    if (mysql_stmt_num_rows(stmt) == 0) {
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        cout << "Status: 303 See Other\r\nLocation: " << LOGIN_PAGE 
             << "?msg=" << urlencode_msg("Account not found") << "\r\n\r\n";
        return 0;
    }

    mysql_stmt_fetch(stmt);
    string db_pass = is_null ? "" : string(passbuf, pass_len);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    mysql_close(conn);

    if (!db_pass.empty() && sha256hex(password) == db_pass) {
        cout << "Content-Type: text/html\r\n\r\n";
        cout << "<script>";
        cout << "localStorage.setItem('accno','" << accno << "');";
        cout << "document.cookie = 'AccNo=" << accno << "; path=/';";
        cout << "window.location='" << DASHBOARD_PAGE << "';";
        cout << "</script>";
    } else {
        cout << "Status: 303 See Other\r\nLocation: " << LOGIN_PAGE 
             << "?msg=" << urlencode_msg("Invalid credentials") << "\r\n\r\n";
    }
    return 0;
}

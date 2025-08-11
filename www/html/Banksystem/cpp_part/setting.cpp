#include <iostream>
#include <string>
#include <cstdlib>
#include <mysql/mysql.h>
#include <sstream>
#include <map>
#include <iomanip>
#include <openssl/sha.h>
#include <cctype>
#include <algorithm>
using namespace std;

string url_decode(const string& src) {
    string ret;
    char ch;
    int i, ii;
    for (i = 0; i < src.length(); i++) {
        if ((int)src[i] == 37 && i + 2 < src.length()) {
            sscanf(src.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i += 2;
        } else if (src[i] == '+') {
            ret += ' ';
        } else {
            ret += src[i];
        }
    }
    return ret;
}
map<string, string> parsePostData(const string& data) {
    map<string, string> result;
    stringstream ss(data);
    string token;
    while (getline(ss, token, '&')) {
        size_t pos = token.find('=');
        if (pos != string::npos) {
            string key = url_decode(token.substr(0, pos));
            string value = url_decode(token.substr(pos + 1));
            result[key] = value;
        }
    }
    return result;
}
string sha256(const string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(str.c_str()), str.length(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    return ss.str();
}
bool is_numeric(const string& s) {
    return !s.empty() && all_of(s.begin(), s.end(), ::isdigit);
}
int main() {
    cout << "Content-type: text/html\n\n";
    const string redirect_url = "/Banksystem/htmlpart/setting.html";
    const char* len_str = getenv("CONTENT_LENGTH");
    if (!len_str) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Missing+form+data';</script>";
        return 0;
    }
    int len = atoi(len_str);
    if (len <= 0 || len > 4096) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Invalid+form+data+length';</script>";
        return 0;
    }
    string postData(len, 0);
    cin.read(&postData[0], len);
    auto form = parsePostData(postData);
    string accno = form["accno"];
    string new_password = form["new_password"];
    string confirm_password = form["confirm_password"];
    if (accno.empty() || new_password.empty() || confirm_password.empty()) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Please+fill+all+fields';</script>";
        return 0;
    }
    if (!is_numeric(accno)) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Invalid+account+number';</script>";
        return 0;
    }
    if (new_password != confirm_password) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Passwords+do+not+match';</script>";
        return 0;
    }
    MYSQL* conn = mysql_init(NULL);
    if (!conn) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=MySQL+initialization+failed';</script>";
        return 0;
    }
    if (!mysql_real_connect(conn, "localhost", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Database+connection+failed';</script>";
        mysql_close(conn);
        return 0;
    }
    string hashed = sha256(new_password);
    char esc_newpass[256];
    unsigned long esc_len = mysql_real_escape_string(conn, esc_newpass, hashed.c_str(), hashed.length());
    esc_newpass[esc_len] = '\0';
    stringstream query;
    query << "UPDATE credentials SET Pass='" << esc_newpass << "' WHERE AccNo=" << accno;
    if (mysql_query(conn, query.str().c_str()) != 0) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Password+update+failed';</script>";
        mysql_close(conn);
        return 0;
    }
    if (mysql_affected_rows(conn) == 0) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Account+number+not+found';</script>";
        mysql_close(conn);
        return 0;
    }
    mysql_close(conn);
    cout << "<script>window.location.href='" << redirect_url << "?msg=Password+updated+successfully';</script>";
    return 0;
}

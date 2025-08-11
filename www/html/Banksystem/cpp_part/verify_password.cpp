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
    string old_password = form["old_password"];
    if (accno.empty() || old_password.empty()) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Please+fill+all+fields';</script>";
        return 0;
    }
    if (!is_numeric(accno)) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Invalid+account+number';</script>";
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
    stringstream query;
    query << "SELECT Pass FROM credentials WHERE AccNo=" << accno << " LIMIT 1";
    if (mysql_query(conn, query.str().c_str()) != 0) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Database+query+failed';</script>";
        mysql_close(conn);
        return 0;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Database+result+error';</script>";
        mysql_close(conn);
        return 0;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Account+number+not+found';</script>";
        mysql_free_result(res);
        mysql_close(conn);
        return 0;
    }
    string stored_hash = row[0] ? row[0] : "";
    string hashed_input = sha256(old_password);
    mysql_free_result(res);
    mysql_close(conn);
    if (hashed_input == stored_hash) {
        cout << "<script>window.location.href='" << redirect_url << "?stage=verify&accno=" << accno << "';</script>";
    } else {
        cout << "<script>window.location.href='" << redirect_url << "?msg=Old+password+is+incorrect';</script>";
    }
    return 0;
}

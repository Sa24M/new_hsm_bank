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
#include <cstring> // Required for memset

using namespace std;

string url_decode(const string& src) {
    string ret;
    char ch;
    size_t i, ii;
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

string urlencode_msg(const string& s) {
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

void redirect_with_message(const string& page, const string& msg) {
    cout << "Status: 303 See Other\r\n";
    cout << "Location: " << page << "?msg=" << urlencode_msg(msg) << "\r\n\r\n";
}

int main() {
    // Correcting the path to be relative to the server's root
    const string redirect_url = "/htmlpart/setting.html";
    
    const char* len_str = getenv("CONTENT_LENGTH");
    if (!len_str) {
        redirect_with_message(redirect_url, "Missing form data");
        return 0;
    }

    int len = atoi(len_str);
    if (len <= 0 || len > 4096) {
        redirect_with_message(redirect_url, "Invalid form data length");
        return 0;
    }

    string postData(len, 0);
    cin.read(&postData[0], len);
    
    auto form = parsePostData(postData);
    
    if (form.count("accno") == 0 || form.count("new_password") == 0 || form.count("confirm_password") == 0) {
        redirect_with_message(redirect_url, "Please fill all fields");
        return 0;
    }

    string accno_str = form["accno"];
    string new_password = form["new_password"];
    string confirm_password = form["confirm_password"];
    
    if (!is_numeric(accno_str)) {
        redirect_with_message(redirect_url, "Invalid account number");
        return 0;
    }
    
    if (new_password != confirm_password) {
        redirect_with_message(redirect_url, "Passwords do not match");
        return 0;
    }

    int accno = stoi(accno_str);
    string hashed_password = sha256(new_password);
    
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        redirect_with_message(redirect_url, "MySQL initialization failed");
        return 0;
    }

    if (!mysql_real_connect(conn, "localhost", "bankuser", "bankpass", "bms", 3306, NULL, 0)) {
        redirect_with_message(redirect_url, "Database connection failed");
        mysql_close(conn);
        return 0;
    }
    
    // Use prepared statements to prevent SQL Injection
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        redirect_with_message(redirect_url, "Prepared statement failed");
        mysql_close(conn);
        return 0;
    }
    
    const char* query = "UPDATE credentials SET Pass = ? WHERE AccNo = ?";
    if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
        redirect_with_message(redirect_url, "Query preparation failed");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)hashed_password.c_str();
    bind[0].buffer_length = hashed_password.length();

    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = (char*)&accno;
    
    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        redirect_with_message(redirect_url, "Parameter binding failed");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }
    
    if (mysql_stmt_execute(stmt) != 0) {
        redirect_with_message(redirect_url, "Password update failed");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }
    
    if (mysql_stmt_affected_rows(stmt) == 0) {
        redirect_with_message(redirect_url, "Account number not found or no changes made");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }
    
    mysql_stmt_close(stmt);
    mysql_close(conn);
    
    redirect_with_message(redirect_url, "Password updated successfully");
    return 0;
}
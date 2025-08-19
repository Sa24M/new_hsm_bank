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
#include <cstring>

using namespace std;

// Helper function for secure server-side redirects
void redirect_with_message(const string& page, const string& msg) {
    cout << "Status: 303 See Other\r\n";
    cout << "Location: " << page << "?msg=";
    
    // Simple URL encode for the message
    for (char c : msg) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.') {
            cout << c;
        } else if (c == ' ') {
            cout << '+';
        } else {
            cout << '%' << uppercase << hex << setw(2) << setfill('0') << (int)(unsigned char)c;
        }
    }
    cout << "\r\n\r\n";
}

// Function to handle redirects for a different stage
void redirect_to_stage(const string& page, const string& stage, const string& accno) {
    cout << "Status: 303 See Other\r\n";
    cout << "Location: " << page << "?stage=" << stage << "&accno=" << accno << "\r\n\r\n";
}

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

int main() {
    cout << "Content-type: text/html\n\n";

    const string redirect_url = "/htmlpart/setting.html";
    
    const char* len_str = getenv("CONTENT_LENGTH");
    if (!len_str) {
        redirect_with_message(redirect_url, "Missing form data");
        return 0;
    }

    int len = atoi(len_str);
    if (len <= 0) {
        redirect_with_message(redirect_url, "Invalid form data length");
        return 0;
    }
    
    string postData(len, 0);
    cin.read(&postData[0], len);
    auto form = parsePostData(postData);

    string accno_str = form["accno"];
    string old_password = form["old_password"];

    if (accno_str.empty() || old_password.empty()) {
        redirect_with_message(redirect_url, "Please fill all fields");
        return 0;
    }

    if (!is_numeric(accno_str)) {
        redirect_with_message(redirect_url, "Invalid account number format");
        return 0;
    }

    int accno = stoi(accno_str);

    MYSQL* conn = mysql_init(NULL);
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
        redirect_with_message(redirect_url, "Statement initialization failed");
        mysql_close(conn);
        return 0;
    }

    const char* query = "SELECT Pass FROM credentials WHERE AccNo = ?";
    if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
        redirect_with_message(redirect_url, "Query preparation failed");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &accno;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        redirect_with_message(redirect_url, "Parameter binding failed");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        redirect_with_message(redirect_url, "Query execution failed");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }

    MYSQL_RES* res = mysql_stmt_result_metadata(stmt);
    if (!res) {
        redirect_with_message(redirect_url, "Failed to get result metadata");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }
    
    string stored_hash;
    char pass_buf[65] = {0};
    unsigned long pass_len = 0;
    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));
    result_bind[0].buffer_type = MYSQL_TYPE_STRING;
    result_bind[0].buffer = pass_buf;
    result_bind[0].buffer_length = sizeof(pass_buf);
    result_bind[0].length = &pass_len;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
        redirect_with_message(redirect_url, "Result binding failed");
        mysql_free_result(res);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }

    int fetch_status = mysql_stmt_fetch(stmt);
    if (fetch_status == 1) { // Error
        redirect_with_message(redirect_url, "Failed to fetch result");
        mysql_free_result(res);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }

    if (fetch_status == MYSQL_NO_DATA) {
        redirect_with_message(redirect_url, "Account not found");
        mysql_free_result(res);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 0;
    }

    stored_hash.assign(pass_buf, pass_len);
    string hashed_input = sha256(old_password);

    mysql_free_result(res);
    mysql_stmt_close(stmt);
    mysql_close(conn);

    if (hashed_input == stored_hash) {
        // Success: Redirect to the next stage to change the password
        redirect_to_stage(redirect_url, "change", accno_str);
    } else {
        // Failure: Redirect back to the verify stage with an error message
        redirect_to_stage(redirect_url, "verify", accno_str);
        // It's also good practice to include a message for better user feedback
        // redirect_with_message(redirect_url, "Old password is incorrect");
    }

    return 0;
}
#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <map>
#include <mysql/mysql.h>
#include <openssl/sha.h>
#include <iomanip>
#include <cstring>
#include <cctype>
#include <algorithm>

using namespace std;

// Helper to handle server-side redirects
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

// Function to hash password using SHA-256
string sha256(const string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash);
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << setw(2) << (int)hash[i];
    }
    return ss.str();
}

// Helper to parse POST data
map<string, string> parse_post_data() {
    map<string, string> data;
    string input;
    char* len_str = getenv("CONTENT_LENGTH");
    if (!len_str) return data;

    int len = atoi(len_str);
    input.resize(len);
    cin.read(&input[0], len);

    stringstream ss(input);
    string pair;
    while (getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != string::npos) {
            string key = pair.substr(0, pos);
            string value = pair.substr(pos + 1);
            for (size_t i = 0; i < value.size(); i++) {
                if (value[i] == '+') value[i] = ' ';
                else if (value[i] == '%') {
                    string hex = value.substr(i + 1, 2);
                    value[i] = static_cast<char>(strtol(hex.c_str(), nullptr, 16));
                    value.erase(i + 1, 2);
                }
            }
            data[key] = value;
        }
    }
    return data;
}

int main() {
    cout << "Content-type: text/html\r\n\r\n";
    const string transaction_page = "/htmlpart/transaction.html";

    map<string, string> form = parse_post_data();
    string sender_accNo_str = form["sender_accNo"];
    string sender_password = form["sender_password"];
    string receiver_accNo_str = form["receiver_accNo"];
    string amount_str = form["amount"];
    string remarks = form["remarks"];

    if (sender_accNo_str.empty() || sender_password.empty() || receiver_accNo_str.empty() || amount_str.empty()) {
        redirect_with_message(transaction_page, "Invalid Input");
        return 0;
    }

    int sender_accNo, receiver_accNo;
    float amount;
    try {
        sender_accNo = stoi(sender_accNo_str);
        receiver_accNo = stoi(receiver_accNo_str);
        amount = stof(amount_str);
    } catch (...) {
        redirect_with_message(transaction_page, "Invalid Input Format");
        return 0;
    }
    
    if (amount <= 0) {
        redirect_with_message(transaction_page, "Amount must be positive");
        return 0;
    }
    if (sender_accNo == receiver_accNo) {
        redirect_with_message(transaction_page, "Cannot transact with yourself");
        return 0;
    }

    MYSQL* conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "localhost", "bankuser", "bankpass", "bms", 0, NULL, 0)) {
        redirect_with_message(transaction_page, "Database Connection Failed");
        return 0;
    }

    mysql_query(conn, "START TRANSACTION");

    // 1. Authenticate sender with prepared statement
    MYSQL_STMT* stmt_auth = mysql_stmt_init(conn);
    const char* auth_sql = "SELECT Pass FROM credentials WHERE AccNo = ?";
    mysql_stmt_prepare(stmt_auth, auth_sql, strlen(auth_sql));
    
    MYSQL_BIND auth_bind[1];
    memset(auth_bind, 0, sizeof(auth_bind));
    auth_bind[0].buffer_type = MYSQL_TYPE_LONG;
    auth_bind[0].buffer = &sender_accNo;
    
    mysql_stmt_bind_param(stmt_auth, auth_bind);
    mysql_stmt_execute(stmt_auth);
    
    MYSQL_RES* auth_res = mysql_stmt_result_metadata(stmt_auth);
    if (auth_res == nullptr) {
        mysql_stmt_close(stmt_auth);
        mysql_close(conn);
        redirect_with_message(transaction_page, "DB Error");
        return 0;
    }

    string stored_password_hash;
    char pass_buf[65];
    unsigned long pass_len;
    MYSQL_BIND auth_result_bind[1];
    memset(auth_result_bind, 0, sizeof(auth_result_bind));
    auth_result_bind[0].buffer_type = MYSQL_TYPE_STRING;
    auth_result_bind[0].buffer = pass_buf;
    auth_result_bind[0].buffer_length = sizeof(pass_buf);
    auth_result_bind[0].length = &pass_len;

    mysql_stmt_bind_result(stmt_auth, auth_result_bind);
    if (mysql_stmt_fetch(stmt_auth) == 0) {
        stored_password_hash.assign(pass_buf, pass_len);
    }
    mysql_free_result(auth_res);
    mysql_stmt_close(stmt_auth);
    
    if (stored_password_hash.empty() || sha256(sender_password) != stored_password_hash) {
        mysql_close(conn);
        redirect_with_message(transaction_page, "Invalid Credentials");
        return 0;
    }
    
    // 2. Get balances with prepared statements (for locking)
    MYSQL_STMT* stmt_balance = mysql_stmt_init(conn);
    const char* balance_sql = "SELECT AccNo, Balance FROM balance WHERE AccNo IN (?, ?) ORDER BY AccNo FOR UPDATE";
    mysql_stmt_prepare(stmt_balance, balance_sql, strlen(balance_sql));

    MYSQL_BIND balance_bind[2];
    memset(balance_bind, 0, sizeof(balance_bind));
    balance_bind[0].buffer_type = MYSQL_TYPE_LONG;
    balance_bind[0].buffer = &sender_accNo;
    balance_bind[1].buffer_type = MYSQL_TYPE_LONG;
    balance_bind[1].buffer = &receiver_accNo;
    
    mysql_stmt_bind_param(stmt_balance, balance_bind);
    mysql_stmt_execute(stmt_balance);
    
    int fetched_accNo;
    float fetched_balance;
    int found_count = 0;
    float current_sender_balance = -1.0f, current_receiver_balance = -1.0f;

    MYSQL_BIND balance_result_bind[2];
    memset(balance_result_bind, 0, sizeof(balance_result_bind));
    balance_result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    balance_result_bind[0].buffer = &fetched_accNo;
    balance_result_bind[1].buffer_type = MYSQL_TYPE_FLOAT;
    balance_result_bind[1].buffer = &fetched_balance;

    mysql_stmt_bind_result(stmt_balance, balance_result_bind);
    
    while (mysql_stmt_fetch(stmt_balance) == 0) {
        found_count++;
        if (fetched_accNo == sender_accNo) {
            current_sender_balance = fetched_balance;
        } else if (fetched_accNo == receiver_accNo) {
            current_receiver_balance = fetched_balance;
        }
    }
    mysql_stmt_close(stmt_balance);

    if (found_count < 2 || current_sender_balance < 0 || current_receiver_balance < 0) {
        mysql_query(conn, "ROLLBACK");
        mysql_close(conn);
        redirect_with_message(transaction_page, "Sender or Receiver not found");
        return 0;
    }
    
    if (current_sender_balance < amount) {
        mysql_query(conn, "ROLLBACK");
        mysql_close(conn);
        redirect_with_message(transaction_page, "Insufficient Balance");
        return 0;
    }
    
    float new_sender_balance = current_sender_balance - amount;
    float new_receiver_balance = current_receiver_balance + amount;
    
    // 3. Update balances with prepared statements
    MYSQL_STMT* stmt_update = mysql_stmt_init(conn);
    const char* update_sql = "UPDATE balance SET Balance = CASE AccNo WHEN ? THEN ? ELSE ? END WHERE AccNo IN (?, ?)";
    mysql_stmt_prepare(stmt_update, update_sql, strlen(update_sql));
    
    MYSQL_BIND update_bind[5];
    memset(update_bind, 0, sizeof(update_bind));
    update_bind[0].buffer_type = MYSQL_TYPE_LONG;
    update_bind[0].buffer = &sender_accNo;
    update_bind[1].buffer_type = MYSQL_TYPE_FLOAT;
    update_bind[1].buffer = &new_sender_balance;
    update_bind[2].buffer_type = MYSQL_TYPE_FLOAT;
    update_bind[2].buffer = &new_receiver_balance;
    update_bind[3].buffer_type = MYSQL_TYPE_LONG;
    update_bind[3].buffer = &sender_accNo;
    update_bind[4].buffer_type = MYSQL_TYPE_LONG;
    update_bind[4].buffer = &receiver_accNo;

    mysql_stmt_bind_param(stmt_update, update_bind);
    if (mysql_stmt_execute(stmt_update) != 0) {
        mysql_query(conn, "ROLLBACK");
        mysql_stmt_close(stmt_update);
        mysql_close(conn);
        redirect_with_message(transaction_page, "Update Failed");
        return 0;
    }
    mysql_stmt_close(stmt_update);
    
    // 4. Insert transaction log with prepared statement
    MYSQL_STMT* stmt_log = mysql_stmt_init(conn);
    const char* log_sql = "INSERT INTO transactions (Sender, Receiver, Amount, Remarks, SenBalance, RecBalance) VALUES (?, ?, ?, ?, ?, ?)";
    mysql_stmt_prepare(stmt_log, log_sql, strlen(log_sql));
    
    MYSQL_BIND log_bind[6];
    memset(log_bind, 0, sizeof(log_bind));
    log_bind[0].buffer_type = MYSQL_TYPE_LONG;
    log_bind[0].buffer = &sender_accNo;
    log_bind[1].buffer_type = MYSQL_TYPE_LONG;
    log_bind[1].buffer = &receiver_accNo;
    log_bind[2].buffer_type = MYSQL_TYPE_FLOAT;
    log_bind[2].buffer = &amount;
    log_bind[3].buffer_type = MYSQL_TYPE_STRING;
    log_bind[3].buffer = (char*)remarks.c_str();
    log_bind[3].buffer_length = remarks.length();
    log_bind[4].buffer_type = MYSQL_TYPE_FLOAT;
    log_bind[4].buffer = &new_sender_balance;
    log_bind[5].buffer_type = MYSQL_TYPE_FLOAT;
    log_bind[5].buffer = &new_receiver_balance;

    mysql_stmt_bind_param(stmt_log, log_bind);
    if (mysql_stmt_execute(stmt_log) != 0) {
        mysql_query(conn, "ROLLBACK");
        mysql_stmt_close(stmt_log);
        mysql_close(conn);
        redirect_with_message(transaction_page, "Transaction Logging Failed");
        return 0;
    }
    mysql_stmt_close(stmt_log);

    // Commit transaction and close connection
    mysql_query(conn, "COMMIT");
    mysql_close(conn);
    redirect_with_message(transaction_page, "Transaction Successful");
    return 0;
}
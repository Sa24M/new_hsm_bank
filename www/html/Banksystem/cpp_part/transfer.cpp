#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include <mysql/mysql.h>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;

// Function to hash password using SHA-256
string sha256(const string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
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

    map<string, string> form = parse_post_data();
    string sender_accNo = form["sender_accNo"];
    string sender_password = form["sender_password"];
    string receiver_accNo = form["receiver_accNo"];
    string amount_str = form["amount"];
    string remarks = form["remarks"];
    float amount = stof(amount_str);

    if (sender_accNo.empty() || sender_password.empty() || receiver_accNo.empty() || amount <= 0) {
        cout << "<script>window.location.href='/Banksystem/htmlpart/transaction.html?msg=Invalid+Input';</script>";
        return 0;
    }

    MYSQL* conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "localhost", "bankuser", "bankpass", "bms", 0, NULL, 0)) {
        cout << "<script>window.location.href='/Banksystem/htmlpart/transaction.html?msg=Database+Connection+Failed';</script>";
        return 0;
    }

    MYSQL_RES* res;
    MYSQL_ROW row;

    // Authenticate sender
    string query = "SELECT Pass FROM credentials WHERE AccNo = '" + sender_accNo + "'";
    mysql_query(conn, query.c_str());
    res = mysql_store_result(conn);
    if (mysql_num_rows(res) == 0) {
        mysql_free_result(res);
        cout << "<script>window.location.href='/Banksystem/htmlpart/transaction.html?msg=Sender+Not+Found';</script>";
        mysql_close(conn);
        return 0;
    }

    row = mysql_fetch_row(res);
    string stored_password = row[0];
    mysql_free_result(res);

    string hashed_input = sha256(sender_password);
    if (stored_password != hashed_input && stored_password != sender_password) {
        cout << "<script>window.location.href='/Banksystem/htmlpart/transaction.html?msg=Wrong+Password';</script>";
        mysql_close(conn);
        return 0;
    }

    // Get sender balance
    query = "SELECT Balance FROM balance WHERE AccNo = '" + sender_accNo + "'";
    mysql_query(conn, query.c_str());
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    float sender_balance = atof(row[0]);
    mysql_free_result(res);

    if (sender_balance < amount) {
        cout << "<script>window.location.href='/Banksystem/htmlpart/transaction.html?msg=Insufficient+Balance';</script>";
        mysql_close(conn);
        return 0;
    }

    // Check receiver exists
    query = "SELECT Balance FROM balance WHERE AccNo = '" + receiver_accNo + "'";
    mysql_query(conn, query.c_str());
    res = mysql_store_result(conn);
    if (mysql_num_rows(res) == 0 || sender_accNo == receiver_accNo) {
        mysql_free_result(res);
        cout << "<script>window.location.href='/Banksystem/htmlpart/transaction.html?msg=Invalid+Receiver';</script>";
        mysql_close(conn);
        return 0;
    }
    row = mysql_fetch_row(res);
    float receiver_balance = atof(row[0]);
    mysql_free_result(res);

    // Update balances
    sender_balance -= amount;
    receiver_balance += amount;

    stringstream ss1, ss2, ss3;
    ss1 << "UPDATE balance SET Balance = " << sender_balance << " WHERE AccNo = '" << sender_accNo << "'";
    mysql_query(conn, ss1.str().c_str());

    ss2 << "UPDATE balance SET Balance = " << receiver_balance << " WHERE AccNo = '" << receiver_accNo << "'";
    mysql_query(conn, ss2.str().c_str());

    ss3 << "INSERT INTO transactions (Sender, Receiver, Amount, Remarks, SenBalance, RecBalance) VALUES ('"
        << sender_accNo << "', '" << receiver_accNo << "', " << amount << ", '" << remarks << "', "
        << sender_balance << ", " << receiver_balance << ")";
    mysql_query(conn, ss3.str().c_str());

    mysql_close(conn);
    cout << "<script>window.location.href='/Banksystem/htmlpart/transaction.html?msg=Transaction+Successful';</script>";
    return 0;
}

# 🌐 HSM Bank: Banking Management System

A **full-stack Banking Management System** developed as part of an internship project (June 2025 – July 2025).  
The system supports **account creation, deposits, withdrawals, transfers, and transaction logs** with a **real-time dashboard**.  
It is powered by a **C++ backend** (via CGI scripts), **MySQL database**, and a **static HTML/CSS/JS frontend**, hosted locally on an **Apache web server**.

<br>

---

## ✨ Features

- **Account Management**: Account creation, balance tracking, deposits, withdrawals, and transfers.  
- **Transaction Logs**: Detailed record of all user transactions.  
- **Real-Time Dashboard**: Displays balances, income, and expenses dynamically.  
- **User Authentication**: Credentials secured with **SHA-256 hashing**.  
- **Loan Services**: Mock loan applications (home, education, vehicle, gold, personal).  
- **Password Management**: Secure password updates.  

**Impact:** Streamlined account management, provided real-time financial insights, and ensured secure data handling.

<br>

---

## 💻 Technology Stack

- **Frontend**: HTML, CSS, JavaScript  
- **Backend**: C++ (compiled as CGI executables)  
- **Database**: MySQL (for persistent storage & transaction logs)  
- **Web Server**: Apache HTTP Server  
- **Toolchain**: MSYS2 with MinGW-w64 (`libmysqlclient.dll` for DB connectivity)  

<br>

---

## 🔧 Setup & Installation

### 1. Prerequisites
- Apache HTTP Server (assumed at `C:\Apache24`)  
- MySQL Community Server (running locally)  
- MSYS2 with MinGW-w64 toolchain  
- MySQL development libraries  

Install required packages inside MSYS2 shell:
```bash
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-libmysqlclient
```

---

### 2. Directory Structure

The project integrates with Apache as follows:

```
C:\Apache24\
├───bin\httpd.exe
├───conf\httpd.conf
├───htdocs\
│   ├── index.html
│   ├── csspart\
│   ├── htmlpart\
│   └── img\
└───cgi-bin\
    └── <compiled .exe files + required DLLs>
```

---

### 3. Database Setup

👉 A ready-to-use **`sql/bms.sql`** is provided in the repository (schema + sample data).  

Steps:  
1. Log in to MySQL:
   ```bash
   mysql -u root -p
   ```
2. Create the database and user:
   ```sql
   CREATE DATABASE bms;
   CREATE USER 'bankuser'@'localhost' IDENTIFIED BY 'bankpass';
   GRANT ALL PRIVILEGES ON bms.* TO 'bankuser'@'localhost';
   FLUSH PRIVILEGES;
   ```
3. Import the schema:
   ```bash
   mysql -u bankuser -p bms < sql/bms.sql
   ```

---

### 4. Compiling the Backend

👉 The repo includes a **`Makefile`** to automate compilation.  

From the MSYS2 MinGW64 shell:
```bash
make all
```

- All `.cpp` files in `src/` will be compiled.  
- Output `.exe` files (and required DLLs) will be copied to `C:\Apache24\cgi-bin`.  

---

### 5. Apache Configuration

- Updated `conf/httpd.conf`is provided in codebase.



Restart Apache afterwards.

---

## 🚀 Running the Project

1. Start Apache (`httpd.exe`).  
2. Open [http://localhost/htmlpart/home.html](http://localhost/htmlpart/home.html).  

---

## 📈 Future Scope & Improvements

In future iterations, the Banking Management System can be enhanced with more real-world banking features, such as:

- **Session Management**: Stronger security with login sessions.  
- **Account Types & Interest**: Savings, current, and fixed deposits with automatic interest calculation.  
- **Fund Transfers**: NEFT/IMPS/RTGS-style money transfers with unique transaction IDs.  
- **Loan Management**: Loan application, EMI calculation, and repayment tracking.  
- **Advanced Transaction History**: Search/filter transactions; export as PDF/CSV.  
- **Security Enhancements**: Two-factor authentication (2FA), role-based access.  
- **Customer Services**: Simulated cheque book requests, ATM withdrawals, bill payments.  
- **Notifications**: Email/SMS alerts for transactions and low balance.  
- **Technical Improvements**: Database normalization, concurrency handling, OOP refactoring.  
- **Audit & Fraud Detection**: Audit logs and anomaly detection.  
- **UI/UX Modernization**: Better styling and responsive design.  

---

## 🌐 Hosting Online

To host beyond local environment:  
1. Purchase domain + hosting that supports Apache & MySQL.  
2. Upload `htdocs` and `cgi-bin` files to server.  
3. Import the provided `bms.sql` schema into server MySQL.  
4. Configure Apache to enable CGI execution.  


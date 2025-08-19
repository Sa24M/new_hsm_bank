# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -O2

# Apache cgi-bin path
CGIBIN = /c/Apache24/cgi-bin

# Source files
SRCS = dashboard.cpp \
       get_balance.cpp \
       get_transaction.cpp \
       login.cpp \
       register.cpp \
       setting.cpp \
       statement.cpp \
       transfer.cpp \
       verify_password.cpp

# Executable names (replace .cpp with .cgi)
EXES = $(SRCS:.cpp=.cgi)

# Default target
all: $(EXES) deploy

# Rule to build each cgi from its cpp (MySQL version)
%.cgi: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ \
	-I"C:/Program Files/MySQL/MySQL Server 8.0/include" \
	-L"C:/MyLibs" -lmysql -lcrypto

# Deploy all executables into Apache cgi-bin
deploy: $(EXES)
	cp -f $(EXES) $(CGIBIN)

# Clean rule
clean:
	rm -f *.cgi
	# You might also want to remove any old .exe files if they exist from previous builds
	rm -f *.exe
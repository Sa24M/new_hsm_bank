// ------------------- Helper Functions ------------------- //

// Read a cookie value by name
function readCookie(name) {
  const cookies = document.cookie.split(";").map(c => c.trim());
  for (let c of cookies) {
    if (c.startsWith(name + "=")) {
      return decodeURIComponent(c.substring(name.length + 1));
    }
  }
  return null;
}

// Format numbers with Indian locale commas
function fmt(n) {
  if (n === null || n === undefined || isNaN(n)) return "0";
  return Number(n).toLocaleString("en-IN");
}

// Get the logged-in account number
function getAccNo() {
  // Priority: cookie -> localStorage -> hidden input field
  let acc = readCookie("AccNo");
  if (acc) return acc;

  acc = localStorage.getItem("accno");
  if (acc) return acc;

  const hidden = document.getElementById("accno");
  if (hidden && hidden.value) return hidden.value;

  return "";
}

// ------------------- UI Update Functions ------------------- //

// Fill the top cards with fetched data
function renderCards(data) {
  document.getElementById("balanceCard").innerText =
    "₹ " + fmt(data.balance || 0);
  document.getElementById("interestCard").innerText =
    "₹ " + fmt(data.interest || 0);
  document.getElementById("debitCard").innerText =
    "₹ " + fmt(data.debited || data.debit || 0);
  document.getElementById("creditCard").innerText =
    "₹ " + fmt(data.credited || data.credit || 0);

  if (data.name) {
    document.getElementById("welcomeUser").innerText = `Welcome, ${data.name}`;
  }
}

// Render or update the Chart.js bar chart
let chartInstance = null;
function renderChart(data) {
  const ctx = document.getElementById("balanceChart").getContext("2d");
  const values = [
    Number(data.balance || 0),
    Number(data.interest || 0),
    Number(data.debited || data.debit || 0),
    Number(data.credited || data.credit || 0),
  ];

  if (chartInstance) {
    chartInstance.data.datasets[0].data = values;
    chartInstance.update();
    return;
  }

  chartInstance = new Chart(ctx, {
    type: "bar",
    data: {
      labels: ["Balance", "Interest", "Debited", "Credited"],
      datasets: [
        {
          label: "Amount (₹)",
          data: values,
          backgroundColor: ["#1a237e", "#6ec1e4", "#e53935", "#43a047"],
        },
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: { beginAtZero: true },
      },
    },
  });
}

// Load and render recent transactions
function loadTransactions(accNo) {
  fetch(`/cgi-bin/get_transaction.cgi?accno=${encodeURIComponent(accNo)}`)
    .then((res) => {
      if (!res.ok) throw new Error("No transactions endpoint");
      return res.json();
    })
    .then((data) => {
      const container = document.getElementById("transactionList");
      container.innerHTML = "";

      if (!Array.isArray(data) || data.length === 0) {
        container.innerHTML = "<div>No transactions found</div>";
        return;
      }

      data.forEach((txn) => {
        const row = document.createElement("div");
        row.classList.add("transaction-row");

        const left = document.createElement("div");
        const right = document.createElement("div");

        // Left: Date + Remarks
        const dateText = txn.DateTime
          ? txn.DateTime.split(" ")[0]
          : txn.date || "";
        const remarkText =
          txn.Remarks || txn.remarks || txn.type || "No remark";
        left.innerHTML = `<strong>${dateText}</strong><br><small>${remarkText}</small>`;

        // Right: +/- amount with color
        const amt = Number(txn.Amount || txn.amount || 0);
        const isCredit =
          String(txn.Receiver || txn.receiver) === String(accNo);
        right.innerHTML = `${isCredit ? "+" : "-"} ₹ ${fmt(amt)}`;
        right.style.color = isCredit ? "#2e7d32" : "#c62828";

        row.appendChild(left);
        row.appendChild(right);
        container.appendChild(row);
      });
    })
    .catch((err) => {
      console.error(err);
      document.getElementById("transactionList").innerHTML =
        "<div>No transactions found</div>";
    });
}

// ------------------- Main Loader ------------------- //
function loadDashboard() {
  const accNo = getAccNo();

  if (!accNo) {
    // No login detected
    document.getElementById("welcomeUser").innerText = "Welcome, User";
    console.warn("No account number found — user not logged in?");
    return;
  }

  // Fetch the dashboard summary data
  fetch(`/cgi-bin/dashboard.cgi?accno=${encodeURIComponent(accNo)}`)
    .then((res) => {
      if (!res.ok) throw new Error("dashboard.cgi error");
      return res.json();
    })
    .then((data) => {
      renderCards(data);
      renderChart(data);
    })
    .catch((err) => {
      console.error("Dashboard fetch failed", err);
    });

  // Fetch the recent transactions list
  loadTransactions(accNo);
}

// ------------------- Initialize ------------------- //
document.addEventListener("DOMContentLoaded", loadDashboard);

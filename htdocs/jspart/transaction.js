function updateBalance(accNo) {
  fetch(`/cgi-bin/get_balance.cgi?accno=${accNo}`)
    .then(res => res.text())
    .then(balance => {
      document.getElementById("balanceAmount").innerText = "₹ " + balance;
    });
}

function updateTransactions(accNo) {
  fetch(`/cgi-bin/get_transaction.cgi?accno=${accNo}`)
    .then(res => res.json())
    .then(data => {
      const container = document.getElementById("transactionList");
      container.innerHTML = "";
      data.forEach(txn => {
        const el = document.createElement("div");
        el.innerHTML = `${txn.date} - ${txn.type} ₹${txn.amount}`;
        container.appendChild(el);
      });
    });
}

// Call these when dashboard loads
document.addEventListener("DOMContentLoaded", () => {
  const accNo = localStorage.getItem("accno"); // or prompt, or pass in hidden field
  if (accNo) {
    updateBalance(accNo);
    updateTransactions(accNo);
  }
});


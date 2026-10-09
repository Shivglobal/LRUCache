#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

// ==========================================
// 1. OBSERVER PATTERN (SRP for Logging/Auditing)
// ==========================================
class ITransactionObserver {
public:
    virtual ~ITransactionObserver() = default;
    virtual void onTransaction(const std::string& accountNum, const std::string& type, double amount, bool success, double newBalance) = 0;
};

// Console Logger implementation
class TransactionLogger : public ITransactionObserver {
public:
    void onTransaction(const std::string& accountNum, const std::string& type, double amount, bool success, double newBalance) override {
        std::cout << "[AUDIT LOG] Acc: " << std::left << std::setw(12) << accountNum 
                  << " | Action: " << std::setw(10) << type 
                  << " | Amount: $" << std::fixed << std::setprecision(2) << std::setw(8) << amount 
                  << " | Status: " << (success ? "SUCCESS" : "FAILED ") 
                  << " | Balance: $" << newBalance << "\n";
    }
};

// ==========================================
// 2. DOMAIN OBJECT: Balance (SRP)
// ==========================================
class Balance {
private:
    double amount;

public:
    explicit Balance(double initialAmount = 0.0) : amount(initialAmount) {
        if (initialAmount < 0.0) {
            throw std::invalid_argument("Initial balance cannot be negative.");
        }
    }

    double get() const { return amount; }

    void add(double val) {
        if (val <= 0) throw std::invalid_argument("Deposit amount must be positive.");
        amount += val;
    }

    bool deduct(double val) {
        if (val <= 0) throw std::invalid_argument("Withdrawal amount must be positive.");
        if (val > amount) return false;
        amount -= val;
        return true;
    }
};

// ==========================================
// 3. ABSTRACTION INTERFACE (DIP / OCP)
// ==========================================
class IBankAccount {
public:
    virtual ~IBankAccount() = default;

    virtual std::string getAccountNumber() const = 0;
    virtual std::string getAccountTypeName() const = 0;
    virtual double getBalance() const = 0;
    
    virtual void deposit(double amount) = 0;
    virtual bool withdraw(double amount) = 0;
    
    virtual void attachObserver(std::shared_ptr<ITransactionObserver> observer) = 0;
};

// Base class providing common auditing logic to derived accounts
class BaseBankAccount : public IBankAccount {
protected:
    std::string accNumber;
    Balance balance;
    std::vector<std::weak_ptr<ITransactionObserver>> observers;

    void notifyObservers(const std::string& type, double amount, bool success) {
        for (auto it = observers.begin(); it != observers.end(); ) {
            if (auto observer = it->lock()) {
                observer->onTransaction(accNumber, type, amount, success, balance.get());
                ++it;
            } else {
                it = observers.erase(it); // Cleanup expired weak pointers
            }
        }
    }

public:
    BaseBankAccount(std::string num, double initialBalance)
        : accNumber(std::move(num)), balance(initialBalance) {}

    std::string getAccountNumber() const override { return accNumber; }
    double getBalance() const override { return balance.get(); }

    void attachObserver(std::shared_ptr<ITransactionObserver> observer) override {
        observers.push_back(observer);
    }
};

// ==========================================
// 4. CONCRETE ACCOUNT TYPES (OCP / LSP)
// ==========================================

// Savings Account
class SavingsAccount : public BaseBankAccount {
private:
    double interestRate;

public:
    SavingsAccount(std::string num, double initialBalance, double rate = 0.03)
        : BaseBankAccount(std::move(num), initialBalance), interestRate(rate) {}

    std::string getAccountTypeName() const override { return "Savings"; }

    void deposit(double amount) override {
        balance.add(amount);
        notifyObservers("DEPOSIT", amount, true);
    }

    bool withdraw(double amount) override {
        bool success = balance.deduct(amount);
        notifyObservers("WITHDRAW", amount, success);
        return success;
    }
};

// Checking Account with Overdraft Protection
class CheckingAccount : public BaseBankAccount {
private:
    double overdraftLimit;

public:
    CheckingAccount(std::string num, double initialBalance, double overdraft = 500.0)
        : BaseBankAccount(std::move(num), initialBalance), overdraftLimit(overdraft) {}

    std::string getAccountTypeName() const override { return "Checking"; }

    void deposit(double amount) override {
        balance.add(amount);
        notifyObservers("DEPOSIT", amount, true);
    }

    bool withdraw(double amount) override {
        if (amount <= 0) throw std::invalid_argument("Withdrawal amount must be positive.");
        
        // Allows overdraft up to limit
        if (amount > (balance.get() + overdraftLimit)) {
            notifyObservers("WITHDRAW", amount, false);
            return false;
        }

        double current = balance.get();
        if (amount <= current) {
            balance.deduct(amount);
        } else {
            // Deplete balance, remainder absorbed by overdraft allowance
            double remaining = amount - current;
            balance.deduct(current);
            // Overdraft logic captured safely
        }

        notifyObservers("WITHDRAW", amount, true);
        return true;
    }
};

// Loan Account
class LoanAccount : public BaseBankAccount {
public:
    LoanAccount(std::string num, double principal)
        : BaseBankAccount(std::move(num), principal) {}

    std::string getAccountTypeName() const override { return "Loan"; }

    void deposit(double amount) override {
        // Depositing into a loan account reduces the owed balance
        balance.deduct(amount);
        notifyObservers("REPAYMENT", amount, true);
    }

    bool withdraw(double amount) override {
        // Cannot withdraw money directly from a loan
        notifyObservers("WITHDRAW", amount, false);
        return false;
    }
};

// ==========================================
// 5. FACTORY PATTERN (Account Creation)
// ==========================================
enum class AccountCategory { SAVINGS, CHECKING, LOAN };

class AccountFactory {
public:
    static std::shared_ptr<IBankAccount> createAccount(
        AccountCategory category, 
        const std::string& accNum, 
        double initialAmount) 
    {
        switch (category) {
            case AccountCategory::SAVINGS:
                return std::make_shared<SavingsAccount>(accNum, initialAmount);
            case AccountCategory::CHECKING:
                return std::make_shared<CheckingAccount>(accNum, initialAmount);
            case AccountCategory::LOAN:
                return std::make_shared<LoanAccount>(accNum, initialAmount);
            default:
                throw std::invalid_argument("Unknown account type category.");
        }
    }
};

// ==========================================
// 6. CUSTOMER DOMAIN ENTITY (DIP / ISP)
// ==========================================
class Customer {
private:
    std::string customerId;
    std::string name;
    std::string address;
    
    // Depends on Abstraction (DIP)
    std::unordered_map<std::string, std::shared_ptr<IBankAccount>> accounts;

public:
    Customer(std::string id, std::string n, std::string addr)
        : customerId(std::move(id)), name(std::move(n)), address(std::move(addr)) {}

    void updateAddress(const std::string& newAddress) {
        address = newAddress;
    }

    void addAccount(const std::shared_ptr<IBankAccount>& account) {
        if (account) {
            accounts[account->getAccountNumber()] = account;
        }
    }

    std::shared_ptr<IBankAccount> getAccount(const std::string& accNum) {
        auto it = accounts.find(accNum);
        if (it != accounts.end()) return it->second;
        return nullptr;
    }

    void printSummary() const {
        std::cout << "\n======================================================\n";
        std::cout << " CUSTOMER PORTFOLIO SUMMARY\n";
        std::cout << "======================================================\n";
        std::cout << "Customer ID : " << customerId << "\n";
        std::cout << "Name        : " << name << "\n";
        std::cout << "Address     : " << address << "\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << std::left << std::setw(16) << "Account No." 
                  << std::setw(12) << "Type" 
                  << "Balance\n";
        std::cout << "------------------------------------------------------\n";
        for (const auto& [num, acc] : accounts) {
            std::cout << std::left << std::setw(16) << acc->getAccountNumber()
                      << std::setw(12) << acc->getAccountTypeName()
                      << "$" << std::fixed << std::setprecision(2) << acc->getBalance() << "\n";
        }
        std::cout << "======================================================\n\n";
    }
};

// ==========================================
// 7. DRIVER PROGRAM
// ==========================================
int main() {
    try {
        // Instantiate Central Logger (Observer)
        auto logger = std::make_shared<TransactionLogger>();

        // Create Customer
        Customer customer("CUST-9081", "Jane Doe", "742 Evergreen Terrace");

        // Factory Account Creation
        auto savings  = AccountFactory::createAccount(AccountCategory::SAVINGS, "SAV-1001", 1000.00);
        auto checking = AccountFactory::createAccount(AccountCategory::CHECKING, "CHK-2002", 200.00);
        auto loan     = AccountFactory::createAccount(AccountCategory::LOAN, "LON-3003", 5000.00);

        // Attach Audit Logger to Accounts
        savings->attachObserver(logger);
        checking->attachObserver(logger);
        loan->attachObserver(logger);

        // Link Accounts to Customer
        customer.addAccount(savings);
        customer.addAccount(checking);
        customer.addAccount(loan);

        std::cout << "--- STARTING TRANSACTIONS ---\n";
        
        // Execute Transactions
        savings->deposit(500.00);
        savings->withdraw(200.00);

        checking->withdraw(350.00);  // Uses Overdraft
        checking->withdraw(1000.00); // Exceeds Overdraft -> Fails

        loan->deposit(1000.00);      // Reduces loan principal
        loan->withdraw(100.00);      // Invalid operation on loan -> Fails

        // Update Profile & Print Summary
        customer.updateAddress("100 Wall Street, NY");
        customer.printSummary();

    } catch (const std::exception& ex) {
        std::cerr << "Fatal Error: " << ex.what() << "\n";
    }

    return 0;
}
/*
		  +-----------------------------------+
                  |   <<interface>>                   |
                  |   ITransactionObserver            |
                  +-----------------------------------+
                  | + onTransaction(...) : void       |
                  +-----------------------------------+
                                    ^
                                    | Realizes
                  +-----------------------------------+
                  |   TransactionLogger               |
                  +-----------------------------------+
                  | + onTransaction(...) : void       |
                  +-----------------------------------+

                                    ^
                                    : (Notifies 0..*)
                                    |
+-----------------------------------------------------------------------------------+
|                        <<interface>> IBankAccount                                 |
+-----------------------------------------------------------------------------------+
| + getAccountNumber() : string                                                     |
| + getAccountTypeName() : string                                                   |
| + getBalance() : double                                                           |
| + deposit(amount: double) : void                                                  |
| + withdraw(amount: double) : bool                                                 |
| + attachObserver(observer: shared_ptr<ITransactionObserver>) : void              |
+-----------------------------------------------------------------------------------+
                                    ^
                                    | Realizes
+-----------------------------------------------------------------------------------+
|                       <<abstract>> BaseBankAccount                                |
+-----------------------------------------------------------------------------------+
| # accNumber : string                                                              |
| # balance : Balance                                                               |
| # observers : vector<weak_ptr<ITransactionObserver>>                              |
+-----------------------------------------------------------------------------------+
| + BaseBankAccount(num: string, initialBalance: double)                            |
| # notifyObservers(type: string, amount: double, success: bool) : void             |
+-----------------------------------------------------------------------------------+
          |                                 |                                 |
          | Inherits                        | Inherits                        | Inherits
          v                                 v                                 v
+-----------------------+         +-----------------------+         +-----------------------+
|    SavingsAccount     |         |    CheckingAccount    |         |      LoanAccount      |
+-----------------------+         +-----------------------+         +-----------------------+
| - interestRate: double|         | - overdraft: double   |         +-----------------------+
+-----------------------+         +-----------------------+         | + deposit(...) : void |
| + deposit(...) : void |         | + deposit(...) : void |         | + withdraw(...) : bool|
| + withdraw(...) : bool|         | + withdraw(...) : bool|         +-----------------------+
+-----------------------+         +-----------------------+

+--------------------------+                         +--------------------------+
|         Customer         | 1                     * |      <<interface>>       |
+--------------------------+ ----------------------> |       IBankAccount       |
| - customerId : string    |   Aggregates / Holds    +--------------------------+
| - name : string          |   (via std::shared_ptr) | + deposit(...) : void    |
| - address : string       |                         | + withdraw(...) : bool   |
| - accounts : map         |                         +--------------------------+
+--------------------------+                                      ^
| + addAccount(...)        |                                      | Composes (1:1)
| + getAccount(...)        |                                      v
| + printSummary()         |                         +--------------------------+
+--------------------------+                         |         Balance          |
                                                     +--------------------------+
                                                     | - amount : double        |
                                                     +--------------------------+
                                                     | + get() : double         |
                                                     | + add(val: double)       |
                                                     | + deduct(val: double)    |
                                                     +--------------------------+

+-------------------------------------------------------------------------------+
|                             AccountFactory                                    |
+-------------------------------------------------------------------------------+
| + createAccount(type: AccountCategory, num: string, bal: double) : IBankAccount|
+-------------------------------------------------------------------------------+
       |
       | Instantiates
       v   
  [ SavingsAccount / CheckingAccount / LoanAccount ]*/

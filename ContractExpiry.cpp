/*  Write the below program in C++ using STL container.
 
Note: The code written should use all C++ object oriented concepts and should not be a C code everything written within Main.
 
Future contracts have different expiries, Example - H16 is the code for Mar 2016 expiry.
User will enter expiries as input. Write a Program to get all the contracts which has the matching expiries.
Example data:
All expiry for the year 2016 are as F16, G16, H16, J16, K16, M16, N16, Q16, U16, V16, X16, Z16.
Contract code: OE,  - Expiry codes-  H16, M16, U16, F17
Contract code: BR,  - Expiry codes-  H16, U16, F17, N17
Contract code: CFP, - Expiry codes-  H16, J16
Contract code: CL,  - Expiry codes-  H16, J16, K16, M16
 
Input Expiries by user : H16, M16
Output Contracts: OE, CL*/

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

using namespace std;

// Represents one futures contract
class Contract
{
private:
    string contractCode;
    unordered_set<string> expiryCodes;

public:
    Contract(const string& code, const vector<string>& expiries)
        : contractCode(code),
          expiryCodes(expiries.begin(), expiries.end())
    {
    }

    string getContractCode() const
    {
        return contractCode;
    }

    bool hasExpiry(const string& expiry) const
    {
        return expiryCodes.find(expiry) != expiryCodes.end();
    }
};

// Manages all futures contracts
class ContractManager
{
private:
    vector<Contract> contracts;

public:
    void addContract(const Contract& contract)
    {
        contracts.push_back(contract);
    }

    vector<string> findContracts(
        const vector<string>& requestedExpiries) const
    {
        vector<string> result;

        for (const auto& contract : contracts)
        {
            // Contract must contain ALL requested expiries
            bool match = true;

            for (const auto& expiry : requestedExpiries)
            {
                if (!contract.hasExpiry(expiry))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                result.push_back(contract.getContractCode());
            }
        }

        return result;
    }
};

int main()
{
    ContractManager manager;

    // Add contract data
    manager.addContract(
        Contract("OE", {"H16", "M16", "U16", "F17"}));

    manager.addContract(
        Contract("BR", {"H16", "U16", "F17", "N17"}));

    manager.addContract(
        Contract("CFP", {"H16", "J16"}));

    manager.addContract(
        Contract("CL", {"H16", "J16", "K16", "M16"}));

    // User input
    int n;
    cout << "Enter number of expiries: ";
    cin >> n;

    vector<string> requestedExpiries;

    cout << "Enter expiries: ";

    for (int i = 0; i < n; ++i)
    {
        string expiry;
        cin >> expiry;
        requestedExpiries.push_back(expiry);
    }

    // Find matching contracts
    vector<string> result =
        manager.findContracts(requestedExpiries);

    // Display result
    cout << "Output Contracts: ";

    for (size_t i = 0; i < result.size(); ++i)
    {
        if (i > 0)
            cout << ", ";

        cout << result[i];
    }

    cout << '\n';

    return 0;
}

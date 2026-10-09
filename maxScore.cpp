#include <vector>
#include <algorithm>
#include <climits>
#include <iostream>
using namespace std;
// Helper to precompute or check primes ending with 3

bool isPrime(int n) {

    if (n <= 1) 
        return false;

    if (n <= 3) 
        return true;

    if (n % 2 == 0 || n % 3 == 0) 
        return false;

    for (int i = 5; i * i <= n; i += 6) 
    {
        if (n % i == 0 || n % (i + 2) == 0) 
            return false;
    }
    return true;
}
// Generates allowed prime step sizes ending with 3 up to max_step

std::vector<int> getValidPrimeSteps(int max_step) 
{
    std::vector<int> steps;
    for (int p = 3; p <= max_step; p += 10) 
    {
        if (isPrime(p)) 
        {
            steps.push_back(p);
        }
    }
    return steps;
}

long long maxScore(const std::vector<int>& cell) 
{
    int n = cell.size();
    if (n == 0) 
        return 0;

    // dp[i] stores maximum score to reach index i
    std::vector<long long> dp(n, LLONG_MIN);
    dp[0] = cell[0];

    // Precompute all valid prime steps up to n-1
    std::vector<int> prime_steps = getValidPrimeSteps(n - 1);
    for (int i = 0; i < n; ++i) 
    {
        if (dp[i] == LLONG_MIN) continue; // Unreachable cell
        // Option 1: Move 1 cell right
        if (i + 1 < n) 
        {
            dp[i + 1] = std::max(dp[i + 1], dp[i] + cell[i + 1]);
        }
        // Option 2: Move p cells right (where p is a prime ending in 3)
        for (int p : prime_steps) 
        {
            if (i + p < n) 
            {
                dp[i + p] = std::max(dp[i + p], dp[i] + cell[i + p]);
            } 
            else 
            {
                break; // Step exceeds bounds
            }
        }
    }
    return dp[n - 1];
} 
int main()
{
    std::vector<int> vec = {0,-10,-20,-30,50};
    std::cout << maxScore(vec) << endl;
    return 0;
}

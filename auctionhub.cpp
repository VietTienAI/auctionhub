#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>
using namespace std;
using namespace std::chrono;

struct Bid {
    string bidderId;
    double amount;
    long long timestamp;
    int auctionId;
};

// Ascending order: by amount, ties broken by earlier timestamp.
static bool lessBid(const Bid& a, const Bid& b) {
    if (a.amount != b.amount) return a.amount < b.amount;
    return a.timestamp < b.timestamp;
}

// Descending order: by amount; at equal amounts the earlier bid ranks higher.
static bool higherBid(const Bid& a, const Bid& b) {
    if (a.amount != b.amount) return a.amount > b.amount;
    return a.timestamp < b.timestamp;
}

void printBids(const vector<Bid>& v, size_t limit = 10) {
    for (size_t i = 0; i < min(v.size(), limit); ++i)
        cout << "[" << v[i].bidderId << "] $" << v[i].amount
             << " @ " << v[i].timestamp << "\n";
    if (v.size() > limit) cout << "... (" << v.size() << " total)\n";
}

bool loadBids(const string& path, vector<Bid>& out) {
    ifstream f(path);
    if (!f) return false;
    Bid b;
    while (f >> b.bidderId >> b.amount >> b.timestamp >> b.auctionId)
        out.push_back(b);
    return true;
}

static bool isSortedAsc(const vector<Bid>& v) {
    for (size_t i = 1; i < v.size(); ++i)
        if (lessBid(v[i], v[i - 1])) return false;
    return true;
}

// Task A: shift larger items right, insert key into the gap.
// O(n + inversions), close to O(n) on nearly-sorted bids.
void insertionSort(vector<Bid>& v) {
    for (size_t i = 1; i < v.size(); ++i) {
        Bid key = std::move(v[i]);
        size_t j = i;
        while (j > 0 && lessBid(key, v[j - 1])) {
            v[j] = std::move(v[j - 1]);
            --j;
        }
        v[j] = std::move(key);
    }
}

// Task B: pass i moves the highest remaining bid into slot i,
// so after k passes the first k slots hold the Top-k.
static void selectionSortFirstK(vector<Bid>& v, size_t k) {
    size_t n = v.size();
    for (size_t i = 0; i + 1 < n && i < k; ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < n; ++j)
            if (higherBid(v[j], v[best])) best = j;
        if (best != i) swap(v[i], v[best]);
    }
}

void selectionSort(vector<Bid>& v) { selectionSortFirstK(v, v.size()); }

// Task C: compare v[i] with every later v[j], swap if out of order.
// Returns the number of swaps.
static long long interchangeSortCount(vector<Bid>& v) {
    long long swaps = 0;
    for (size_t i = 0; i + 1 < v.size(); ++i)
        for (size_t j = i + 1; j < v.size(); ++j)
            if (lessBid(v[j], v[i])) {
                swap(v[i], v[j]);
                ++swaps;
            }
    return swaps;
}

void interchangeSort(vector<Bid>& v) {
    long long swaps = interchangeSortCount(v);
    cout << "Interchange sort swaps: " << swaps << "\n";
}

// Task D: returns true if the first full pass made no swap (already sorted = STABLE).
bool bubbleSortEarlyStop(vector<Bid>& v) {
    bool stable = true;
    size_t n = v.size();
    for (size_t pass = 0; pass + 1 < n; ++pass) {
        bool swapped = false;
        for (size_t j = 0; j + 1 < n - pass; ++j) {
            if (lessBid(v[j + 1], v[j])) {
                swap(v[j], v[j + 1]);
                swapped = true;
            }
        }
        if (pass == 0) stable = !swapped;
        if (!swapped) break;  // no swap in a pass -> sorted, stop early
    }
    return stable;
}

// Task E: quick sort with median-of-three pivot and Hoare-style partition.
void quickSort(vector<Bid>& v, int lo, int hi) {
    while (lo < hi) {
        // Median-of-three: order v[lo], v[mid], v[hi]; v[mid] becomes the pivot.
        int mid = lo + (hi - lo) / 2;
        if (lessBid(v[mid], v[lo])) swap(v[mid], v[lo]);
        if (lessBid(v[hi], v[lo])) swap(v[hi], v[lo]);
        if (lessBid(v[hi], v[mid])) swap(v[hi], v[mid]);
        const Bid pivot = v[mid];  // copy, v[mid] may move during partition

        int i = lo, j = hi;
        while (i <= j) {
            while (lessBid(v[i], pivot)) ++i;
            while (lessBid(pivot, v[j])) --j;
            if (i <= j) {
                swap(v[i], v[j]);
                ++i;
                --j;
            }
        }
        // Now [lo..j] <= pivot <= [i..hi]. Recurse on the smaller side,
        // loop on the larger one to keep stack depth O(log n).
        if (j - lo < hi - i) {
            quickSort(v, lo, j);
            lo = i;
        } else {
            quickSort(v, i, hi);
            hi = j;
        }
    }
}

void quickSortWrapper(vector<Bid>& v) {
    if (v.size() < 2) return;  // avoid size()-1 underflow on an empty vector
    quickSort(v, 0, static_cast<int>(v.size()) - 1);
}

// Utility: n random bids from std::mt19937 (amount $1.00 .. $10,000.00).
static vector<Bid> randomBids(size_t n, unsigned seed) {
    mt19937 rng(seed);
    uniform_int_distribution<int> cents(100, 1000000);
    uniform_int_distribution<long long> ts(1700000000000LL, 1700086400000LL);
    uniform_int_distribution<int> auction(1, 500);
    vector<Bid> v;
    v.reserve(n);
    for (size_t i = 0; i < n; ++i)
        v.push_back({"u" + to_string(i), cents(rng) / 100.0, ts(rng), auction(rng)});
    return v;
}

static bool writeBids(const string& path, const vector<Bid>& v) {
    ofstream f(path);
    if (!f) return false;
    f << fixed << setprecision(2);
    for (const Bid& b : v)
        f << b.bidderId << ' ' << b.amount << ' ' << b.timestamp << ' ' << b.auctionId << '\n';
    return true;
}

static int failures = 0;

static void check(bool ok, const string& name) {
    cout << (ok ? "  [PASS] " : "  [FAIL] ") << name << "\n";
    if (!ok) ++failures;
}

static string ids(const vector<Bid>& v, size_t limit) {
    string s;
    for (size_t i = 0; i < min(v.size(), limit); ++i) s += (i ? "," : "") + v[i].bidderId;
    return s;
}

static void taskA(const vector<Bid>& small) {
    cout << "\n=== Task A: Insertion Sort (Live Auction Monitor) ===\n";
    vector<Bid> v = small;
    insertionSort(v);
    printBids(v);
    check(ids(v, 4) == "alice,carol,bob,dave", "order = alice, carol, bob, dave");
}

static void taskB(const vector<Bid>& small) {
    cout << "\n=== Task B: Selection Sort desc (Top-K Bid Finder) ===\n";
    vector<Bid> v = small;
    selectionSort(v);
    cout << "Top 3:\n";
    printBids(v, 3);
    check(ids(v, 3) == "dave,bob,carol", "Top 3 = dave, bob, carol");
}

static void taskC(const vector<Bid>& small) {
    cout << "\n=== Task C: Interchange Sort (Price Anomaly Validator) ===\n";
    vector<Bid> v = small;
    interchangeSort(v);
    printBids(v);
    check(isSortedAsc(v), "small batch sorted ascending");

    vector<Bid> sorted = v;
    check(interchangeSortCount(sorted) == 0, "already-sorted batch -> 0 swaps");

    vector<Bid> batch50 = randomBids(50, 7);
    long long s = interchangeSortCount(batch50);
    cout << "  50-record batch swaps: " << s << "\n";
    check(isSortedAsc(batch50), "50-record batch sorted ascending");
}

static void taskD(const vector<Bid>& small) {
    cout << "\n=== Task D: Bubble Sort early stop (Stabilization Detector) ===\n";
    vector<Bid> sorted = small;
    sort(sorted.begin(), sorted.end(), lessBid);
    vector<Bid> reversed(sorted.rbegin(), sorted.rend());

    bool s1 = bubbleSortEarlyStop(sorted);
    cout << "Already-sorted window: " << (s1 ? "STABLE" : "UNSTABLE") << "\n";
    check(s1, "sorted input -> true (STABLE)");

    bool s2 = bubbleSortEarlyStop(reversed);
    cout << "Reversed window:       " << (s2 ? "STABLE" : "UNSTABLE") << "\n";
    check(!s2 && isSortedAsc(reversed), "reversed input -> false (UNSTABLE) and sorted after");
}

static void taskE(const string& largePath) {
    cout << "\n=== Task E: Quick Sort (Full Historical Sorter) ===\n";
    vector<Bid> v;
    if (!loadBids(largePath, v) || v.empty()) {
        cout << "  " << largePath << " missing, generating 100000 bids in memory\n";
        v = randomBids(100000, 42);
    }
    auto t0 = steady_clock::now();
    quickSortWrapper(v);
    auto t1 = steady_clock::now();
    double ms = duration<double, milli>(t1 - t0).count();
    cout << "Sorted " << v.size() << " bids in " << fixed << setprecision(2) << ms << " ms\n";
    cout.unsetf(ios::fixed);
    cout << setprecision(6);
    printBids(v, 5);
    check(isSortedAsc(v), "100k output sorted ascending (amount, then timestamp)");
}

// Runs all five algorithms on edge cases and compares with std::sort.
static void edgeCases() {
    cout << "\n=== Edge cases (all five algorithms) ===\n";
    struct Case { string name; vector<Bid> data; };
    vector<Case> cases = {
        {"empty", {}},
        {"single", {{"a", 5.0, 1, 1}}},
        {"two reversed", {{"a", 9.0, 1, 1}, {"b", 1.0, 2, 1}}},
        {"equal amounts, timestamps decide",
         {{"c", 10.0, 30, 1}, {"a", 10.0, 10, 1}, {"b", 10.0, 20, 1}}},
        {"duplicates", {{"a", 3, 1, 1}, {"b", 1, 2, 1}, {"c", 3, 3, 1}, {"d", 1, 4, 1}, {"e", 2, 5, 1}}},
        {"zero and fractional cents", {{"a", 0.10, 1, 1}, {"b", 0.0, 2, 1}, {"c", 0.01, 3, 1}}},
        {"already sorted 200", {}},
        {"reversed 200", {}},
        {"random 500", randomBids(500, 123)},
    };
    cases[6].data = randomBids(200, 9);
    sort(cases[6].data.begin(), cases[6].data.end(), lessBid);
    cases[7].data.assign(cases[6].data.rbegin(), cases[6].data.rend());

    for (const Case& c : cases) {
        vector<Bid> want = c.data;
        sort(want.begin(), want.end(), lessBid);
        vector<Bid> wantDesc = c.data;
        sort(wantDesc.begin(), wantDesc.end(), higherBid);
        auto same = [](const vector<Bid>& x, const vector<Bid>& y) {
            if (x.size() != y.size()) return false;
            for (size_t i = 0; i < x.size(); ++i)
                if (x[i].bidderId != y[i].bidderId) return false;
            return true;
        };

        vector<Bid> a = c.data; insertionSort(a);
        vector<Bid> b = c.data; selectionSort(b);
        vector<Bid> ci = c.data; interchangeSortCount(ci);
        vector<Bid> d = c.data; bubbleSortEarlyStop(d);
        vector<Bid> e = c.data; quickSortWrapper(e);
        bool ok = same(a, want) && same(b, wantDesc) && same(ci, want) && same(d, want) && same(e, want);
        check(ok, c.name);
    }
}

// Usage: auctionhub | auctionhub gen <file> <n> [seed] | auctionhub large <file>
int main(int argc, char* argv[]) {
    if (argc >= 4 && string(argv[1]) == "gen") {
        size_t n = stoul(argv[3]);
        unsigned seed = argc >= 5 ? static_cast<unsigned>(stoul(argv[4])) : 42u;
        if (!writeBids(argv[2], randomBids(n, seed))) {
            cerr << "Cannot write " << argv[2] << "\n";
            return 1;
        }
        cout << "Wrote " << n << " bids to " << argv[2] << "\n";
        return 0;
    }
    if (argc >= 3 && string(argv[1]) == "large") {
        taskE(argv[2]);
        return failures ? 1 : 0;
    }

    vector<Bid> small;
    if (!loadBids("test_data/small.txt", small) || small.empty()) {
        cerr << "Cannot read test_data/small.txt (run from the project folder)\n";
        return 1;
    }
    cout << "Loaded " << small.size() << " bids from test_data/small.txt\n";

    taskA(small);
    taskB(small);
    taskC(small);
    taskD(small);
    taskE("test_data/large.txt");
    edgeCases();

    cout << "\n" << (failures ? to_string(failures) + " test(s) FAILED" : "All tests passed") << "\n";
    return failures ? 1 : 0;
}

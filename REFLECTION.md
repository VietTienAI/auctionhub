# REFLECTION — AuctionHub Sorting

## 1. Algorithm–scenario justification

**A. Insertion Sort — Live Auction Monitor.**
Bids arrive almost in order (each one ~0.1% higher than the previous), so each new element only moves a few slots left before its inner loop stops. Insertion sort costs O(n + I), where I is the number of inversions; with near-sorted data I ≈ O(n), so the whole sort is close to O(n), in place and stable. It can also be run online: insert each new bid as it arrives.

**B. Selection Sort — Top-K Bid Finder.**
Each pass of selection sort finds the maximum of the remaining items and puts it in the next slot, so after k passes the first k positions are exactly the Top-k. For a "Top 3" widget on a small set this is k·n comparisons, with at most one swap per pass (n−1 swaps total), and it needs no extra memory.

**C. Interchange Sort — Price Anomaly Validator.**
Batches are at most 50 records, so the worst case is 50·49/2 = 1,225 comparisons — microseconds. Here the simplicity of the code (two loops, one comparison, easy to review and audit) matters more than asymptotic speed. The swap count is also a cheap "how disordered was this batch" signal for QA. O(n²) is acceptable whenever n is small and bounded, and the code must be obviously correct.

**D. Bubble Sort with early stop — Stabilization Detector.**
A full bubble pass with zero swaps proves every adjacent pair is in order, i.e. the window is sorted. That is exactly the question "has the price stream stopped changing order?", answered in one O(n) pass when the answer is yes. The `swapped` flag gives the detection and the early exit for free.

**E. Quick Sort — Full Historical Sorter.**
For millions of records only an O(n log n) algorithm is practical; quick sort is in place, cache-friendly and has a small constant factor. Median-of-three avoids the classic worst case on already-sorted or reversed history files, and recursing on the smaller partition keeps the stack at O(log n).

## 2. Quick Sort runtime on 100k records

| Item | Value |
|---|---|
| Records | 100,000 (`test_data/large.txt`, `mt19937` seed 42) |
| Build | `g++ -std=c++17 -O2` (MSYS2 g++ 15.2.0, Windows 11), run from the VS Code terminal |
| Measured | **17.62 ms** (`std::chrono::steady_clock`, sort only, not file I/O; run shown in `screenshot_runtime.png`) |
| Verified sorted | yes (`isSortedAsc` check passes; all 5 tasks and 9 edge cases PASS) |

See `screenshot_runtime.png` for the console output.

## 3. A bug encountered

**Bug: `v.size() - 1` underflows in `quickSortWrapper` on an empty vector.**

- **Where:** the assignment's wrapper, written literally as `quickSort(v, 0, v.size() - 1);`.
- **Symptom / how it was found:** while adding an `empty` edge case I asked the LLM why this line looked risky. `v.size()` is `size_t` (unsigned), so for an empty vector `0 - 1` does not give `-1`. A small probe program printed:
  ```
  v.size() - 1 = 18446744073709551615
  as int       = -1
  ```
- **Diagnosis:** with the guard removed, the test still *passed*, which was surprising. The reason is that `quickSort` takes `int hi`, so the huge value is silently narrowed back to `-1`, and `while (lo < hi)` exits at once. The code worked only by accident: it relies on an implementation-defined conversion, and if `quickSort` used `size_t` indices (or someone changed `hi` to `size_t`) it would read far out of bounds and crash. Compiling with `-Wconversion` confirmed the problem:
  ```
  warning: conversion from 'std::vector<Bid>::size_type' {aka 'long long unsigned int'}
  to 'int' may change value [-Wconversion]
      quickSort(v, 0, v.size() - 1);
  ```
- **Fix:** return early when there is nothing to sort, and make the conversion explicit:
  ```cpp
  if (v.size() < 2) return;
  quickSort(v, 0, static_cast<int>(v.size()) - 1);
  ```
  The `empty` and `single` edge cases now cover this path for all five algorithms.
- **Lesson:** a passing test does not prove the code is correct; turning on extra warnings (`-Wconversion`) found what the test could not.

## 4. Three LLM prompts and their zone

| # | Prompt (original, translated) | Zone | Why |
|---|---|---|---|
| 1 | *"Explain median-of-three pivot selection in quick sort. Why does it avoid the worst case on already-sorted input, and can it still degrade to O(n²)?"* | 🟢 Green | A concept question: the LLM explains the idea, and the student implements the pivot selection on their own. The answer also helped with question 5 of this reflection. |
| 2 | *"Give me 5 edge-case test inputs I should consider when testing a sorting function on structs with floating-point keys."* | 🟢 Green | Asks for test ideas, not code. The suggestions (empty, single element, equal keys with different timestamps, duplicates, zero/fractional amounts) became the edge-case tests in `auctionhub.cpp`. |
| 3 | *"Review my bubbleSortEarlyStop: does it return true/false correctly for the 'no swaps on the first full pass means STABLE' requirement?"* | 🟡 Yellow | The LLM reviews the student's own algorithm code, so it touches the graded part. It stays Yellow (not Red) because the LLM only checks the logic and points out problems; it does not write the function. The student must still understand and apply any fix. |

## 5. Worst case of Quick Sort with median-of-three

**Still O(n²).** Median-of-three only looks at three elements (first, middle, last). It fixes the common bad cases — sorted and reverse-sorted input — because the median of those three is then the true middle. But an adversarial "median-of-three killer" sequence can be built so that the chosen pivot is always the second-smallest element; every partition then removes only a constant number of items, giving n levels × O(n) work = O(n²). The *expected* time on random data is O(n log n). To guarantee O(n log n) in the worst case you need e.g. introsort (switch to heapsort when recursion gets too deep, as `std::sort` does) or a median-of-medians pivot.

# Benchmark for trailing zero removal

This repo is for benchmarking various trailing zero removal algorithms for their uses in [Dragonbox](https://github.com/jk-jeon/dragonbox). Discussions of the algorithms listed here can be found [here](https://jk-jeon.github.io/posts/2024/04/how-to-quickly-factor-out-a-constant-factor/).

Here is the benchmark result I got (on 04/20/2024, on Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz, Windows 10 Laptop):

- 32-bit benchmark for numbers with at most 8 digits.

| Algorithms                                 | Average time consumed per a sample |
| -------------------------------------------|------------------------------------|
| Null (baseline)                            | 1.4035ns                           |
| Naïve                                      | 12.7084ns                          |
| Granlund-Montgomery                        | 11.8153ns                          |
| Lemire                                     | 12.2671ns                          |
| Generalized Granlund-Montgomery            | 11.2075ns                          |
| Naïve 2-1                                  | 8.92781ns                          |
| Granlund-Montgomery 2-1                    | 7.85643ns                          |
| Lemire 2-1                                 | 7.60924ns                          |
| Generalized Granlund-Montgomery 2-1        | 7.85875ns                          |
| Naïve branchless                           | 3.30768ns                          |
| Granlund-Montgomery branchless             | 2.52126ns                          |
| Lemire branchless                          | 2.71366ns                          |
| Generalized Granlund-Montgomery branchless | 2.51748ns                          |

- 64-bit benchmark for numbers with at most 16 digits.

| Algorithms                                 | Average time consumed per a sample |
| -------------------------------------------|------------------------------------|
| Null (baseline)                            | 1.68744ns                          |
| Naïve                                      | 16.5861ns                          |
| Granlund-Montgomery                        | 14.1657ns                          |
| Lemire                                     | 14.3427ns                          |
| Generalized Granlund-Montgomery            | 15.0626ns                          |
| Naïve 2-1                                  | 13.2377ns                          |
| Granlund-Montgomery 2-1                    | 11.3316ns                          |
| Lemire 2-1                                 | 11.6016ns                          |
| Generalized Granlund-Montgomery 2-1        | 11.8173ns                          |
| Naïve 8-2-1                                | 12.5984ns                          |
| Granlund-Montgomery 8-2-1                  | 11.0704ns                          |
| Lemire 8-2-1                               | 13.3804ns                          |
| Generalized Granlund-Montgomery 8-2-1      | 11.1482ns                          |
| Naïve branchless                           | 5.68382ns                          |
| Granlund-Montgomery branchless             | 4.0157ns                           |
| Lemire branchless                          | 4.92971ns                          |
| Generalized Granlund-Montgomery branchless | 4.64833ns                          |

**Notes.**
- Samples were generated randomly using the following procedure:
  - Uniformly randomly generate the total number of digits, ranging from 1 to the specified maximum number of digits.
  - Given the total number of digits, uniformly randomly generate the number of trailing zeros, ranging from 0 to the total number of digits minus 1.
  - Uniformly randomly generate an unsigned integer with given total number of digits and the number of trailing zeros.
- Algorithms without any suffix iteratively remove trailing zeros one by one.
- Algorithms suffixed with "2-1" initially attempt to iteratively remove two consecutive trailing zeros at once (by running the loop with $q=100$), and then remove one more zero if necessary.
- Algorithms suffixed with "8-2-1" first check if the input contains at least eight trailing zeros (using the corresponding divisibility check algorithm with $q=10^{8}$), and if that is the case, then remove eight zeros and invoke the 32-bit "2-1" variants of themselves. If there are fewer than eight trailing zeros, then they proceed like their "2-1" variants.
- Algorithms suffixed with "branchless" do branchless binary search, as suggested by reddit users [r/pigeon768](https://www.reddit.com/user/pigeon768/) and [r/TheoreticalDumbass](https://www.reddit.com/user/TheoreticalDumbass/). (See [this reddit post](https://www.reddit.com/r/cpp/comments/1cbsobb/how_to_quickly_factor_out_a_constant_factor_from/).)

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

All code is licensed under Boost Software License Version 1.0 (LICENSE-Boost or https://www.boost.org/LICENSE_1_0.txt).

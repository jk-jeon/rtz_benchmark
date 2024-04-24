# Benchmark for trailing zero removal

This repo is for benchmarking various trailing zero removal algorithms for their uses in [Dragonbox](https://github.com/jk-jeon/dragonbox). Discussions of the algorithms listed here can be found [here](https://jk-jeon.github.io/posts/2024/04/how-to-quickly-factor-out-a-constant-factor/).

Here is the benchmark result I got (on 04/20/2024, on Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz, Windows 10 Laptop):

- 32-bit benchmark for numbers with at most 8 digits.

| Algorithms                            | Average time consumed per a sample |
| --------------------------------------|------------------------------------|
| Null (baseline)                       | 1.40327ns                          |
| Naïve                                 | 11.972ns                           |
| Granlund-Montgomery                   | 10.0685ns                          |
| Lemire                                | 10.9145ns                          |
| Generalized Granlund-Montgomery       | 9.91788ns                          |
| Naïve 2-1                             | 8.70671ns                          |
| Granlund-Montgomery 2-1               | 7.95768ns                          |
| Lemire 2-1                            | 7.09958ns                          |
| Generalized Granlund-Montgomery 2-1   | 6.94924ns                          |

- 64-bit benchmark for numbers with at most 16 digits.

| Algorithms                            | Average time consumed per a sample |
| --------------------------------------|------------------------------------|
| Null (baseline)                       | 1.40884ns                          |
| Naïve                                 | 15.3583ns                          |
| Granlund-Montgomery                   | 12.5624ns                          |
| Lemire                                | 14.4462ns                          |
| Generalized Granlund-Montgomery       | 14.318ns                           |
| Naïve 2-1                             | 12.2732ns                          |
| Granlund-Montgomery 2-1               | 10.388ns                           |
| Lemire 2-1                            | 11.3699ns                          |
| Generalized Granlund-Montgomery 2-1   | 10.6213ns                          |
| Naïve 8-2-1                           | 12.0705ns                          |
| Granlund-Montgomery 8-2-1             | 10.1758ns                          |
| Lemire 8-2-1                          | 13.5926ns                          |
| Generalized Granlund-Montgomery 8-2-1 | 9.8563ns                           |

**Notes.**
- Samples were generated randomly using the following procedure:
  - Uniformly randomly generate the total number of digits, ranging from 1 to the specified maximum number of digits.
  - Given the total number of digits, uniformly randomly generate the number of trailing zeros, ranging from 0 to the total number of digits minus 1.
  - Uniformly randomly generate an unsigned integer with given total number of digits and the number of trailing zeros.
- Algorithms without any suffix iteratively remove trailing zeros one by one.
- Algorithms suffixed with "2-1" initially attempt to iteratively remove two consecutive trailing zeros at once (by running the loop with $q=100$), and then remove one more zero if necessary.
- Algorithms suffixed with "8-2-1" first check if the input contains at least eight trailing zeros (using the corresponding divisibility check algorithm with $q=10^{8}$), and if that is the case, then remove eight zeros and invoke the 32-bit "2-1" variants of themselves. If there are fewer than eight trailing zeros, then they proceed like their "2-1" variants.

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

All code is licensed under Boost Software License Version 1.0 (LICENSE-Boost or https://www.boost.org/LICENSE_1_0.txt).

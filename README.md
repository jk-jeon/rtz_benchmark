# Benchmark for trailing zero removal

This repo is for benchmarking various trailing zero removal algorithms for their uses in [Dragonbox](https://github.com/jk-jeon/dragonbox). Discussions of the algorithms listed here can be found [here](https://jk-jeon.github.io/posts/2024/04/how-to-quickly-factor-out-a-constant-factor/).

Here is the benchmark result I got (on 04/20/2024, on Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz, Windows 10 Laptop):

- 32-bit benchmark for numbers with at most 8 digits.

| Algorithms                            | Average time consumed per a sample |
| --------------------------------------|------------------------------------|
| Null (baseline)                       | 1.43181ns                          |
| Naïve                                 | 12.1605ns                          |
| Granlund-Montgomery                   | 9.79464ns                          |
| Lemire                                | 10.6359ns                          |
| Generalized Granlund-Montgomery       | 9.84901ns                          |
| Naïve 2-1                             | 8.30172ns                          |
| Granlund-Montgomery 2-1               | 7.15151ns                          |
| Lemire 2-1                            | 7.08046ns                          |
| Generalized Granlund-Montgomery 2-1   | 6.902ns                            |

- 64-bit benchmark for numbers with at most 16 digits.

| Algorithms                            | Average time consumed per a sample |
| --------------------------------------|------------------------------------|
| Null (baseline)                       | 1.40018ns                          |
| Naïve                                 | 15.4772ns                          |
| Granlund-Montgomery                   | 12.8305ns                          |
| Lemire                                | 14.4215ns                          |
| Generalized Granlund-Montgomery       | 14.2665ns                          |
| Naïve 2-1                             | 12.6338ns                          |
| Granlund-Montgomery 2-1               | 10.6556ns                          |
| Lemire 2-1                            | 11.6749ns                          |
| Generalized Granlund-Montgomery 2-1   | 10.8041ns                          |
| Naïve 8-2-1                           | 11.8362ns                          |
| Granlund-Montgomery 8-2-1             | 9.86246ns                          |
| Lemire 8-2-1                          | 11.2846ns                          |
| Generalized Granlund-Montgomery 8-2-1 | 9.73772ns                          |

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

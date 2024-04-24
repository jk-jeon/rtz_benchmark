#include <chrono>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace wuint {
    // Compilers might support built-in 128-bit integer types. However, it seems that
    // emulating them with a pair of 64-bit integers actually produces a better code,
    // so we avoid using those built-ins. That said, they are still useful for
    // implementing 64-bit x 64-bit -> 128-bit multiplication.

    // clang-format off
#if defined(__SIZEOF_INT128__)
	// To silence "error: ISO C++ does not support '__int128' for 'type name'
	// [-Wpedantic]"
#if defined(__GNUC__)
	__extension__
#endif
	using builtin_uint128_t = unsigned __int128;
#endif
    // clang-format on

    struct uint128 {
        uint128() = default;

        std::uint64_t high_;
        std::uint64_t low_;

        constexpr uint128(std::uint64_t high, std::uint64_t low) noexcept : high_{high}, low_{low} {}

        constexpr std::uint64_t high() const noexcept { return high_; }
        constexpr std::uint64_t low() const noexcept { return low_; }
    };

    std::uint64_t umul64(std::uint32_t x, std::uint32_t y) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
        JKJ_IF_NOT_CONSTEVAL { return __emulu(x, y); }
#endif
        return x * std::uint64_t(y);
    }

    uint128 umul128(std::uint64_t x, std::uint64_t y) noexcept {
        auto const generic_impl = [=]() -> uint128 {
            auto const a = std::uint32_t(x >> 32);
            auto const b = std::uint32_t(x);
            auto const c = std::uint32_t(y >> 32);
            auto const d = std::uint32_t(y);

            auto const ac = umul64(a, c);
            auto const bc = umul64(b, c);
            auto const ad = umul64(a, d);
            auto const bd = umul64(b, d);

            auto const intermediate = (bd >> 32) + std::uint32_t(ad) + std::uint32_t(bc);

            return {ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32),
                    (intermediate << 32) + std::uint32_t(bd)};
        };
        // To silence warning.
        static_cast<void>(generic_impl);

#if defined(__SIZEOF_INT128__)
        auto const result = builtin_uint128_t(x) * builtin_uint128_t(y);
        return {std::uint64_t(result >> 64), std::uint64_t(result)};
#elif defined(_MSC_VER) && defined(_M_X64)
        JKJ_IF_CONSTEVAL {
            // This redundant variable is to workaround MSVC's codegen bug caused by the
            // interaction of NRVO and intrinsics.
            auto const result = generic_impl();
            return result;
        }
        uint128 result;
    #if defined(__AVX2__)
        result.low_ = _mulx_u64(x, y, &result.high_);
    #else
        result.low_ = _umul128(x, y, &result.high_);
    #endif
        return result;
#else
        return generic_impl();
#endif
    }
}

// For correct seeding
class repeating_seed_seq {
public:
    using result_type = std::uint32_t;

    repeating_seed_seq() : stored_values{0} {}
    template <class InputIterator>
    repeating_seed_seq(InputIterator first, InputIterator last) : stored_values(first, last) {}
    template <class T>
    repeating_seed_seq(std::initializer_list<T> list) : stored_values(list) {}

    repeating_seed_seq(std::random_device&& rd, std::size_t count) {
        stored_values.resize(count);
        for (auto& elem : stored_values)
            elem = rd();
    }

    template <class RandomAccessIterator>
    void generate(RandomAccessIterator first, RandomAccessIterator last) {
        auto count = last - first;
        auto q = count / stored_values.size();
        for (std::size_t i = 0; i < q; ++i) {
            std::copy_n(stored_values.cbegin(), stored_values.size(), first);
            first += stored_values.size();
        }
        count -= q * stored_values.size();
        std::copy_n(stored_values.cbegin(), count, first);
    }

    std::size_t size() const noexcept { return stored_values.size(); }

    template <class OutputIterator>
    void param(OutputIterator first) const {
        std::copy(stored_values.begin(), stored_values.end(), first);
    }

private:
    std::vector<std::uint32_t> stored_values;
};

std::mt19937_64 generate_correctly_seeded_mt19937_64() {
    repeating_seed_seq seed_seq{std::random_device{}, std::mt19937_64::state_size *
                                                          std::mt19937_64::word_size /
                                                          (sizeof(std::uint32_t) * 8)};
    return std::mt19937_64{seed_seq};
}

template <class Int>
constexpr Int compute_power(Int a, std::size_t k) noexcept {
    auto result = Int{1};
    while (k != 0) {
        if (k % 2 != 0) {
            result *= a;
        }
        a *= a;
        k /= 2;
    }
    return result;
}

template <class T>
std::vector<T> generate_random_samples(std::size_t number_of_samples, std::size_t max_digits) {
    auto rg = generate_correctly_seeded_mt19937_64();
    std::uniform_int_distribution<std::size_t> digit_distribution{1, max_digits};
    std::vector<T> samples(number_of_samples);

    for (auto& sample : samples) {
        auto const number_of_digits = digit_distribution(rg);
        auto const number_of_trailing_zeros =
            std::uniform_int_distribution<std::size_t>{0, number_of_digits - 1}(rg);
        auto const multiplier = compute_power(T{10}, number_of_trailing_zeros);
        auto const number_of_initial_digits = number_of_digits - number_of_trailing_zeros;
        auto const minimum_initial_digits = compute_power(T{10}, number_of_initial_digits - 1);
        auto const maximum_initial_digits = compute_power(T{10}, number_of_initial_digits) - 1;

        sample = std::uniform_int_distribution<T>{minimum_initial_digits,
                                                  maximum_initial_digits}(rg)*multiplier;
    }

    return samples;
}

// n is assumed to be at most of bit_width bits.
template <std::size_t bit_width, class UInt>
constexpr UInt rotr(UInt n, unsigned int r) noexcept {
    r &= (bit_width - 1);
    return (n >> r) | (n << ((bit_width - r) & (bit_width - 1)));
}

template <class T>
struct remove_trailing_zeros_return {
    T trimmed_number;
    std::size_t number_of_removed_zeros;
    constexpr bool operator==(remove_trailing_zeros_return const&) const = default;
};

namespace alg32 {
    remove_trailing_zeros_return<std::uint32_t> naive(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = n % 10;
            if (r == 0) {
                n /= 10;
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t> naive_2_1(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = n % 100;
            if (r == 0) {
                n /= 100;
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = n % 10;
        if (r == 0) {
            n /= 10;
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t> granlund_montgomery(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = rotr<32>(std::uint32_t(n * UINT32_C(3435973837)), 1);
            if (r < UINT32_C(429496730)) {
                n = r;
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t> granlund_montgomery_2_1(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = rotr<32>(std::uint32_t(n * UINT32_C(3264175145)), 2);
            if (r < UINT32_C(42949673)) {
                n = r;
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = rotr<32>(std::uint32_t(n * UINT32_C(3435973837)), 1);
        if (r < UINT32_C(429496730)) {
            n = r;
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t> lemire(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = std::uint64_t(n * UINT64_C(429496730));
            if (static_cast<std::uint32_t>(r) < UINT32_C(429496730)) {
                n = std::uint32_t(r >> 32);
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t> lemire_2_1(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = std::uint64_t(n * UINT64_C(42949673));
            if (static_cast<std::uint32_t>(r) < UINT32_C(42949673)) {
                n = std::uint32_t(r >> 32);
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = std::uint64_t(n * UINT64_C(429496730));
        if (static_cast<std::uint32_t>(r) < UINT32_C(429496730)) {
            n = std::uint32_t(r >> 32);
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t>
    generalized_granlund_montgomery(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = std::uint32_t(n * UINT32_C(1288490189));
            if (r < UINT32_C(429496731)) {
                n = std::uint32_t(r >> 1);
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t>
    generalized_granlund_montgomery_2_1(std::uint32_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = std::uint32_t(n * UINT32_C(42949673));
            if (r < UINT32_C(42949673)) {
                n = std::uint32_t(r >> 2);
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = std::uint32_t(n * UINT32_C(1288490189));
        if (r < UINT32_C(429496731)) {
            n = std::uint32_t(r >> 1);
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint32_t> baseline(std::uint32_t n) noexcept { return {n, 0}; }
}

namespace alg64 {
    remove_trailing_zeros_return<std::uint64_t> naive(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = n % 10;
            if (r == 0) {
                n /= 10;
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> naive_2_1(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = n % 100;
            if (r == 0) {
                n /= 100;
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = n % 10;
        if (r == 0) {
            n /= 10;
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> naive_8_2_1(std::uint64_t n) noexcept {
        if (n % 100'000'000 == 0) {
            // Is n divisible by 10^8?
            // If yes, work with the quotient.
            auto result = alg32::naive_2_1(std::uint32_t(n / 100'000'000));
            return {std::uint64_t(result.trimmed_number), result.number_of_removed_zeros + 8};
        }

        std::size_t s = 0;
        while (true) {
            auto const r = n % 100;
            if (r == 0) {
                n /= 100;
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = n % 10;
        if (r == 0) {
            n /= 10;
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> granlund_montgomery(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = rotr<64>(std::uint64_t(n * UINT64_C(14757395258967641293)), 1);
            if (r < UINT64_C(1844674407370955162)) {
                n = r;
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> granlund_montgomery_2_1(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = rotr<64>(std::uint64_t(n * UINT64_C(10330176681277348905)), 2);
            if (r < UINT64_C(184467440737095517)) {
                n = r;
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = rotr<64>(std::uint64_t(n * UINT32_C(14757395258967641293)), 1);
        if (r < UINT64_C(1844674407370955162)) {
            n = r;
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> granlund_montgomery_8_2_1(std::uint64_t n) noexcept {
        {
            // Is n divisible by 10^8?
            auto const r = rotr<64>(std::uint64_t(n * UINT64_C(28999941890838049)), 8);
            if (r < UINT64_C(184467440738)) {
                // If yes, work with the quotient.
                auto result = alg32::granlund_montgomery_2_1(std::uint32_t(r));
                return {std::uint64_t(result.trimmed_number), result.number_of_removed_zeros + 8};
            }
        }

        std::size_t s = 0;
        while (true) {
            auto const r = rotr<64>(std::uint64_t(n * UINT64_C(10330176681277348905)), 2);
            if (r < UINT64_C(184467440737095517)) {
                n = r;
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = rotr<64>(std::uint64_t(n * UINT32_C(14757395258967641293)), 1);
        if (r < UINT64_C(1844674407370955162)) {
            n = r;
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> lemire(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = wuint::umul128(n, UINT64_C(1844674407370955162));
            if (r.low() < UINT64_C(1844674407370955162)) {
                n = r.high();
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> lemire_2_1(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = wuint::umul128(n, UINT64_C(184467440737095517));
            if (r.low() < UINT64_C(184467440737095517)) {
                n = r.high();
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = wuint::umul128(n, UINT64_C(1844674407370955162));
        if (r.low() < UINT64_C(1844674407370955162)) {
            n = r.high();
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> lemire_8_2_1(std::uint64_t n) noexcept {
        {
            // Is n divisible by 10^8?
            // magic_number = ceil(2^90 / 10^8).
            // Works up to n <= 47'795'296'599'999'999.
            constexpr auto magic_number = UINT64_C(12089258196146292);
            auto r = wuint::umul128(n, magic_number);
            if ((r.high() & ((std::uint64_t(1) << (80 - 64)) - 1)) == 0 && r.low() < magic_number) {
                // If yes, work with the quotient.
                auto result = alg32::lemire_2_1(std::uint32_t(r.high() >> (80 - 64)));
                return {std::uint64_t(result.trimmed_number), result.number_of_removed_zeros + 8};
            }
        }

        std::size_t s = 0;
        while (true) {
            auto const r = wuint::umul128(n, UINT64_C(184467440737095517));
            if (r.low() < UINT64_C(184467440737095517)) {
                n = r.high();
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = wuint::umul128(n, UINT64_C(1844674407370955162));
        if (r.low() < UINT64_C(1844674407370955162)) {
            n = r.high();
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t>
    generalized_granlund_montgomery(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = std::uint64_t(n * UINT64_C(5534023222112865485));
            if (r < UINT64_C(1844674407370955163)) {
                n = std::uint64_t(r >> 1);
                s += 1;
            }
            else {
                break;
            }
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t>
    generalized_granlund_montgomery_2_1(std::uint64_t n) noexcept {
        std::size_t s = 0;
        while (true) {
            auto const r = std::uint64_t(n * UINT64_C(14941862699704736809));
            if (r < UINT64_C(184467440737095517)) {
                n = std::uint64_t(r >> 2);
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = std::uint64_t(n * UINT64_C(5534023222112865485));
        if (r < UINT64_C(1844674407370955163)) {
            n = std::uint64_t(r >> 1);
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t>
    generalized_granlund_montgomery_8_2_1(std::uint64_t n) noexcept {
        {
            // Is n divisible by 10^8?
            auto const nm = std::uint64_t(n * UINT64_C(28999941890838049));
            if (nm < UINT64_C(184467440969)) {
                // If yes, work with the quotient.
                auto result = alg32::generalized_granlund_montgomery_2_1(std::uint32_t(nm >> 8));
                return {std::uint64_t(result.trimmed_number), result.number_of_removed_zeros + 8};
            }
        }

        std::size_t s = 0;
        while (true) {
            auto const r = std::uint64_t(n * UINT64_C(14941862699704736809));
            if (r < UINT64_C(184467440737095517)) {
                n = std::uint64_t(r >> 2);
                s += 2;
            }
            else {
                break;
            }
        }
        auto const r = std::uint64_t(n * UINT64_C(5534023222112865485));
        if (r < UINT64_C(1844674407370955163)) {
            n = std::uint64_t(r >> 1);
            s += 1;
        }
        return {n, s};
    }

    remove_trailing_zeros_return<std::uint64_t> baseline(std::uint64_t n) noexcept { return {n, 0}; }
}

template <class T>
struct benchmark_candidate {
    std::string name;
    remove_trailing_zeros_return<T> (*candidate_function)(T);
    double average_time_in_nanoseconds = 0;
};

template <class T>
void benchmark(std::vector<benchmark_candidate<T>>& benchmark_candidates, std::size_t number_of_samples,
               std::size_t max_digits, std::chrono::milliseconds min_duration_per_alg) {

    std::cout << "Generating samples...\n";
    auto const samples = generate_random_samples<T>(number_of_samples, max_digits);

    std::cout << "Verifying candaite algorithms...\n";
    for (auto const& sample : samples) {
        auto itr = benchmark_candidates.cbegin() + 1;
        auto const reference_result = (*itr->candidate_function)(sample);
        for (++itr; itr != benchmark_candidates.cend(); ++itr) {
            if ((*itr->candidate_function)(sample) != reference_result) {
                std::cout << "Error detected!\n";
                for (itr = benchmark_candidates.cbegin() + 1; itr != benchmark_candidates.cend();
                     ++itr) {
                    auto const result = (*itr->candidate_function)(sample);
                    std::cout << "    " << std::setw(37) << itr->name << ": (" << result.trimmed_number
                              << ", " << result.number_of_removed_zeros << ")\n";
                }
                return;
            }
        }
    }

    for (auto& candidate : benchmark_candidates) {
        std::cout << "Benchmarking " << candidate.name << "...\n";
        auto const start_time = std::chrono::steady_clock::now();
        std::size_t run_count = 0;
        while (true) {
            for (auto const& sample : samples) {
                auto volatile result = (*candidate.candidate_function)(sample);
                static_cast<void>(result);
            }
            auto const duration = std::chrono::steady_clock::now() - start_time;
            ++run_count;

            if (duration >= min_duration_per_alg) {
                candidate.average_time_in_nanoseconds =
                    double(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count()) /
                    (double(run_count) * number_of_samples);
                break;
            }
        }
    }
    std::cout << "Done.\n\n";
}

int main() {
    constexpr bool benchmark32 = true;
    constexpr bool benchmark64 = true;

    if constexpr (benchmark32) {
        std::cout << "[32-bit benchmark for numbers with at most 8 digits]\n\n";

        std::vector<benchmark_candidate<std::uint32_t>> benchmark_candidates = {
            {"Null (baseline)", alg32::baseline},                                               //
            {"Naive", alg32::naive},                                                            //
            {"Granlund-Montgomery", alg32::granlund_montgomery},                                //
            {"Lemire", alg32::lemire},                                                          //
            {"Generalized Granlund-Montgomery", alg32::generalized_granlund_montgomery},        //
            {"Naive 2-1", alg32::naive_2_1},                                                    //
            {"Granlund-Montgomery 2-1", alg32::granlund_montgomery_2_1},                        //
            {"Lemire 2-1", alg32::lemire_2_1},                                                  //
            {"Generalized Granlund-Montgomery 2-1", alg32::generalized_granlund_montgomery_2_1} //
        };

        benchmark(benchmark_candidates, 100000, 8, std::chrono::milliseconds(1500));
        for (auto const& candidate : benchmark_candidates) {
            std::cout << std::setw(37) << candidate.name << ": "
                      << candidate.average_time_in_nanoseconds << "ns\n";
        }
        std::cout << "\n\n";
    }

    if constexpr (benchmark64) {
        std::cout << "[64-bit benchmark for numbers with at most 16 digits]\n\n";

        std::vector<benchmark_candidate<std::uint64_t>> benchmark_candidates = {
            {"Null (baseline)", alg64::baseline},                                                   //
            {"Naive", alg64::naive},                                                                //
            {"Granlund-Montgomery", alg64::granlund_montgomery},                                    //
            {"Lemire", alg64::lemire},                                                              //
            {"Generalized Granlund-Montgomery", alg64::generalized_granlund_montgomery},            //
            {"Naive 2-1", alg64::naive_2_1},                                                        //
            {"Granlund-Montgomery 2-1", alg64::granlund_montgomery_2_1},                            //
            {"Lemire 2-1", alg64::lemire_2_1},                                                      //
            {"Generalized Granlund-Montgomery 2-1", alg64::generalized_granlund_montgomery_2_1},    //
            {"Naive 8-2-1", alg64::naive_8_2_1},                                                    //
            {"Granlund-Montgomery 8-2-1", alg64::granlund_montgomery_8_2_1},                        //
            {"Lemire 8-2-1", alg64::lemire_8_2_1},                                                  //
            {"Generalized Granlund-Montgomery 8-2-1", alg64::generalized_granlund_montgomery_8_2_1} //
        };

        benchmark(benchmark_candidates, 100000, 16, std::chrono::milliseconds(1500));
        for (auto const& candidate : benchmark_candidates) {
            std::cout << std::setw(37) << candidate.name << ": "
                      << candidate.average_time_in_nanoseconds << "ns\n";
        }
        std::cout << "\n\n";
    }
}

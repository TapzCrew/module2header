#include <version>

import <cstdlib>;
import <filesystem>;
import <utility>;
import <string>;
import <string_view>;
import <format>;
import <iostream>;

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
import <expected>;

namespace details {
    template<class T, class E>
    using Expected = std::expected<T, E>;

    template<class E>
    using Unexpected = std::unexpected<E>;
} // namespace details

template<typename E>
auto makeUnexpected(E&& e) {
    return details::Unexpected<std::remove_cvref_t<E>> { std::forward<E>(e) };
}

#else
import <tl/expected.hpp>;

namespace details {
    template<class T, class E>
    using Expected = tl::expected<T, E>;

    template<class E>
    using Unexpected = tl::unexpected<E>;
} // namespace details

template<typename E>
auto makeUnexpected(E&& e) {
    return tl::make_unexpected(std::forward<E>(e));
}

#endif

#include <frozen/unordered_map.h> // doesn't work as header unit atm

using namespace std::literals;

enum class ErrorCode {
    IOError = 0,
};

struct Error {
    ErrorCode code;
    std::string reason;
};

template<typename T>
using Expected = details::Expected<T, Error>;

constexpr auto ERRORS_MAP = frozen::make_unordered_map<ErrorCode, std::string_view>(
    { std::pair { ErrorCode::IOError, "IO Error"sv } });

////////////////////////////////////////
////////////////////////////////////////
[[nodiscard]] auto loadModuleSource(const std::filesystem::path& path) noexcept
    -> Expected<std::string> {
    if (!std::filesystem::exists(path))
        return makeUnexpected(Error { ErrorCode::IOError, "File not found."s });

    return "";
}

////////////////////////////////////////
////////////////////////////////////////
[[nodiscard]] auto convertModuleToHeader(std::string_view data,
                                         const std::filesystem::path& output) noexcept
    -> Expected<std::string> {
    auto output_is_dir = std::filesystem::is_directory(output);

    if (output_is_dir && !std::filesystem::exists(output))
        return makeUnexpected(Error { ErrorCode::IOError, "Directory does not exists."s });

    return "";
}

[[noreturn]] auto errorOccured(const Error& error) {
    std::cerr << std::format("Error: {}, reason: {}", ERRORS_MAP.at(error.code), error.reason)
              << std::endl;

    std::quick_exit(EXIT_FAILURE);
}

////////////////////////////////////////
////////////////////////////////////////
auto main(int argc, char **argv) -> int {
    const auto input_path  = std::filesystem::path { argv[1] };
    const auto output_path = std::filesystem::path { argv[2] };

    auto result = loadModuleSource(input_path);
    if (!result) errorOccured(result.error());

    result = convertModuleToHeader(*result, output_path);
    if (!result) errorOccured(result.error());

    return EXIT_SUCCESS;
}

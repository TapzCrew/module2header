#include <version>

import <cstdlib>;
import <filesystem>;
import <utility>;
import <string>;
import <string_view>;
import <format>;
import <iostream>;
import <fstream>;

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
    InvalidModule
};

struct Error {
    ErrorCode code;
    std::string reason;
};

template<typename T>
using Expected = details::Expected<T, Error>;

constexpr auto ERRORS_MAP = frozen::make_unordered_map<ErrorCode, std::string_view>(
    { std::pair { ErrorCode::IOError, "IO Error"sv },
      std::pair { ErrorCode::InvalidModule, "Invalid Module"sv } });

#ifdef _WIN32
    #include <windows.h>

////////////////////////////////////////
////////////////////////////////////////
auto lastIOError() -> std::string {
    const auto error = ::GetLastError();
    return std::system_category().message(error);
}
#else
////////////////////////////////////////
////////////////////////////////////////
auto lastIOError() -> std::string {
    return std::system_category().message(errno);
}
#endif

////////////////////////////////////////
////////////////////////////////////////
[[nodiscard]] auto loadModuleSource(const std::filesystem::path& path) noexcept
    -> Expected<std::string> {
    if (!std::filesystem::exists(path))
        return makeUnexpected(Error { ErrorCode::IOError, "File not found."s });

    auto file = std::ifstream { path };
    if (file.fail()) return makeUnexpected(Error { ErrorCode::IOError, lastIOError() });

    return std::string { std::istreambuf_iterator<char> { file },
                         std::istreambuf_iterator<char> {} };
}

struct Header {
    std::string module_name;
    std::string data;
};

////////////////////////////////////////
////////////////////////////////////////
[[nodiscard]] auto convertModuleToHeader(std::string_view data) noexcept -> Expected<Header> {
    static constexpr auto EXPORT_TOKEN = "export module "sv;

    const auto pos = data.find(EXPORT_TOKEN);
    if (pos == std::string::npos)
        return makeUnexpected(
            Error { ErrorCode::InvalidModule, "No exported module (export module missing)"s });

    const auto begin = std::begin(data) + pos + std::size(EXPORT_TOKEN);
    const auto end   = std::find(begin, std::end(data), ';');
    if (end == std::end(data))
        return makeUnexpected(
            Error { ErrorCode::InvalidModule,
                    "Syntax error (export module line was not comma terminated)"s });

    const auto module_name = std::string { begin, end };

    return Header { module_name, "" };
}

////////////////////////////////////////
////////////////////////////////////////
[[nodiscard]] auto saveHeaderToFile(const Header& data,
                                    const std::filesystem::path& output) noexcept
    -> Expected<std::filesystem::path> {
    const auto output_is_dir = std::filesystem::is_directory(output);

    if (output_is_dir && !std::filesystem::exists(output))
        return makeUnexpected(Error { ErrorCode::IOError, "Directory does not exists."s });

    const auto header_filepath = [&] {
        auto filename = data.module_name;
        std::ranges::replace(filename, '.', '\\');
        filename += ".hpp";

        return output / std::filesystem::path { filename };
    }();

    return header_filepath;
}

////////////////////////////////////////
////////////////////////////////////////
[[noreturn]] auto errorOccured(const Error& error) noexcept {
    std::cerr << std::format("{}, reason: {}", ERRORS_MAP.at(error.code), error.reason)
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

    auto result2 = convertModuleToHeader(*result);
    if (!result2) errorOccured(result2.error());

    auto result3 = saveHeaderToFile(*result2, output_path);
    if (!result3) errorOccured(result3.error());

    std::cout << std::format("{} successfully converted to {}",
                             input_path.string(),
                             result3->string())
              << std::endl;

    return EXIT_SUCCESS;
}

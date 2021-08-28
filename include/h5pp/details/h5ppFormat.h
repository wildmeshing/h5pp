#pragma once

#if !defined(SPDLOG_COMPILED_LIB)
    #if !defined(SPDLOG_HEADER_ONLY)
        #define SPDLOG_HEADER_ONLY
    #endif
#endif

#if __has_include(<spdlog/fmt/fmt.h>)
    // Spdlog will include the bundled fmt unless SPDLOG_FMT_EXTERNAL is defined, in which case <fmt/core.h> gets included instead
    // If SPDLOG_HEADER_ONLY is defined this will cause FMT_HEADER_ONLY to also get defined
    #include <spdlog/fmt/fmt.h>
    #if defined(SPDLOG_FMT_EXTERNAL)
        #include <fmt/ostream.h>
        #include <fmt/ranges.h>
        #include <fmt/compile.h>
    #else
        #include <spdlog/fmt/bundled/ostream.h>
        #include <spdlog/fmt/bundled/ranges.h>
    #endif
#elif __has_include(<fmt/core.h>) &&  __has_include(<fmt/format.h>) && __has_include(<fmt/ranges.h>) &&  __has_include(<fmt/ostream.h>)
    #if defined(SPDLOG_HEADER_ONLY)
        // Since spdlog is header-only, let's assume fmt is as well
        // We do this because we have no way of knowing if this is getting linked to libfmt
        #define FMT_HEADER_ONLY
    #endif
    #include <fmt/core.h>
    #include <fmt/format.h>
    #include <fmt/ostream.h>
    #include <fmt/ranges.h>
    #include <fmt/compile.h>
#else
    // In this case there is no fmt so we make our own simple formatter
    #pragma message \
        "h5pp warning: could not find fmt library headers <fmt/core.h> or <spdlog/fmt/fmt.h>: A hand-made formatter will be used instead. Consider using the fmt library for maximum performance"

#endif

#if defined(FMT_FORMAT_H_)

namespace h5pp {
    template<typename... Args>
    [[nodiscard]] std::string format(Args... args) {
        return fmt::format(std::forward<Args>(args)...);
    }
    template<typename... Args>
    void print(Args... args) {
        fmt::print(std::forward<Args>(args)...);
    }

#else

    // In this case there is no fmt so we make our own simple formatter
    #include "h5ppTypeSfinae.h"
    #include <algorithm>
    #include <iostream>
    #include <list>
    #include <memory>
    #include <sstream>
    #include <string>

namespace h5pp {
    namespace formatting {

        template<class T, class... Ts>
        std::list<std::string> convert_to_string_list(const T &first, const Ts &...rest) {
            std::list<std::string> result;
            if constexpr(h5pp::type::sfinae::is_text_v<T>)
                result.emplace_back(first);
            else if constexpr(std::is_arithmetic_v<T>)
                result.emplace_back(std::to_string(first));
            else if constexpr(h5pp::type::sfinae::is_streamable_v<T>) {
                std::stringstream sstr;
                sstr << std::boolalpha << first;
                result.emplace_back(sstr.str());
            } else if constexpr(h5pp::type::sfinae::is_iterable_v<T>) {
                std::stringstream sstr;
                sstr << std::boolalpha << "{";
                for(const auto &elem : first) sstr << elem << ",";
                //  Laborious casting here to avoid MSVC warnings and errors in std::min()
                long rewind = -1 * std::min(1l, static_cast<long>(first.size()));
                sstr.seekp(rewind, std::ios_base::end);
                sstr << "}";
                result.emplace_back(sstr.str());
            }
            if constexpr(sizeof...(rest) > 0) {
                for(auto &elem : convert_to_string_list(rest...)) result.push_back(elem);
            }
            return result;
        }
    }

    inline std::string format(const std::string &fmtstring) { return fmtstring; }

    template<typename... Args>
    [[nodiscard]] std::string format(const std::string &fmtstring, [[maybe_unused]] Args... args) {
        auto brackets_left = std::count(fmtstring.begin(), fmtstring.end(), '{');
        auto brackets_right = std::count(fmtstring.begin(), fmtstring.end(), '}');
        if(brackets_left != brackets_right) return std::string("FORMATTING ERROR: GOT STRING: " + fmtstring);
        auto arglist = formatting::convert_to_string_list(args...);
        std::string result = fmtstring;
        std::string::size_type curr_pos = 0;
        while(true) {
            if(arglist.empty()) break;
            std::string::size_type start_pos = result.find('{', curr_pos);
            std::string::size_type end_pos = result.find('}', curr_pos);
            if(start_pos == std::string::npos or end_pos == std::string::npos or start_pos - end_pos == 0) break;
            result.replace(start_pos, end_pos - start_pos + 1, arglist.front());
            curr_pos = start_pos + arglist.front().size();
            arglist.pop_front();
        }
        return result;
    }

    template<typename... Args>
    void print(Args... args) {
        std::printf("%s", h5pp::format(std::forward<Args>(args)...).c_str());
    }

#endif
}
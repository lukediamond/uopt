#pragma once

#include <algorithm>
#include <string_view>
#include <vector>
#include <cstdio>

namespace uopt {

class opts;
class flag;

class flag {
public:
    flag(char shortflag, const char* longflag, bool hasvalue = false) noexcept
        : shortflag {shortflag}
        , longflag {longflag}
        , hasvalue {hasvalue}
    {}
    flag(char shortflag, bool hasvalue = false) noexcept
        : shortflag {shortflag}
        , hasvalue {hasvalue}
    {}
    flag(const char* longflag, bool hasvalue = false) noexcept
        : longflag {longflag}
        , hasvalue { hasvalue}
    {}
private:
    friend opts;

    char shortflag = 0;
    std::string_view longflag;
    bool hasvalue = false;
    bool present = false;
    std::string_view value;
};

#define FINDFLAG(field) std::find_if(flags.begin(), flags.end(), [&](auto x) { return x.field; })

class opts {
public:
    void register_flag(flag&& flag) {
        flags.push_back(flag);
    }

    bool present(char arg) const noexcept {
        auto pos = FINDFLAG(shortflag == arg);
        return pos != flags.end() && pos->present;
    }

    bool present(const char* arg) const noexcept {
        auto pos = FINDFLAG(longflag == arg);
        return pos != flags.end() && pos->present;
    }

    std::string_view value(char arg, const char* alt = "") const noexcept {
        auto pos = FINDFLAG(shortflag == arg);
        return pos != flags.end() ? pos->value : alt;
    }

    std::string_view value(const char* arg, const char* alt = "") const noexcept {
        auto pos = FINDFLAG(longflag == arg);
        return pos != flags.end() ? pos->value : alt;
    }

    const std::string_view& pos(int i) const noexcept { return positional[i]; }
    int pos_count() const noexcept { return positional.size(); }

    int parse(int argc, const char* const* argv) noexcept {
        for (int i = 1; i < argc;) {
            const char* arg = argv[i];
            if (arg[0] == '-') {
                int n = 0;
                if (arg[1] == '-') {
                    n = parse_longarg(argc, argv, i);
                } else {
                    n = parse_shortarg(argc, argv, i);
                }
                if (n < 0) {
                    return -1;
                }
                i += n;
               continue;
            }
            positional.push_back({arg});
            ++i;
        }
        return 0;
    }
private:
    int parse_longarg(int argc, const char* const* argv, int i) noexcept {
        int n = 1;
        
        const char* arg = argv[i] + 2;

        int eq;
        for (eq = 0; arg[eq] && arg[eq] != '='; ++eq);
        if (arg[eq] == 0) eq = -1;

        std::string_view argname = 
            eq > 0 ? std::string_view {arg, (size_t) eq} : arg;
        if (argname.size() == 0) {
            return n;
        }

        auto flag = FINDFLAG(longflag == argname);

        if (flag == flags.end()) {
            fprintf(stderr, "invalid flag \"%.*s\"\n", (int) argname.size(), argname.data());
            return -1;
        }

        flag->present = true;
        if (flag->hasvalue) {
            if (eq > 0) {
                flag->value = {arg + eq + 1};
            } else if (i + 1 < argc) {
                flag->value = argv[i + 1];
                ++n;
            }
        }

        return n;
    }

    int parse_shortarg(int argc, const char* const* argv, int i) noexcept {
        int n = 1;

        const char* arg = argv[i] + 1;
        for (const char* pt = arg; *pt; ++pt) {
            auto flag = std::find_if(flags.begin(), flags.end(), [&](auto x) { return x.shortflag == *pt; });
            if (flag != flags.end()) {
                flag->present = true;
                if (flag->hasvalue) {
                    if (pt[1] == 0 && i + 1 < argc) {
                        flag->value = argv[i + 1];
                        ++n;
                    } else {
                        flag->value = pt + 1;
                    }
                    return n;
                }
            } else {
                fprintf(stderr, "invalid shortflag \"%c\"\n", *pt);
                return -1;
            }
        }
        return n;
    }

    std::vector<flag> flags;
    std::vector<std::string_view> positional;
};
#undef FINDFLAG

} // namespace uopt
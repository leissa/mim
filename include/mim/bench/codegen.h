#pragma once

#include <cstddef>

#include <mim/def.h>
#include <mim/driver.h>
#include <mim/world.h>

#include <mim/plug/regex/regex.h>

namespace mim::bench {
class MimRegex;

// MimChar represents a single character literal in MimIR.
// It should be constructed via MimirCodeGen::char_lit.
// In case one needs to construct an invalid MimChar, use MimChar{nullptr}.
//
// You can check if a MimChar is valid via the bool operator, thus like this:
// ```
// MimChar c = ...;
// if (c) { ... } // valid
// ```
// You can print a MimChar to an output stream, e.g. std::cout << c << "\n";
//
class MimChar {
    friend class MimirCodeGen;

    explicit MimChar(const mim::Def* c)
        : c_(c) {}

    operator const mim::Def*() const { return c_; }

    const mim::Def* c_;

public:
    // Construct an invalid MimChar.
    MimChar(std::nullptr_t)
        : c_(nullptr) {}

    // Default move and copy constructors and assignment operators.
    // Thus you can copy and move MimChar instances.
    MimChar(MimChar&&)      = default;
    MimChar(const MimChar&) = default;

    MimChar& operator=(MimChar&&)      = default;
    MimChar& operator=(const MimChar&) = default;

    // Check if the MimChar is valid.
    operator bool() const { return c_ != nullptr; }
    operator MimRegex() const;

    // Print the MimChar to an output stream.
    friend std::ostream& operator<<(std::ostream& os, const MimChar& regex) {
        if (regex.c_)
            return regex.c_->stream(os, 0);
        else
            return os << "null";
    }
};

// MimRegex represents a regular expression in MimIR.
// It should be constructed via MimirCodeGen methods.
// In case one needs to construct an invalid MimRegex, use MimRegex{nullptr}.
//
// You can check if a MimRegex is valid via the bool operator, thus like this:
// ```
// MimRegex r = ...;
// if (r) { ... } // valid
// ```
// You can print a MimRegex to an output stream, e.g. std::cout << r << "\n";
//
class MimRegex {
    friend class MimirCodeGen;
    friend class MimChar;

    explicit MimRegex(const mim::Def* re)
        : re_(re) {}

    operator const mim::Def*() const { return re_; }

    const mim::Def* re_;

public:
    // Construct an invalid MimRegex.
    MimRegex(std::nullptr_t)
        : re_(nullptr) {}

    // Default move and copy constructors and assignment operators.
    // Thus you can copy and move MimRegex instances.
    MimRegex(MimRegex&&)      = default;
    MimRegex(const MimRegex&) = default;

    MimRegex& operator=(MimRegex&&)      = default;
    MimRegex& operator=(const MimRegex&) = default;

    // Check if the MimRegex is valid.
    operator bool() const { return re_ != nullptr; }

    // Print the MimRegex to an output stream.
    friend std::ostream& operator<<(std::ostream& os, const MimRegex& regex) {
        if (regex.re_)
            return regex.re_->stream(os, 0);
        else
            return os << "null";
    }
};

// Character classes for regex character classes, e.g. \d, \D, \w, \W, \s, \S
using cls = mim::plug::regex::cls;

// MimirCodeGen is a helper class to generate MimIR for regular expressions.
// It hides all the MimIR details and provides a simple interface to construct regexes.
//
// You can use it like this:
// ```
// MimirCodeGen codegen;
// MimChar c = codegen.char_lit('a'); // character literal 'a'
// MimRegex r = codegen.regex_lit(c); // regex matching 'a'
// r = codegen.regex_star(r);         // regex matching 'a'*
// auto matcher = codegen.make_matcher(r); // compile the regex to a matcher function
// bool matches = matcher("aa");      // use the matcher function
// ```
class MimirCodeGen {
    using LibHandle = std::unique_ptr<void, void (*)(void*)>;

    static constexpr const char* MATCHER_FUNC_NAME = "mim_match_regex";

public:
    // Construct the MimirCodeGen and its internal MimIR world.
    // The world is where all MimIR definitions are created.
    // The MimirCodeGen instance must outlive all MimIR definitions created by it.
    explicit MimirCodeGen();

    MimirCodeGen(const MimirCodeGen&)            = delete;
    MimirCodeGen& operator=(const MimirCodeGen&) = delete;

    using LogLevel = mim::Log::Level;

    // Set the logging level for the internal MimIR world.
    // By default, the logging level is LogLevel::Error.
    // When setting to LogLevel::Debug, a lot of output is generated.
    // This also includes dot files for the generated RegEx automata.
    // You can search for them, by searching for "digraph" in the output.
    void set_log_level(LogLevel level);

    /// MimIR construction wrappers

    // Create a MimChar representing the given character literal.
    // The returned MimChar is valid.
    MimChar char_lit(char c);

    // Create a MimRegex that matches the given character literal.
    MimRegex regex_lit(char c);
    // Create a MimRegex that matches the given MimChar.
    MimRegex regex_lit(MimChar c);

    // Create a MimRegex that is the concatenation of the given regexes.
    // I.e. the regex `ab` means that `a` is followed by `b`.
    // This is translated to:
    // ```
    // regex_conj({regex_lit('a'), regex_lit('b')});
    // ```
    MimRegex regex_conj(const std::vector<MimRegex>& exprs);
    MimRegex regex_conj(std::vector<MimRegex>&& exprs);

    // Create a MimRegex that is the disjunction (alternation) of the given regexes.
    // I.e. the regex `[ab]` means that either `a` or `b` matches.
    // This is translated to:
    // ```
    // regex_disj({regex_lit('a'), regex_lit('b')});
    // ```
    MimRegex regex_disj(const std::vector<MimRegex>& exprs);
    MimRegex regex_disj(std::vector<MimRegex>&& exprs);

    // Create a MimRegex that matches a single character from the given range (inclusive).
    // I.e. the regex `[a-z]` means that any character from `a` to `z` matches.
    // This is translated to:
    // ```
    // regex_range(char_lit('a'), char_lit('z'));
    // ```
    MimRegex regex_range(MimChar left, MimChar right);

    // Create a MimRegex that matches any single character (dot `.` in regex).
    MimRegex regex_any();

    // Create a MimRegex that matches the empty string (epsilon).
    MimRegex regex_empty();

    // Create a MimRegex that matches zero or more repetitions of the given regex.
    // I.e. the regex `a*` means that `a` can appear zero or more times.
    // This is translated to:
    // ```
    // regex_star(regex_lit('a'));
    // ```
    MimRegex regex_star(MimRegex expr);

    // Create a MimRegex that matches one or more repetitions of the given regex.
    // I.e. the regex `a+` means that `a` appears at least once.
    // This is translated to:
    // ```
    // regex_plus(regex_lit('a'));
    // ```
    MimRegex regex_plus(MimRegex expr);

    // Create a MimRegex that matches zero or one occurrence of the given regex.
    // I.e. the regex `a?` means that `a` can appear zero or one time.
    // This is translated to:
    // ```
    // regex_optional(regex_lit('a'));
    // ```
    MimRegex regex_optional(MimRegex expr);

    // Create a MimRegex that matches if the given regex does NOT match.
    // I.e. the regex `[^a]` means that any character except `a` matches.
    // This is translated to:
    // ```
    // regex_not(regex_lit('a'));
    // ```
    MimRegex regex_not(MimRegex expr);

    // Create a MimRegex that matches any single character from the given character class.
    // I.e. the regex `\d` means that any digit character matches.
    // This is translated to:
    // ```
    // regex_class(cls::d);
    // ```
    MimRegex regex_class(cls c);

    /// MimIR construction wrappers end

    // Compile the given MimRegex into a matcher function.
    // The returned function takes a const char* (C-string) as input and returns true
    // if the input matches the regex, false otherwise.
    // The returned function is valid as long as the MimirCodeGen instance is alive.
    // It throws std::runtime_error if compilation fails.
    std::function<bool(const char*)> make_matcher(MimRegex re);

private:
    static mim::DefVec to_defvec(const std::vector<MimRegex>& exprs);

    void mim_match(const mim::Def* re);

    int compile_to_shared(std::string out);

    mim::Driver driver_;
    mim::World& world_;

    LibHandle jit_lib_;
};

} // namespace mim::bench

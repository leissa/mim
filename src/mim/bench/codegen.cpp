#include "mim/bench/codegen.h"

#include <sstream>
#include <stdexcept>
#include <thread>

#include <mim/ast/ast.h>
#include <mim/pass/optimize.h>
#include <mim/util/dl.h>
#include <mim/util/sys.h>

#include <mim/plug/core/core.h>
#include <mim/plug/mem/mem.h>

#ifdef _WIN32
#    define WEXITSTATUS
#endif

namespace mim::bench {

MimirCodeGen::MimirCodeGen()
    : driver_()
    , world_(driver_.world())
    , jit_lib_(nullptr, mim::dl::close) {
    world_.log().set(&std::cerr);
    mim::ast::load_plugins(world_, {"compile", "mem", "core", "opt", "regex", "direct"});
}

void MimirCodeGen::set_log_level(mim::Log::Level level) { world_.log().set(level); }

// clang-format off
/*
.con .extern match[mem: %mem.M, to_match: %mem.Ptr («⊤:.Nat; .Idx 256», 0), exit : .Cn [%mem.M, .Idx 2]] =
    .let (regex_mem, matched, pos) = re (mem, to_match, 0:(.Idx Top));
    .let last_elem_ptr = %mem.lea (Top, <Top; .Idx 256>, 0) (to_match, pos);
    .let (final_mem, last_elem) = %mem.load (regex_mem, last_elem_ptr);
    exit (final_mem, %core.bit2.and_ 0 (matched, %core.icmp.e (last_elem, 0:I8)));
*/
// clang-format on
void MimirCodeGen::mim_match(const mim::Def* re) {
    auto match = world_
                     .mut_con({world_.annex<mim::plug::mem::M>(),
                               world_.call<mim::plug::mem::Ptr0>(world_.arr(world_.top_nat(), world_.type_i8())),
                               world_.cn({world_.annex<mim::plug::mem::M>(), world_.type_bool()})})
                     ->set(MATCHER_FUNC_NAME);
    match->make_external();
    auto [mem, to_match, exit] = match->vars<3>();

    auto [regex_mem, matched, pos]
        = world_.implicit_app(re, {mem, to_match, world_.lit(world_.type_idx(world_.top_nat()), 0)})->projs<3>();
    auto last_elem_ptr          = world_.call<mim::plug::mem::lea>(mim::DefVec{to_match, pos});
    auto [final_mem, last_elem] = world_.call<mim::plug::mem::load>(mim::DefVec{regex_mem, last_elem_ptr})->projs<2>();
    auto eq_zero                = world_.call(mim::plug::core::icmp::e, mim::DefVec{last_elem, world_.lit_i8(0)});
    auto matched_and_end = world_.call(mim::plug::core::bit2::and_, world_.lit_nat_0(), mim::DefVec{matched, eq_zero});
    match->app(false, exit, {final_mem, matched_and_end});
}

int MimirCodeGen::compile_to_shared(std::string out) {
    auto ll = out + ".ll";
    {
        std::ofstream ofs(ll);
        driver_.backend("ll")(world_, ofs);
    }

#ifdef _WIN32
    std::string clang_extension = ".exe";
#else
    std::string clang_extension = "";
#endif
    auto cmd = mim::fmt("clang{} \"{}\" -o \"{}\" -Wno-override-module -shared", clang_extension, ll, out);
    int exit = std::system(cmd.c_str());
    return WEXITSTATUS(exit);
}

std::function<bool(const char*)> MimirCodeGen::make_matcher(MimRegex re) {
    mim_match(re);

    mim::optimize(world_);

    auto tmp = std::filesystem::temp_directory_path();

    auto shared_lib = mim::fmt("{}/regex-{}.{}", tmp.string(), std::this_thread::get_id(), mim::dl::extension);

    if (compile_to_shared(shared_lib) == 0)
        world_.DLOG("Compiled regex to shared library: {}", shared_lib);
    else
        throw std::runtime_error{"0: error: Failed to compile regex to shared library."};

    // dl::open throws on error
    jit_lib_.reset(mim::dl::open(shared_lib.c_str()));

    std::function<bool(const char*)> fn_handle = (bool (*)(const char*))mim::dl::get(jit_lib_.get(), MATCHER_FUNC_NAME);
    return fn_handle;
}

// MimIR construction wrappers

MimChar MimirCodeGen::char_lit(char c) { return MimChar{world_.lit_i8(c)}; }

MimRegex MimirCodeGen::regex_lit(char c) { return regex_lit(char_lit(c)); }
MimRegex MimirCodeGen::regex_lit(MimChar c) { return MimRegex{world_.call<mim::plug::regex::lit>(c.c_)}; }

MimRegex MimirCodeGen::regex_conj(const std::vector<MimRegex>& exprs) {
    return MimRegex{world_.call<mim::plug::regex::conj>(to_defvec(exprs))};
}
MimRegex MimirCodeGen::regex_conj(std::vector<MimRegex>&& exprs) { return regex_conj(exprs); }

MimRegex MimirCodeGen::regex_disj(const std::vector<MimRegex>& exprs) {
    return MimRegex{world_.call<mim::plug::regex::disj>(to_defvec(exprs))};
}
MimRegex MimirCodeGen::regex_disj(std::vector<MimRegex>&& exprs) { return regex_disj(exprs); }

MimRegex MimirCodeGen::regex_any() { return MimRegex{world_.annex<mim::plug::regex::any>()}; }

MimRegex MimirCodeGen::regex_empty() { return MimRegex{world_.annex<mim::plug::regex::empty>()}; }

MimRegex MimirCodeGen::regex_star(MimRegex expr) {
    return MimRegex{world_.call(mim::plug::regex::quant::star, expr.re_)};
}

MimRegex MimirCodeGen::regex_plus(MimRegex expr) {
    return MimRegex{world_.call(mim::plug::regex::quant::plus, expr.re_)};
}

MimRegex MimirCodeGen::regex_optional(MimRegex expr) {
    return MimRegex{world_.call(mim::plug::regex::quant::optional, expr.re_)};
}

MimRegex MimirCodeGen::regex_not(MimRegex expr) { return MimRegex{world_.call<mim::plug::regex::not_>(expr.re_)}; }

MimRegex MimirCodeGen::regex_class(cls c) { return MimRegex{world_.annex(c)}; }

MimRegex MimirCodeGen::regex_range(MimChar left, MimChar right) {
    return MimRegex{world_.call<mim::plug::regex::range>(mim::DefVec{left.c_, right.c_})};
}

mim::DefVec MimirCodeGen::to_defvec(const std::vector<MimRegex>& exprs) {
    mim::DefVec exprs_def;
    std::transform(exprs.begin(), exprs.end(), std::back_inserter(exprs_def), [](const MimRegex& r) { return r.re_; });
    return exprs_def;
}

MimChar::operator MimRegex() const { return MimRegex{c_->world().call<mim::plug::regex::lit>(c_)}; }
} // namespace mim::bench

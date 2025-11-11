#include "mim/bench/codegen.h"

namespace mim::bench {

MimRegex build_hex(MimirCodeGen& cg, uint32_t i) {
    if (i <= 9) return cg.regex_lit('0' + i);
    return cg.regex_lit('a' + i - 10);
}

MimRegex build_num(MimirCodeGen& cg, uint32_t i) {
    std::vector<MimRegex> hexes;
    hexes.emplace_back(build_hex(cg, (i >> uint32_t( 0)) & uint32_t(0xf)));
    hexes.emplace_back(build_hex(cg, (i >> uint32_t( 8)) & uint32_t(0xf)));
    hexes.emplace_back(build_hex(cg, (i >> uint32_t(16)) & uint32_t(0xf)));
    hexes.emplace_back(build_hex(cg, (i >> uint32_t(24)) & uint32_t(0xf)));
    return cg.regex_conj(hexes);
}

void build_loop_cascade() {
    MimirCodeGen cg;

    std::vector<MimRegex> stars;
    for (uint32_t i = 0; i != 10'000; ++i)
        stars.emplace_back(cg.regex_star(build_num(cg, i)));

    auto exp = cg.regex_conj(stars);
    cg.make_matcher(exp);
}

} // namespace mim::bench

int main() { mim::bench::build_loop_cascade(); }

#include <sstream>

#include <mim/driver.h>
#include <mim/nest.h>
#include <mim/world.h>

#include <mim/ast/parser.h>

#include <mim/plug/core/core.h>

static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

std::ofstream fvs_cascade;
std::ofstream nest_cascade;
std::ofstream beta_cascade;

std::ofstream fvs_nest;
std::ofstream nest_nest;
std::ofstream beta_nest;

namespace mim::bench {
using namespace mim::plug;

std::pair<Lam*, const Def*> build_loop(World& w, Lam* prev, const Def* in) {
    auto I64  = w.type_i64();
    auto zero = w.lit_i64(0);
    auto one  = w.lit_i64(1);

    auto head = w.mut_con(I64);
    auto body = w.mut_con(Defs{});
    auto exit = w.mut_con(Defs{});

    prev->app(false, head, zero);

    auto phi  = head->var();
    auto cond = w.call(core::icmp::ul, Defs{phi, in});
    head->branch(false, cond, body, exit);

    auto add = w.call(core::wrap::add, 0_n, Defs{phi, one});
    body->app(false, head, add);

    return {exit, phi};
}

void cascade(int n) {
    Driver driver;
    auto& w = driver.world();
    ast::load_plugins(w, "core");

    auto I64       = w.type_i64();
    auto top       = w.mut_fun(I64, I64);
    auto prev      = top;
    auto [in, ret] = prev->vars<2>();

    for (int i = 0; i != n; ++i)
        std::tie(prev, in) = build_loop(w, prev, in);

    prev->app(false, ret, in);

    {
        auto t1 = rdtsc();
        top->free_vars();
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        fvs_cascade << n << " " << cycles << std::endl;
    }

    {
        auto t1     = rdtsc();
        auto nest   = Nest(top);
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        nest_cascade << n << " " << cycles << std::endl;
    }

    {
        auto dummy = w.axm(top->type()->dom());
        auto t1     = rdtsc();
        auto _      = top->reduce(dummy);
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        beta_cascade << n << " " << cycles << std::endl;
    }
}

std::pair<Lam*, const Def*> build_nest(World& w, Lam* prev, const Def* in) {
    auto I64  = w.type_i64();
    auto zero = w.lit_i64(0);
    auto one  = w.lit_i64(1);

    auto head = w.mut_con(I64);
    auto body = w.mut_con(Defs{});
    auto exit = w.mut_con(Defs{});

    prev->app(false, head, zero);

    auto phi  = head->var();
    auto cond = w.call(core::icmp::ul, Defs{phi, in});
    head->branch(false, cond, body, exit);

    auto add = w.call(core::wrap::add, 0_n, Defs{phi, one});
    body->app(false, head, add);

    return {exit, phi};
}

} // namespace mim::bench

int main() {
    fvs_cascade.open("fvs_cascade.txt");
    nest_cascade.open("nest_cascade.txt");
    beta_cascade.open("beta_cascade.txt");

    fvs_nest.open("fvs_nest.txt");
    nest_nest.open("nest_nest.txt");
    beta_nest.open("beta_nest.txt");

    fvs_cascade << "% n cycles" << std::endl;
    nest_cascade << "% n cycles" << std::endl;
    beta_cascade << "% n cycles" << std::endl;

    fvs_nest << "% n cycles" << std::endl;
    nest_nest << "% n cycles" << std::endl;
    beta_nest << "% n cycles" << std::endl;

    for (int i = 1; i <= (1 << 20); i <<= 1)
        mim::bench::cascade(i);
}

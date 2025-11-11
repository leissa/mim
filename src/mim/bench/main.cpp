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

std::ofstream fvs__cascade0;
std::ofstream nest_cascade0;
std::ofstream beta_cascade0;

std::ofstream fvs__cascade1;
std::ofstream nest_cascade1;
std::ofstream beta_cascade1;

std::ofstream fvs__nest;
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

void cascade(int n, bool combine) {
    std::cout << n << std::endl;
    Driver driver;
    auto& w = driver.world();
    ast::load_plugins(w, "core");

    auto ti64      = w.type_i64();
    auto top       = w.mut_fun(ti64, ti64);
    auto prev      = top;
    auto [in, ret] = prev->vars<2>();
    auto res       = in;

    for (int i = 0; i != n; ++i) {
        std::tie(prev, in) = build_loop(w, prev, in);
        res                = combine ? w.call(core::wrap::add, 0_n, Defs{res, in}) : in;
    }

    prev->app(false, ret, in);

    auto& fvs__cascade = combine ? fvs__cascade1 : fvs__cascade0;
    auto& nest_cascade = combine ? nest_cascade1 : nest_cascade0;
    auto& beta_cascade = combine ? beta_cascade1 : beta_cascade0;

    // prev->invalidate();

    {
        // validate that all free var caches are empty
        unique_queue<LamSet> q;
        q.push(top);

        while (!q.empty()) {
            auto mut = q.pop();
            if (!mut->is_set()) continue;
            if (!mut->is_cache_empty()) std::cout << "oh no!" << std::endl;

            for (auto op : mut->ops()) {
                for (auto local_mut : op->local_muts())
                    if (auto lam = local_mut->isa<Lam>()) q.push(lam);
            }
        }
    }

    // std::cout << "free_vars" << std::endl;
    {
        auto t1 = rdtsc();
        top->free_vars();
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        fvs__cascade << n << " " << cycles << std::endl;
    }

    // std::cout << "nest" << std::endl;
    {
        auto t1     = rdtsc();
        auto nest   = Nest(top);
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        nest_cascade << n << " " << cycles << std::endl;
    }

    // std::cout << "beta" << std::endl;
    {
        auto dummy  = w.axm(top->type()->dom());
        auto t1     = rdtsc();
        auto _      = top->reduce(dummy);
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        beta_cascade << n << " " << cycles << std::endl;
    }
    // std::cout << "done" << std::endl;
}

std::pair<Lam*, const Def*> build_nest(int i, World& w, Lam* prev, const Def* in) {
    auto ti64 = w.type_i64();
    auto zero = w.lit_i64(0);
    auto one  = w.lit_i64(1);

    auto head = w.mut_con(ti64);
    auto body = w.mut_con(Defs{});
    auto exit = w.mut_con(Defs{});

    prev->app(false, head, zero);

    auto phi  = head->var();
    auto cond = w.call(core::icmp::ul, Defs{phi, in});
    head->branch(false, cond, body, exit);

    auto add = w.call(core::wrap::add, 0_n, Defs{phi, one});
    // auto sadf = body->app(false, head, add);
    // auto [x, y] = build_nest(i - 1, w, x, y);

    return {exit, phi};
}

} // namespace mim::bench

int main() {
    fvs__cascade0.open("fvs__cascade0.data");
    nest_cascade0.open("nest_cascade0.data");
    beta_cascade0.open("beta_cascade0.data");

    fvs__cascade1.open("fvs__cascade1.data");
    nest_cascade1.open("nest_cascade1.data");
    beta_cascade1.open("beta_cascade1.data");

    fvs__nest.open("fvs__nest.data");
    nest_nest.open("nest_nest.data");
    beta_nest.open("beta_nest.data");

    fvs__cascade0 << "% n cycles" << std::endl;
    nest_cascade0 << "% n cycles" << std::endl;
    beta_cascade0 << "% n cycles" << std::endl;

    fvs__cascade1 << "% n cycles" << std::endl;
    nest_cascade1 << "% n cycles" << std::endl;
    beta_cascade1 << "% n cycles" << std::endl;

    fvs__nest << "% n cycles" << std::endl;
    nest_nest << "% n cycles" << std::endl;
    beta_nest << "% n cycles" << std::endl;

    for (int i = 1; i <= (1 << 20); i <<= 1)
        mim::bench::cascade(i, false);

    for (int i = 1; i <= (1 << 19); i <<= 1)
        mim::bench::cascade(i, true);
}

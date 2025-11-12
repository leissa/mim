#include <cstdlib>

#include <sstream>

#include <mim/driver.h>
#include <mim/nest.h>
#include <mim/world.h>

#include <mim/ast/parser.h>

#include <mim/plug/core/core.h>

using namespace std::string_literals;

static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

enum {
    File_FVs,
    File_Nest,
    File_Beta,
    File_Num,
};

namespace mim::bench {
using namespace mim::plug;

#if 0
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
#endif

void do_bench(std::ofstream* os, int n, World& w, Lam* top) {
    std::cout << "n/world size: " << n << "/" << w.size() << " = " << float(w.size())/float(n) << std::endl;
    std::cout << "free vars" << std::endl;
    {
        auto t1 = rdtsc();
        top->free_vars();
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        os[File_FVs] << n << " " << cycles << std::endl;
    }

    std::cout << "nest" << std::endl;
    {
        auto t1     = rdtsc();
        auto nest   = Nest(top);
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        os[File_Nest] << n << " " << cycles << std::endl;
    }

    std::cout << "beta" << std::endl;
    {
        auto dummy  = w.axm(top->type()->dom());
        auto t1     = rdtsc();
        auto _      = top->reduce(dummy);
        auto t2     = rdtsc();
        auto cycles = t2 - t1;
        os[File_Beta] << n << " " << cycles << std::endl;
    }
    std::cout << "done" << std::endl;
}

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

void cascade(std::ofstream* os, int n, bool combine) {
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

    do_bench(os, n, w, top);
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
    if (i == 0) {
        body->app(false, head, add);
    } else {
        auto [next, _] = build_nest(i - 1, w, body, phi);
        next->app(false, head, add);
    }

    return {exit, phi};
}

void loop_nest(std::ofstream* os, int n) {
    Driver driver;
    auto& w = driver.world();
    ast::load_plugins(w, "core");

    auto ti64      = w.type_i64();
    auto top       = w.mut_fun(ti64, ti64);
    auto prev      = top;
    auto [in, ret] = prev->vars<2>();

    auto [exit, phi] = build_nest(n, w, top, in);
    exit->app(false, ret, phi);

    do_bench(os, n, w, top);
}

} // namespace mim::bench

int main(int argc, const char** argv) {
    std::ofstream ofs[File_Num];
    auto names = std::array<std::string, File_Num>{"fvs"s, "nest"s, "beta"s};

    if (argc != 2 && argc != 3) {
        std::cerr << "usage: " << argv[0] << " 0|1|2 [suffix]" << std::endl;
        return EXIT_FAILURE;
    }

    char test;
    if (strcmp(argv[1], "0") == 0) {
        test = '0';
    } else if (strcmp(argv[1], "1") == 0) {
        test = '1';
    } else if (strcmp(argv[1], "2") == 0) {
        test = '2';
    } else {
        std::cerr << "usage: " << argv[0] << " 0|1|2" << std::endl;
        return EXIT_FAILURE;
    }

    for (int i = 0; i != File_Num; ++i) {
        auto& name = names[i];
#ifdef MIM_IMMER
        name += ".immer";
#endif
        name += "."s + test + ".data";

        if (argc == 3) name += "."s + argv[2]; // suffix

        ofs[i].open(name);
        ofs[i] << "% n cycles" << std::endl;
    }

    for (int i = 1; i <= (1 << 20); i <<= 1) {
        switch (test) {
            case '0': mim::bench::cascade(ofs, i, false); break;
            case '1': mim::bench::cascade(ofs, i, true); break;
            case '2': mim::bench::loop_nest(ofs, i); break;
            default: std::cerr << "unknown test" << std::endl; return EXIT_FAILURE;
        }
    }
}

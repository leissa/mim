#include <chrono>
#include <cstdlib>

#include <mim/driver.h>
#include <mim/nest.h>
#include <mim/world.h>

#include <mim/ast/parser.h>

#include <mim/plug/core/core.h>

using namespace std::string_literals;

enum {
    File_FVs,
    File_Nest,
    File_Beta,
    File_Num,
};

namespace mim::bench {
using namespace mim::plug;

void do_bench(std::ofstream* os, int test, int n, World& w, Lam* top) {
    std::cout << "n/world size: " << n << "/" << w.size() << " = " << float(w.size()) / float(n) << std::endl;
    std::cout << "free vars" << std::endl;
    {
        auto t1 = std::chrono::steady_clock::now();
        top->free_vars();
        auto t2 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        os[File_FVs] << n << " " << ms << std::endl;
    }

    std::cout << "nest" << std::endl;
    {
        auto t1   = std::chrono::steady_clock::now();
        auto nest = Nest(top);
        auto t2   = std::chrono::steady_clock::now();
        auto ms   = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        os[File_Nest] << n << " " << ms << std::endl;
    }

    std::cout << "beta" << std::endl;
    {
        auto dummy = w.axm(top->type()->dom());
        auto t1    = std::chrono::steady_clock::now();
        auto _     = top->reduce(dummy);
        auto t2    = std::chrono::steady_clock::now();
        auto ms    = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        os[File_Beta] << n << " " << ms << std::endl;
    }

    {
        auto ll = std::to_string(test) + "."s + std::to_string(n) + ".ll"s;
        auto path = fs::path{ll};
        if (fs::exists(path))
            std::cout << ll << " already exists" << std::endl;
        else {
            std::cout << "emit " << ll << std::endl;
            auto of = std::ofstream(ll);
            w.driver().backend("ll")(w, of);
        }
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
    ast::load_plugins(w, View<std::string>{"compile", "core"});

    auto ti64      = w.type_i64();
    auto top       = w.mut_fun(ti64, ti64)->set("top");
    auto prev      = top;
    auto [in, ret] = prev->vars<2>();
    auto res       = in;

    top->make_external();

    for (int i = 0; i != n; ++i) {
        std::tie(prev, in) = build_loop(w, prev, in);
        res                = combine ? w.call(core::wrap::add, 0_n, Defs{res, in}) : in;
    }

    prev->app(false, ret, in);

    do_bench(os, combine ? 1 : 0, n, w, top);
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
    ast::load_plugins(w, View<std::string>{"compile", "core"});

    auto ti64        = w.type_i64();
    auto top         = w.mut_fun(ti64, ti64)->set("top");
    auto prev        = top;
    auto [in, ret]   = prev->vars<2>();
    auto [exit, phi] = build_nest(n, w, top, in);

    exit->app(false, ret, phi);
    top->make_external();

    do_bench(os, 2, n, w, top);
}

} // namespace mim::bench

int main(int argc, const char** argv) {
    std::ofstream ofs[File_Num];
    auto algos = std::array<std::string, File_Num>{"fvs."s, "nest."s, "beta."s};

    auto usage = [argv] { std::cerr << "usage: " << argv[0] << " 0|1|2 [suffix]" << std::endl; };

    if (argc != 2 && argc != 3) {
        usage();
        return EXIT_FAILURE;
    }

    char row;
    if (false) {}
    else if (strcmp(argv[1], "0") == 0) row = '0';
    else if (strcmp(argv[1], "1") == 0) row = '1';
    else if (strcmp(argv[1], "2") == 0) row = '2';
    else {
        usage();
        return EXIT_FAILURE;
    }

    std::string suffix;
    if (argc == 3) suffix = argv[2];

    for (int i = 0; i != File_Num; ++i) {
#ifdef MIM_IMMER
        auto name = "immer."s;
#elif defined(MIM_STD_SET)
        auto name = "set."s;
#else
        auto name = "trie."s;
#endif
        name += algos[i];
        name += row + "."s;
        name += suffix;

        ofs[i].open(name);
        ofs[i] << "% n ms" << std::endl;
    }

    for (int i = 1; i <= (1 << 6); i <<= 1) {
        switch (row) {
            case '0': mim::bench::cascade(ofs, i, false); break;
            case '1': mim::bench::cascade(ofs, i, true); break;
            case '2': mim::bench::loop_nest(ofs, i); break;
            default: std::cerr << "unknown test" << std::endl; return EXIT_FAILURE;
        }
    }
}

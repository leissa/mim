#include <chrono>
#include <cstdlib>

#include <format>

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

static constexpr auto Set =
#ifdef MIM_IMMER
    "immer";
#elif defined(MIM_STD_SET)
    "set";
#else
    "trie";
#endif

namespace mim::bench {
using namespace mim::plug;

void do_bench(std::ofstream* os, int row, int n, World& w, Lam* top) {
    std::cout << std::format("set: {}, row: {}, n: {}", Set, row, n) << std::endl;
    std::cout << "* free vars" << std::endl;
    {
        auto t1 = std::chrono::steady_clock::now();
        top->free_vars();
        auto t2 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        os[File_FVs] << n << " " << ms << std::endl;
    }

    std::cout << "* nest" << std::endl;
    {
        auto t1   = std::chrono::steady_clock::now();
        auto nest = Nest(top);
        auto t2   = std::chrono::steady_clock::now();
        auto ms   = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        os[File_Nest] << n << " " << ms << std::endl;
    }

    std::cout << "* beta" << std::endl;
    {
        auto dummy = w.axm(top->type()->dom());
        auto t1    = std::chrono::steady_clock::now();
        auto _     = top->reduce(dummy);
        auto t2    = std::chrono::steady_clock::now();
        auto ms    = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        os[File_Beta] << n << " " << ms << std::endl;
    }

    {
        auto ll   = std::to_string(row) + "."s + std::to_string(n) + ".ll"s;
        auto path = fs::path{ll};
        if (fs::exists(path))
            std::cout << "* " << ll << " already exists" << std::endl;
        else {
            std::cout << "* emit " << ll << std::endl;
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
    auto plz       = w.mut_fun(ti64, ti64)->set("plzinline");
    auto prev      = plz;
    auto [in, ret] = prev->vars<2>();
    auto res       = in;
    auto top       = w.mut_fun(ti64, ti64)->set("top");
    auto eta       = w.mut_con(ti64);

    top->externalize();
    top->app(false, plz, {top->var(2, 0), eta});
    eta->app(false, top->var(2, 1), eta->var());

    for (int i = 0; i != n; ++i) {
        std::tie(prev, in) = build_loop(w, prev, in);
        res                = combine ? w.call(core::wrap::add, 0_n, Defs{res, in}) : in;
    }

    prev->app(false, ret, in);

    do_bench(os, combine ? 1 : 0, n, w, plz);
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
    auto plz         = w.mut_fun(ti64, ti64)->set("plzinline");
    auto prev        = plz;
    auto [in, ret]   = prev->vars<2>();
    auto [exit, phi] = build_nest(n, w, plz, in);
    auto top         = w.mut_fun(ti64, ti64)->set("top");
    auto eta         = w.mut_con(ti64);

    exit->app(false, ret, phi);
    top->externalize();
    top->app(false, plz, {top->var(2, 0), eta});
    eta->app(false, top->var(2, 1), eta->var());

    do_bench(os, 2, n, w, plz);
}

} // namespace mim::bench

int main(int argc, const char** argv) {
    std::ofstream ofs[File_Num];
    std::string names[File_Num];
    auto algos = std::array<std::string, File_Num>{"fvs."s, "nest."s, "beta."s};
    auto usage = [argv] { std::cerr << "usage: " << argv[0] << " <iter> 0|1|2 [suffix]" << std::endl; };

    if (argc != 3 && argc != 4) {
        usage();
        return EXIT_FAILURE;
    }

    int iter = std::stoi(argv[1]);
    std::cout << iter << std::endl;

    char row;
    if (false) {
    } else if (strcmp(argv[2], "0") == 0)
        row = '0';
    else if (strcmp(argv[2], "1") == 0)
        row = '1';
    else if (strcmp(argv[2], "2") == 0)
        row = '2';
    else {
        usage();
        return EXIT_FAILURE;
    }

    std::string suffix;
    if (argc >= 4) suffix = argv[3];

    for (int i = 0; i != File_Num; ++i) {
        std::string name = Set;
        name += '.';
        name += algos[i];
        name += row + "."s;
        name += suffix;
        names[i] = name;

        ofs[i].open(name);
        ofs[i] << "% n ms" << std::endl;
    }

    for (int i = 1; i <= (1 << iter); i <<= 1) {
        switch (row) {
            case '0': mim::bench::cascade(ofs, i, false); break;
            case '1': mim::bench::cascade(ofs, i, true); break;
            case '2': mim::bench::loop_nest(ofs, i); break;
            default: std::cerr << "unknown test" << std::endl; return EXIT_FAILURE;
        }
    }
}

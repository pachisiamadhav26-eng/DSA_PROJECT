// Reproducible adversarial inputs for the team's INTERFACE.md replay protocol.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using U = std::uint64_t;
using S = std::int64_t;
struct Writer {
    template<class... T> void line(const T&... values) {
        bool first = true;
        ((std::cout << (first ? (first = false, "") : " ") << values), ...);
        std::cout << '\n';
    }
    void add(U id, S score) { line("ADD", 0, id, score); }
    void page(U board, U n) {
        line("SIZE", board);
        if (n) { line("SELECT", board, 1); line("SELECT", board, n);
            line("RANGE", board, 1, std::min<U>(n, 12)); }
    }
};
U number(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("expected unsigned decimal");
    return std::stoull(text);
}
void generate(const std::string& name, U n, U ops, U seed, Writer& w) {
    if (name == "rotations") {
        U board = 0, next = 1, id = 1;
        for (const auto& scores : std::vector<std::vector<S>>{
                {30,20,10}, {10,20,30}, {30,10,20}, {10,30,20}}) {
            U start = id;
            for (S s : scores) w.line("ADD", board, id++, s);
            w.page(board, 3);
            for (U p = start; p < id; ++p) {
                w.line("REMOVE", board, p); w.line("SIZE", board);
            }
            w.line("SPLIT", board, 0, next, next+1);
            w.line("MERGE", next, next+1, next+2);
            board = next+2; next += 3;
        }
    } else if (name == "ascending" || name == "descending" || name == "ties") {
        // Descending IDs into one tie bucket force vector insertion at its front.
        for (U i = 1; i <= n; ++i)
            w.add(name == "ties" ? n+1-i : i,
                  name == "ties" ? 90 : name == "ascending" ? S(i) : -S(i));
        w.page(0, n);
        for (U i = 1; i <= std::min<U>(n,12); ++i) w.line("RANK", 0, i);
        w.line("SET", 0, 1, 91); w.line("RANK", 0, 1);
        w.line("SET", 0, 1, 90); w.page(0,n);
    } else if (name == "churn") {
        for (U i=1; i<=n; ++i) w.add(i,S(i%7));
        for (U i=1; i<=n; ++i) {
            w.line("REMOVE",0,i); w.line("ADD",0,i,-S(i));
            w.line("SET",0,i,-S(i)); w.line("RANK",0,i);
        }
        for (U i=n; i>0; --i) { w.line("REMOVE",0,i); w.line("SIZE",0); }
        w.line("SELECT",0,1);
    } else if (name == "extremes") {
        w.add(0,std::numeric_limits<S>::min());
        w.add(2147483647,std::numeric_limits<S>::max()); w.add(3,0); w.add(2,0);
        w.page(0,4); w.line("RANK",0,3);
        w.line("SET",0,0,std::numeric_limits<S>::max()); w.page(0,4);
        w.line("SPLIT",0,std::numeric_limits<S>::min(),1,2);
        w.line("SIZE",1); w.line("SIZE",2); w.line("MERGE",1,2,3); w.page(3,4);
    } else if (name == "seasons" || name == "unequal") {
        for (U i=1; i<=n; ++i) w.add(i,S(i));
        S threshold = name == "unequal" ? S(n) : S(n/2+1);
        w.line("SPLIT",0,threshold,1,2);
        w.page(1,n-U(threshold)+1); w.page(2,U(threshold)-1);
        w.line("SIZE",0); // consumed source must be rejected
        if (name == "seasons") {
            // Move a low-division player above the high division: ranges overlap.
            w.line("SET",2,1,S(n)+1); w.line("RANK",2,1);
        }
        w.line("MERGE",1,2,3); w.page(3,n);
        w.line("SIZE",1); w.line("SIZE",2);
        w.line("SPLIT",3,std::numeric_limits<S>::max(),4,5);
        w.line("SIZE",4); w.line("SIZE",5); w.line("MERGE",4,5,6); w.page(6,n);
        w.line("SPLIT",6,std::numeric_limits<S>::min(),7,8);
        w.line("SIZE",7); w.line("SIZE",8); w.line("MERGE",8,7,9); w.page(9,n);
    } else if (name == "errors") {
        w.line("SELECT",0,0); w.line("SELECT",0,1); w.line("RANGE",0,1,0);
        w.add(1,10); w.add(1,20); w.line("RANK",0,1);
        w.line("SET",0,2,20); w.line("REMOVE",0,2);
        w.line("SELECT",0,2); w.line("RANGE",0,0,1);
        w.line("MERGE",0,0,1); w.line("SIZE",999);
        w.line("SPLIT",0,10,1,2); w.line("RANK",2,1);
        w.line("ADD",2,1,30); w.line("MERGE",1,2,3); w.page(3,1);
        for (const auto& s : {"SELECT 3 -1", "ADD 3 4 9223372036854775808",
                "ADD 3 18446744073709551616 0", "SIZE 3 extra", "BOGUS 3",
                "ADD 3 +4 10", "ADD 3 4 --1"}) std::cout << s << '\n';
        w.page(3,1);
    } else if (name == "mixed") {
        // Slots bound the live population; RNG modulo decisions are portable.
        std::mt19937_64 rng(seed);
        std::vector<bool> live(n,false);
        std::vector<S> scores(n,0);
        for (U i=0; i<ops; ++i) {
            U slot=rng()%n, id=slot+1, kind=rng()%8;
            S s=S(rng()%17)-8;
            if (kind==0 || !live[slot]) {
                if (!live[slot]) { w.add(id,s); live[slot]=true; scores[slot]=s; }
                else w.line("RANK",0,id);
            } else if (kind<=2) {
                w.line("SET",0,id,kind==1 ? scores[slot] : s);
                if (kind==2) scores[slot]=s;
            } else if (kind==3) { w.line("REMOVE",0,id); live[slot]=false; }
            else if (kind<=5) w.line("RANK",0,id);
            else if (kind==6) w.line("SELECT",0,1);
            else w.line("SIZE",0);
        }
    } else if (name == "scale") {
        // Stream, rather than retain, one million players and score revisions.
        for (U i=1; i<=n; ++i) w.add(i,S(i));
        for (U i=0; i<ops; ++i) w.line("SET",0,i%n+1,S(n)+S(i)+1);
        w.page(0,n);
    } else throw std::invalid_argument("unknown case: "+name);
}
int main(int argc,char** argv) {
    try {
        std::string name="ties"; U n=64,ops=10000,seed=253;
        for (int i=1;i<argc;++i) {
            std::string arg=argv[i];
            if (arg=="--help") {
                std::cout << "generator --case rotations|ascending|descending|ties|churn|extremes|seasons|unequal|errors|mixed|scale --players 64 --ops 10000 --seed 253\n";
                return 0;
            }
            if (i+1==argc) throw std::invalid_argument("missing option value");
            std::string value=argv[++i];
            if (arg=="--case") name=value;
            else if (arg=="--players") n=number(value);
            else if (arg=="--ops") ops=number(value);
            else if (arg=="--seed") seed=number(value);
            else throw std::invalid_argument("unknown option: "+arg);
        }
        if (n<2 || n>1000000 || ops>10000000)
            throw std::invalid_argument("players must be 2..1000000; ops 0..10000000");
        Writer writer; generate(name,n,ops,seed,writer);
        std::cout.flush();
        if (!std::cout) throw std::runtime_error("output write failed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "generator: " << error.what() << '\n'; return 2;
    }
}

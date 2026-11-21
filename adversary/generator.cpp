// Reproducible adversarial inputs for the team's INTERFACE.md replay protocol.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using U = std::uint64_t;
using S = std::int64_t;
struct Writer {
    template<class First, class... Rest> void line(const First& first, const Rest&... rest) {
        std::cout << first;
        ((std::cout << ' ' << rest), ...);
        std::cout << '\n';
    }
    void add(U board, U id, S score) { line("ADD", board, id, score); }
    void page(U board, U n) {
        line("SIZE", board);
        if (n == 0) return;
        line("SELECT", board, 1);
        line("SELECT", board, n);
        line("RANGE", board, 1, std::min<U>(n, 12));
    }
};
U number(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("expected unsigned decimal");
    return std::stoull(text);
}
struct Subtree {
    U first;
    int height;
    bool taller_left;
    bool on_delete_path;
};

std::pair<U, U> make_tall_avl(Writer& w, U board, U budget) {
    // Minimum node count for each AVL height, starting with an empty subtree.
    std::vector<U> nodes{0, 1};
    while (1 + nodes.back() + nodes[nodes.size()-2] <= budget)
        nodes.push_back(1 + nodes.back() + nodes[nodes.size()-2]);

    std::queue<Subtree> pending;
    pending.push({1, static_cast<int>(nodes.size()) - 1, true, true});
    U remove_id = 1;
    // Level-order insertion preserves the shape without rotations.
    while (!pending.empty()) {
        auto [first, height, taller_left, on_delete_path] = pending.front();
        pending.pop();
        int tall = height - 1, short_height = std::max(0, height - 2);
        int left = taller_left ? tall : short_height;
        int right = taller_left ? short_height : tall;
        U id = first + nodes[left];
        w.line("ADD", board, id, -S(id));
        if (on_delete_path && height <= 2) remove_id = id;
        // Inward-leaning tall children force double rotations on the return path.
        if (left) pending.push({first, left, false, on_delete_path && !taller_left});
        if (right) pending.push({id+1, right, true, on_delete_path && taller_left});
    }
    return {nodes.back(), remove_id};
}

void generate(const std::string& name, U n, U ops, U seed, Writer& w) {
    if (name == "rotations") {
        U board = 0, next = 1, id = 1;
        for (const auto& scores : std::vector<std::vector<S>>{
                {30,20,10}, {10,20,30}, {30,10,20}, {10,30,20}}) {
            U start = id;
            for (S s : scores) w.add(board, id++, s);
            w.page(board, 3);
            for (U p = start; p < id; p++) {
                w.line("REMOVE", board, p); w.line("SIZE", board);
            }
            w.line("SPLIT", board, 0, next, next+1);
            w.line("MERGE", next, next+1, next+2);
            board = next+2; next += 3;
        }
    } else if (name == "cascade") {
        auto [players, remove_id] = make_tall_avl(w, 0, n);
        w.page(0, players);
        w.line("REMOVE", 0, remove_id);
        w.page(0, players - 1);
    } else if (name == "unequal") {
        w.line("SPLIT", 0, 0, 1, 2);
        w.line("ADD", 1, n, 1);
        auto players = make_tall_avl(w, 2, n - 1).first;
        w.page(1, 1); w.page(2, players);
        w.line("MERGE", 1, 2, 3);
        w.page(3, players + 1);
    } else if (name == "ascending" || name == "descending" || name == "ties") {
        // Descending IDs into one tie bucket force vector insertion at its front.
        for (U i = 1; i <= n; i++)
            w.add(0, name == "ties" ? n+1-i : i,
                  name == "ties" ? 90 : name == "ascending" ? S(i) : -S(i));
        w.page(0, n);
        for (U i = 1; i <= std::min<U>(n,12); i++) w.line("RANK", 0, i);
        w.line("SET", 0, 1, 91); w.line("RANK", 0, 1);
        w.line("SET", 0, 1, 90); w.page(0,n);
    } else if (name == "churn") {
        for (U i=1; i<=n; i++) w.add(0,i,S(i%7));
        for (U i=1; i<=n; i++) {
            w.line("REMOVE",0,i); w.line("ADD",0,i,-S(i));
            w.line("SET",0,i,-S(i)); w.line("RANK",0,i);
        }
        for (U i=n; i>0; i--) { w.line("REMOVE",0,i); w.line("SIZE",0); }
        w.line("SELECT",0,1);
    } else if (name == "extremes") {
        w.add(0,0,std::numeric_limits<S>::min());
        w.add(0,2147483647,std::numeric_limits<S>::max()); w.add(0,3,0); w.add(0,2,0);
        w.page(0,4); w.line("RANK",0,3);
        w.line("SET",0,0,std::numeric_limits<S>::max()); w.page(0,4);
        w.line("SPLIT",0,std::numeric_limits<S>::min(),1,2);
        w.line("SIZE",1); w.line("SIZE",2); w.line("MERGE",1,2,3); w.page(3,4);
    } else if (name == "seasons") {
        for (U i=1; i<=n; i++) w.add(0,i,S(i));
        S threshold = S(n/2+1);
        w.line("SPLIT",0,threshold,1,2);
        w.page(1,n-U(threshold)+1); w.page(2,U(threshold)-1);
        w.line("SIZE",0); // consumed source must be rejected
        // With at least two low-division players, this makes the ranges overlap.
        w.line("SET",2,1,S(n)+1); w.line("RANK",2,1);
        w.line("MERGE",1,2,3); w.page(3,n);
        w.line("SIZE",1); w.line("SIZE",2);
        w.line("SPLIT",3,std::numeric_limits<S>::max(),4,5);
        w.line("SIZE",4); w.line("SIZE",5); w.line("MERGE",4,5,6); w.page(6,n);
        w.line("SPLIT",6,std::numeric_limits<S>::min(),7,8);
        w.line("SIZE",7); w.line("SIZE",8); w.line("MERGE",8,7,9); w.page(9,n);
    } else if (name == "errors") {
        w.line("SELECT",0,0); w.line("SELECT",0,1); w.line("RANGE",0,1,0);
        w.add(0,1,10); w.add(0,1,20); w.line("RANK",0,1);
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
        std::vector<U> where(n,0);        // board holding each live slot while split
        U board=0, next_board=1;          // a consumed board id is never reused
        U hi=0, lo=0;
        bool split=false;
        // Reserve a later command for the merge before starting a split.
        const U period=std::max<U>(10, ops/200);
        for (U i=0; i<ops; i++) {
            if ((i + 1) % period == 0 && (split || ops - i > period)) {
                if (split) {
                    U result=next_board++;
                    w.line("MERGE", hi, lo, result);
                    board=result; split=false;
                } else {
                    hi=next_board++; lo=next_board++;
                    w.line("SPLIT", board, 0, hi, lo);
                    for (U s=0; s<n; s++) if (live[s]) where[s]=scores[s]>=0?hi:lo;
                    split=true;
                }
                continue;
            }
            U slot=rng()%n, id=slot+1, kind=rng()%8;
            S s=S(rng()%17)-8;
            if (!live[slot]) {
                U target=split?hi:board;
                w.add(target,id,s); live[slot]=true; scores[slot]=s; where[slot]=target;
                continue;
            }
            U target=split?where[slot]:board;
            if (kind==1 || kind==2) {
                w.line("SET",target,id,kind==1 ? scores[slot] : s);
                if (kind==2) scores[slot]=s;
            } else if (kind==3) { w.line("REMOVE",target,id); live[slot]=false; }
            else if (kind==6) w.line("SELECT",target,1);
            else if (kind==7) w.line("SIZE",target);
            else w.line("RANK",target,id);
        }
    } else if (name == "scale") {
        // Stream, rather than retain, one million players and score revisions.
        for (U i=1; i<=n; i++) w.add(0,i,S(i));
        for (U i=0; i<ops; i++) {
            U id = i % n + 1;
            w.line("SET", 0, id, n+i+1);
            if ((i+1) % 1000 == 0) {
                w.line("RANK", 0, id);
                w.line("RANK", 0, 1);
                w.line("SELECT", 0, n/2+1);
            }
        }
        w.page(0,n);
        if (n >= 50) w.line("RANGE", 0, 40, 50);
        U high = std::min(n, ops - ops/2);
        w.line("SPLIT", 0, n+ops/2+1, 1, 2);
        w.page(1, high); w.page(2, n-high);
        w.line("MERGE", 1, 2, 3);
        w.page(3, n);
    } else throw std::invalid_argument("unknown case: "+name);
}
int main(int argc,char** argv) {
    std::ios::sync_with_stdio(false);
    try {
        std::string name="ties"; U n=64,ops=10000,seed=253;
        for (int i=1;i<argc;i++) {
            std::string arg=argv[i];
            if (arg=="--help") {
                std::cout << "generator --case rotations|cascade|ascending|descending|ties|churn|extremes|seasons|unequal|errors|mixed|scale --players 64 --ops 10000 --seed 253\n";
                return 0;
            }
            if (i+1==argc) throw std::invalid_argument("missing option value");
            i++; std::string value=argv[i];
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

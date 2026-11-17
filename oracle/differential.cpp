#include "backend_tools.hpp"
#include "../app/replay.hpp"
#include <iostream>
#include "replay.hpp"
#include <filesystem>
#include <fstream>
#include <random>
#include <chrono>

namespace {
using namespace oracle;
using Trace=std::vector<std::string>;
void require(bool ok, const char* message) {
    if (!ok) throw std::logic_error(message);
}
std::vector<Player> inspect(const Board& board) {
    if (auto o = dynamic_cast<const SortedArray*>(&board)) { o->audit(); return o->entries(); }
    try { return audit_avl(board); }
    catch (const std::bad_cast&) { return audit_fenwick(board); }
}
struct Failure { std::size_t step; std::string signature; };

void check_state(const Session& s) {
    std::size_t total=0;
    for (const auto& board:s.boards) {
        auto entries=inspect(*board.second);
        require(entries.size()==board.second->size(),"backend size"); total+=entries.size();
        for (auto p:entries) {
            auto it=s.players.find(p.id);
            require(it!=s.players.end() && it->second.first==board.first && it->second.second==p.score,
                    "player lookup/board state mismatch");
        }
    }
    require(total==s.players.size(),"player lookup total");
}
Failure replay(const Trace& trace,bool inject=false) {
    auto scores=coordinates(trace);
    Session expected([](){return std::make_unique<SortedArray>();});
    Session a([](){return make_avl();});
    Session b([&](){return make_fenwick(scores);});
    std::ostringstream input, expected_output;
    for (std::size_t i=0;i<trace.size();i++) {
        auto c=parse(trace[i]);
        std::string want;
        try {want=expected.execute(c);check_state(expected);}
        catch (const std::exception& e) {return {i,std::string("oracle: ")+e.what()};}
        input << trace[i] << '\n'; expected_output << want << '\n';
        for (auto backend:{std::make_pair("AVL",&a),std::make_pair("Fenwick",&b)}) {
            try {
                auto actual=backend.second->execute(c);
                // Deliberate test-only fault used to verify the entire shrink path.
                if (inject && backend.second==&a && c.name=="RANK" && want.find("RANK ")==0)
                    actual="RANK injected";
                if (actual!=want) throw std::logic_error("output mismatch for "+c.name);
                require(backend.second->players==expected.players,"lookup differs from oracle");
                require(backend.second->boards.size()==expected.boards.size(),"board count differs");
                for (auto& board:expected.boards) {
                    const auto& actual_board = *backend.second->boards.at(board.first);
                    auto values = inspect(actual_board);
                    require(values.size() == actual_board.size(), "backend size");
                    const auto& reference=dynamic_cast<SortedArray&>(*board.second).entries();
                    require(values.size()==reference.size() &&
                        std::equal(values.begin(),values.end(),reference.begin(),equal),"contents differ from oracle");
                }
            } catch (const std::exception& e) {return {i,std::string(backend.first)+": "+e.what()};}
        }
    }
    for (bool fenwick : {false, true}) {
        std::istringstream in(input.str());
        std::ostringstream out;
        try {
            leaderboard::run_replay(in, out, [&](const std::vector<Score>& domain) {
                return fenwick ? make_fenwick(domain) : make_avl();
            });
            if (out.str() != expected_output.str()) {
                std::istringstream actual(out.str()), expected_lines(expected_output.str());
                std::string got, want;
                for (std::size_t i = 0; i < trace.size(); i++) {
                    if (!std::getline(actual, got) || !std::getline(expected_lines, want) || got != want)
                        return {i, fenwick ? "app Fenwick output mismatch" : "app AVL output mismatch"};
                }
                return {trace.empty() ? 0 : trace.size() - 1, "app extra output"};
            }
        } catch (const std::exception& e) {
            return {trace.empty() ? 0 : trace.size()-1, std::string("app: ")+e.what()};
        }
    }
    return {trace.size(),""};
}

// Delta debugging followed by single-deletion convergence. The result is
// 1-minimal for the same failure signature, not necessarily globally shortest.
Trace shrink(Trace trace,const std::string& signature,bool inject=false) {
    auto fails=[&](const Trace& candidate){return replay(candidate,inject).signature==signature;};
    std::size_t chunks=2;
    while (trace.size()>=2) {
        std::size_t width=(trace.size()+chunks-1)/chunks; bool reduced=false;
        for (std::size_t start=0;start<trace.size();start+=width) {
            Trace trial=trace;
            trial.erase(trial.begin()+start,trial.begin()+std::min(trace.size(),start+width));
            if (fails(trial)) {trace.swap(trial);chunks=std::max<std::size_t>(2,chunks-1);reduced=true;break;}
        }
        if (!reduced) {if(chunks>=trace.size())break;chunks=std::min(trace.size(),chunks*2);}
    }
    for (std::size_t i=0;i<trace.size();) {
        Trace trial=trace;trial.erase(trial.begin()+i);
        if (fails(trial)) {trace.swap(trial);i=0;} else i++;
    }
    return trace;
}
void save(const std::string& path,const Trace& t) {
    std::ofstream out(path); if (!out) throw std::runtime_error("cannot write "+path);
    for (const auto& line:t) out<<line<<'\n';
    out.flush(); if (!out) throw std::runtime_error("cannot write "+path);
}
Trace read(const std::string& path) {
    if (std::filesystem::is_directory(path)) throw std::runtime_error("cannot read directory " + path);
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot read " + path);
    Trace trace;
    std::string line;
    while (std::getline(in,line)) trace.push_back(line);
    if (in.bad() || !in.eof()) throw std::runtime_error("cannot read " + path);
    return trace;
}
Trace generate(std::uint64_t seed,std::size_t count,std::size_t limit) {
    std::mt19937_64 rng(seed); Trace t; t.reserve(count);
    Session model([](){return std::make_unique<SortedArray>();});
    std::uint64_t current=0,next_board=1;
    auto append=[&](std::string line){model.execute(parse(line));t.push_back(std::move(line));};
    auto score=[&](){ auto r=rng()%40;return r==0 ? std::numeric_limits<Score>::min() :
        r==1 ? std::numeric_limits<Score>::max() : static_cast<Score>(rng()%33)-16;};
    while (t.size()<count) {
        auto prefix=std::to_string(current)+" "; auto kind=rng()%12;
        auto id=rng()%(limit+16);
        if (!model.players.empty() && rng()%4!=0) {
            auto it=model.players.begin();std::advance(it,rng()%model.players.size());id=it->first;
        }
        if (kind==0 && t.size()+2<=count) {
            auto high=next_board++,low=next_board++,joined=next_board++;
            append("SPLIT "+prefix+std::to_string(score())+" "+std::to_string(high)+" "+std::to_string(low));
            append("MERGE "+std::to_string(high)+" "+std::to_string(low)+" "+std::to_string(joined));current=joined;
        } else if (kind<=2 && model.players.size()<limit) {
            append("ADD "+prefix+std::to_string(rng()%(limit+16))+" "+std::to_string(score()));
        } else if (kind<=4) append("SET "+prefix+std::to_string(id)+" "+std::to_string(score()));
        else if (kind==5) append("REMOVE "+prefix+std::to_string(id));
        else if (kind==6 || kind==7) append("RANK "+prefix+std::to_string(id));
        else if (kind==8) append("SELECT "+prefix+std::to_string(rng()%(model.players.size()+2)));
        else if (kind==9) {
            auto first=rng()%(model.players.size()+2),last=rng()%(model.players.size()+2);
            if (first>last) std::swap(first,last);
            append("RANGE "+prefix+std::to_string(first)+" "+std::to_string(last));
        } else if (kind==10) append("SIZE "+std::to_string(current));
        else append("SELECT "+prefix+"-1");
    }
    return t;
}
template<class F> void must_throw(F f) {
    bool threw=false;try {f();}catch(const std::exception&){threw=true;}
    require(threw,"expected exception");
}
void self_test() {
    const auto max_id=std::numeric_limits<PlayerId>::max();
    SortedArray o({{max_id,std::numeric_limits<Score>::min()},{0,std::numeric_limits<Score>::max()},
                   {8,90},{3,90}});
    require(o.select(1).id==0 && o.select(3).id==8 && o.count_strictly_higher(90)==1,"oracle ties/extremes");
    must_throw([&](){o.insert({8,100});});must_throw([&](){o.erase({8,91});});
    must_throw([&](){o.select(0);});must_throw([&](){o.select(5);});
    std::vector<Player> page;must_throw([&](){o.range(2,1,page);});
    SortedArray duplicate({{8,7}});must_throw([&](){o.merge_from(duplicate);});
    require(o.size()==4 && duplicate.size()==1,"failed merge mutated inputs");
    auto parts=o.split_by_score(90);require(o.size()==0 && parts.first->size()==3,"oracle split");
    parts.first->merge_from(*parts.second);require(parts.first->size()==4 && !parts.second->size(),"oracle merge");
    Trace faulty={"SIZE 0","ADD 0 7 10","ADD 0 8 20","RANK 0 7","REMOVE 0 8"};
    auto failure=replay(faulty,true);require(!failure.signature.empty(),"fault injection not detected");
    auto minimal=shrink(faulty,failure.signature,true);require(minimal.size()==2,"shrinker result");
    for (std::size_t i=0;i<minimal.size();i++) {
        auto trial=minimal;trial.erase(trial.begin()+i);
        require(replay(trial,true).signature!=failure.signature,"shrinker not 1-minimal");
    }
    // All four rotation patterns, deletions and highly unequal-height joins.
    for (auto sequence:std::vector<std::vector<int>>{{30,20,10},{10,20,30},{30,10,20},{10,30,20}}) {
        auto tree=make_avl();for(auto s:sequence){tree->insert({static_cast<PlayerId>(s),s});inspect(*tree);}
        for(auto s:sequence){tree->erase({static_cast<PlayerId>(s),s});inspect(*tree);}
    }
    for (bool reverse:{false,true}) {
        auto large=make_avl(),small=make_avl();
        for(int i=0;i<4096;i++)large->insert({static_cast<PlayerId>(i),reverse ? i : i+2});
        small->insert({5000,reverse ? 5000 : 0});
        large->merge_from(*small);inspect(*large);require(large->size()==4097 && !small->size(),"unequal join");
    }
    std::cout<<"PASS oracle boundaries, rotations, unequal joins, shrinker\n";
}
void scale(std::size_t count) {
    require(count>0 && count<=1000000,"scale must be 1..1000000");
    std::vector<Player> data;std::vector<Score> scores;
    for(std::size_t i=0;i<count;i++){data.push_back({i,static_cast<Score>(i/4)});scores.push_back(i/4);}
    SortedArray ref(data);auto a=make_avl(),b=make_fenwick(scores);
    for(auto p:data){a->insert(p);b->insert(p);}
    auto check=[&](Board& board){auto got=inspect(board);require(got.size()==ref.size() &&
        std::equal(got.begin(),got.end(),ref.entries().begin(),equal),"scale contents");};
    check(*a);check(*b);
    // One query per player, alternating rank and select, verified independently.
    for(std::size_t i=0;i<count;i++) {
        if(i%2){auto p=ref.select(i+1);require(equal(a->select(i+1),p)&&equal(b->select(i+1),p),"scale select");}
        else {auto s=data[i].score;auto rank=ref.count_strictly_higher(s);
            require(a->count_strictly_higher(s)==rank && b->count_strictly_higher(s)==rank,"scale rank");}
    }
    for(Board* board:std::vector<Board*>{a.get(),b.get()}) {
        auto pieces=board->split_by_score(static_cast<Score>(count/8));
        require(!board->size(),"scale split source");inspect(*pieces.first);inspect(*pieces.second);
        pieces.first->merge_from(*pieces.second);require(!pieces.second->size(),"scale merge source");check(*pieces.first);
    }
    std::cout<<"PASS scale players="<<count<<" queries="<<count<<" split/merge (full audits at phase boundaries)\n";
}
}
int main(int argc,char** argv) {
    try {
        std::uint64_t seed=253;std::size_t ops=1000000,limit=128;std::string input,dump;
        bool only_self=false,inject=false;std::size_t scale_count=0;
        for(int i=1;i<argc;i++) {
            std::string arg=argv[i];
            if(arg=="--self-test")only_self=true;
            else if(arg=="--inject-fault")inject=true;
            else {
                if(i+1==argc)throw std::invalid_argument("missing argument");
                i++; std::string value=argv[i];
                if(arg=="--seed")seed=unsigned_number(value);
                else if(arg=="--ops")ops=unsigned_number(value);
                else if(arg=="--max-live")limit=unsigned_number(value);
                else if(arg=="--trace")input=value;
                else if(arg=="--dump")dump=value;
                else if(arg=="--scale") {
                    scale_count=unsigned_number(value);
                    if(!scale_count || scale_count>1000000)
                        throw std::invalid_argument("scale must be 1..1000000");
                }
                else throw std::invalid_argument("unknown option: "+arg);
            }
        }
        if(!limit || limit>1000000)throw std::invalid_argument("max-live must be 1..1000000");
        self_test();if(only_self)return 0;if(scale_count){scale(scale_count);return 0;}
        auto start=std::chrono::steady_clock::now();
        auto trace=input.empty()?generate(seed,ops,limit):read(input);
        if(!dump.empty())save(dump,trace);
        auto context = input.empty()
            ? "seed=" + std::to_string(seed) + " max_live=" + std::to_string(limit)
            : "trace=" + input;
        auto failure=replay(trace,inject);
        if(!failure.signature.empty()) {
            if (failure.step < trace.size()) trace.resize(failure.step + 1);
            save("oracle-failure.trace",trace);
            std::cerr<<"FAIL "<<context<<" step="<<failure.step+1<<" "<<failure.signature<<'\n';
            auto minimal=shrink(trace,failure.signature,inject);save("oracle-failure.min.trace",minimal);
            std::cerr<<"Saved full failing prefix and 1-minimal trace ("<<minimal.size()<<" commands)\n";return 1;
        }
        double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::cout<<"PASS "<<context<<" commands="<<trace.size()
                 <<" full invariant/state audits after every command seconds="<<seconds<<'\n';
    } catch(const std::exception& e){std::cerr<<"ERROR "<<e.what()<<'\n';return 2;}
}

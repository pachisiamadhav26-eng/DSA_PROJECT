#pragma once
#include "sorted_array.hpp"
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <limits>

namespace oracle {
using Factory=std::function<std::unique_ptr<Board>()>;
struct Command {
    std::string name;
    std::vector<std::uint64_t> u;
    Score score=0;
    bool valid=false;
};
inline std::uint64_t unsigned_number(const std::string& s) {
    if (s.empty() || s.find_first_not_of("0123456789")!=std::string::npos)
        throw std::invalid_argument("unsigned decimal");
    return std::stoull(s);
}
inline Score signed_number(const std::string& s) {
    std::size_t start=(!s.empty() && s[0]=='-') ? 1 : 0;
    if (start==s.size() || s.find_first_not_of("0123456789",start)!=std::string::npos)
        throw std::invalid_argument("signed decimal");
    return std::stoll(s);
}
inline Command parse(const std::string& line) {
    std::istringstream in(line); Command c; in>>c.name;
    std::vector<std::string> args; std::string token;
    while (in>>token) args.push_back(token);
    int count=0,score_index=-1;
    if (c.name=="ADD" || c.name=="SET") {count=3;score_index=2;}
    else if (c.name=="REMOVE" || c.name=="RANK" || c.name=="SELECT") count=2;
    else if (c.name=="RANGE" || c.name=="MERGE") count=3;
    else if (c.name=="SIZE") count=1;
    else if (c.name=="SPLIT") {count=4;score_index=1;}
    else return c;
    if (args.size()!=static_cast<std::size_t>(count)) return c;
    try {
        for (int i=0;i<count;++i) {
            if (i==score_index) c.score=signed_number(args[i]);
            else c.u.push_back(unsigned_number(args[i]));
        }
        c.valid=true;
    } catch (const std::exception&) { c.valid=false; }
    return c;
}

// Test-only application facade. This is NOT the required handwritten hash table.
// Error precedence is deterministic; the architect specifies codes, not precedence
// when one command violates multiple preconditions simultaneously.
class Session {
    Factory factory_;
    std::set<std::uint64_t> used_{0};
public:
    std::map<std::uint64_t,std::unique_ptr<Board>> boards;
    std::map<PlayerId,std::pair<std::uint64_t,Score>> players;
    explicit Session(Factory factory):factory_(std::move(factory)) { boards[0]=factory_(); }
    std::string execute(const Command& c) {
        if (!c.valid) return "ERR BAD_COMMAND";
        auto b=c.u[0]; auto found=boards.find(b);
        if (found==boards.end()) return "ERR UNKNOWN_BOARD";
        Board& board=*found->second;
        std::ostringstream out;
        if (c.name=="ADD") {
            auto id=c.u[1];
            if (players.count(id)) return "ERR DUPLICATE_ID";
            board.insert({id,c.score}); players[id]={b,c.score};
        } else if (c.name=="SET" || c.name=="REMOVE" || c.name=="RANK") {
            auto p=players.find(c.u[1]);
            if (p==players.end()) return "ERR UNKNOWN_PLAYER";
            if (p->second.first!=b) return "ERR WRONG_BOARD";
            Player entry{p->first,p->second.second};
            if (c.name=="RANK") return "RANK "+std::to_string(1+board.count_strictly_higher(entry.score));
            if (c.name=="REMOVE") { board.erase(entry); players.erase(p); }
            else if (entry.score!=c.score) {
                board.erase(entry); board.insert({entry.id,c.score}); p->second.second=c.score;
            }
        } else if (c.name=="SELECT") {
            if (!c.u[1] || c.u[1]>board.size()) return "ERR BAD_POSITION";
            auto p=board.select(c.u[1]); out<<"PLAYER "<<p.id<<' '<<p.score; return out.str();
        } else if (c.name=="RANGE") {
            if (!c.u[1] || c.u[1]>c.u[2] || c.u[2]>board.size()) return "ERR BAD_RANGE";
            std::vector<Player> page{{999,999}}; // range must replace previous contents
            board.range(c.u[1],c.u[2],page); out<<"PLAYERS "<<page.size();
            for (auto p:page) out<<' '<<p.id<<' '<<p.score;
            return out.str();
        } else if (c.name=="SIZE") return "SIZE "+std::to_string(board.size());
        else if (c.name=="SPLIT") {
            auto high=c.u[1],low=c.u[2];
            if (high==low || used_.count(high) || used_.count(low)) return "ERR BOARD_EXISTS";
            auto pieces=board.split_by_score(c.score);
            if (board.size()!=0) throw std::logic_error("split source not empty");
            boards[high]=std::move(pieces.first); boards[low]=std::move(pieces.second);
            boards.erase(b); used_.insert(high); used_.insert(low);
            for (auto& p:players) if (p.second.first==b)
                p.second.first=p.second.second>=c.score ? high : low;
        } else if (c.name=="MERGE") {
            auto right=c.u[1],result=c.u[2];
            if (!boards.count(right)) return "ERR UNKNOWN_BOARD";
            if (right==b) return "ERR SAME_BOARD";
            if (used_.count(result)) return "ERR BOARD_EXISTS";
            // Global uniqueness at ADD makes ID_CONFLICT unreachable in a valid session.
            board.merge_from(*boards.at(right));
            if (boards.at(right)->size()!=0) throw std::logic_error("merge source not empty");
            boards[result]=std::move(found->second); boards.erase(b); boards.erase(right);
            used_.insert(result);
            for (auto& p:players) if (p.second.first==b || p.second.first==right) p.second.first=result;
        }
        return "OK";
    }
};
inline std::vector<Score> coordinates(const std::vector<std::string>& lines) {
    std::vector<Score> scores;
    for (const auto& line:lines) {
        auto c=parse(line);
        if (c.valid && (c.name=="ADD" || c.name=="SET")) scores.push_back(c.score);
    }
    std::sort(scores.begin(),scores.end()); scores.erase(std::unique(scores.begin(),scores.end()),scores.end());
    return scores;
}
}

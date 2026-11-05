#include "spray_roster.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("spray roster regression");}
int main(){using namespace visual::spraysync;constexpr Peer a=76561198000000001ull,b=76561198000000002ull;
 std::array<unsigned char,144> info{};std::memcpy(info.data(),&a,8);check(playerSteamId(info.data(),info.size())==a);info[116]=1;check(!playerSteamId(info.data(),info.size()));info[116]=0;info[117]=1;check(!playerSteamId(info.data(),info.size()));info[117]=0;
 auto account=std::uint32_t(b);std::memcpy(info.data()+80,&account,4);check(!playerSteamId(info.data(),info.size()));std::memset(info.data(),0,8);check(playerSteamId(info.data(),info.size())==b);check(!playerSteamId(info.data(),143));
 Roster r;r.active=true;r.generation=1;r.tick=100;r.local=a;r.players={a,b};check(r.fresh(5100)&&!r.fresh(5101)&&!r.fresh(99));check(r.others()==std::set<Peer>{b});auto cohort=r.cohort();r.players={b,a};check(r.cohort()==cohort);r.players={a};check(r.cohort()!=cohort);r.players={a,a};check(!r.fresh(101));
 std::cout<<"PASS: bounded Source player info, bot/TV exclusion, SteamID consistency, fresh roster and deterministic same-game cohort\n";
}

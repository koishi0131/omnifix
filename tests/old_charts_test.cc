#include "old_charts.h"
#include <cassert>
#include <array>
#include <iostream>
using namespace omnifix;
int main() {
    constexpr std::size_t count=34000, stride=0x800, first=16+count*4;
    constexpr std::array<int,6> mids={499,500,32761,33499,33500,33801};
    auto make=[&](int version) {
        std::vector<std::uint8_t> b(first+mids.size()*stride,0x55);
        auto put=[&](std::size_t off,int x){std::memcpy(b.data()+off,&x,4);};
        std::memcpy(b.data(),"IIDX",4);put(4,version);put(8,mids.size());put(12,count);
        for(std::size_t i=0;i<count;++i)put(16+i*4,-1);
        for(std::size_t i=0;i<mids.size();++i)put(16+mids[i]*4,i);
        return b;
    };
    for(int version:{33,34}) {
        auto b=make(version),expected=b;
        for(std::size_t i=0;i<mids.size();++i)
            if(i==1||i==2||i==4||i==5)std::memset(expected.data()+first+i*stride+0x3ec,0,10);
        auto out=old_charts::clear_ratings(b);
        assert(out.valid&&out.songs==4&&b==expected);
        auto again=old_charts::clear_ratings(b);assert(again.valid&&b==expected);
        b=make(version);int invalid=999;std::memcpy(b.data()+16+33999*4,&invalid,4);
        auto original=b;assert(!old_charts::clear_ratings(b).valid&&b==original);
        b=make(version);b.pop_back();original=b;
        assert(!old_charts::clear_ratings(b).valid&&b==original);
    }
    auto b=make(99),original=b;assert(!old_charts::clear_ratings(b).valid&&b==original);
    std::vector<std::uint8_t> short_data(8);assert(!old_charts::clear_ratings(short_data).valid);
    assert(!old_charts::matches(-1)&&!old_charts::matches(33000)&&old_charts::matches(33999));
    std::cout<<"PASS: 33/34 buffers; only ten rating bytes per selected song change; boundaries, repeated reads and malformed data checked.\n";
}

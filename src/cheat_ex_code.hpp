#pragma once
#include <array>
#include <cstddef>
namespace mcl {
class CheatExCode {
 static constexpr std::array<int,7> sequence_={40,39,40,37,88,67,13};
 std::size_t at_=0;
 bool unlocked_=false;
public:
 bool unlocked()const{return unlocked_;}
 bool feed(int key){
  if(unlocked_)return false;
  at_=(key==sequence_[at_])?at_+1:(key==sequence_[0]?1:0);
  if(at_==sequence_.size()){unlocked_=true;at_=0;return true;}
  return false;
 }
 void reset(){at_=0;unlocked_=false;}
};
}

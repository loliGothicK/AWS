#include <array>
#include <iostream> 
#include <random>
#include <vector>
#include <chrono>
#include <thread>
#include "sqrt.hpp"


/*
[ コメント:

  1. `int`を`UIntFast`にかえた。負の値になることはないだろうから。
  2. `sqrt_u`はコードがエレファントだったのでフルスクラッチした。
  3. せっかくC++17を使っているので、使い倒した。
  4. 肝心のthreadの部分であるが、ラムダ式を使ったら楽かなと思った。もっとラムダ式を使おう。

-- end note ]
*/

using UIntFast = std::uint_fast32_t;

/*[ Note:
  nested namespace declaration is C++17 feature.
-- end note]*/
namespace cranberries::aws {

/*[ Note:
  inline variable is C++17 feature.
-- end note]*/
inline constexpr UIntFast MAX_ENEMY_AIRCRAFT_PER_SLOT = 256;
inline constexpr UIntFast MAX_ENEMY_AIRCRAFT_STAT_AA = 32;
inline constexpr UIntFast MAX_EQUIP = 5;
inline constexpr UIntFast MAX_WARSHIP = 12;


template <UIntFast EQ, UIntFast WS >
struct basic_remain_aircraft {
  std::array<std::array<UIntFast, EQ>, WS> num_remain_aircraft;
};

using remain_aircraft = basic_remain_aircraft<MAX_EQUIP, MAX_WARSHIP>;

template struct basic_remain_aircraft<MAX_EQUIP, MAX_WARSHIP>;


template <UIntFast EQ, UIntFast WS >
struct basic_aircraft_type {
  using value_type = std::array<std::array<UIntFast, EQ>, WS>;

  value_type stat_AA;
  value_type aircraft_num_start;
  value_type proficiency_bounus;
  value_type participate_blast;
  value_type participate_LBAS;
  value_type participate_st1;
  value_type participate_st2;

  constexpr operator basic_remain_aircraft<EQ, WS>() noexcept { return { aircraft_num_start };  }
};

using aircraft_type = basic_aircraft_type<MAX_EQUIP, MAX_WARSHIP>;
template struct basic_aircraft_type<MAX_EQUIP, MAX_WARSHIP>;


// 制空状態を表す
enum class stat_AW {
  SUPREMANCY,
  SUPERIORITY,
  PSRITY,
  DENIAL,
  INCAPABILITY,
};



class calc_AW {
private:
  std::mt19937_64 mt;
  std::uniform_int_distribution<UIntFast> rand_enemy_remain_supremancy{ 0, 10 };
  std::uniform_int_distribution<UIntFast> rand_enemy_remain_superiority{ 0, 8 };
  std::uniform_int_distribution<UIntFast> rand_enemy_remain_parity{ 0, 6 };
  std::uniform_int_distribution<UIntFast> rand_enemy_remain_denial{ 0, 4 };
  std::uniform_int_distribution<UIntFast> rand_enemy_remain_incapability{ 0, 1 };

public:
  inline static constexpr std::array<std::array<std::array<UIntFast, 11>, 11>, MAX_ENEMY_AIRCRAFT_PER_SLOT> table_num_remain_enemy = []
  {
    std::array<std::array<std::array<UIntFast, 11>, 11>, MAX_ENEMY_AIRCRAFT_PER_SLOT> table = {};
    UIntFast i = 0, j = 0, k = 0;
    for (i = 0; i < MAX_ENEMY_AIRCRAFT_PER_SLOT; i++) {
      for (j = 0; j < 11; j++) {
        for (k = 0; k < 11; k++) {
          table[i][j][k] = i - static_cast<UIntFast>(((0.65 * j) + (0.35 * k)) / 10 * i);
        }
      }
    }
    return table;
  }();
  inline static constexpr std::array<std::array<UIntFast, MAX_ENEMY_AIRCRAFT_STAT_AA>, MAX_ENEMY_AIRCRAFT_PER_SLOT> table_enemy_fighter_power = []
  {
    std::array<std::array<UIntFast, MAX_ENEMY_AIRCRAFT_STAT_AA>, MAX_ENEMY_AIRCRAFT_PER_SLOT> temp = {};
    for (UIntFast i = 0; i < MAX_ENEMY_AIRCRAFT_PER_SLOT; i++) {
      for (UIntFast j = 0; j < MAX_ENEMY_AIRCRAFT_STAT_AA; j++) {
        temp[i][j] = constexpr_sqrt_u::sqrt_u(i * j * j);
      }
    }
    return temp;
  }();
  calc_AW();
  inline static auto calc_enemy_fighter_power(aircraft_type&, remain_aircraft&, bool is_LBAS = false, UIntFast calc_enemy_num = MAX_WARSHIP);
  inline static auto calc_stat_AW(UIntFast self_fighter_power, UIntFast enemy_fighter_power);
  void calc_enemy_aircraft_decrease(aircraft_type&, remain_aircraft&, stat_AW, bool is_LBAS = false, UIntFast calc_enemy_num = MAX_WARSHIP);
};

// 敵制空値を計算
auto calc_AW::calc_enemy_fighter_power(aircraft_type& type, remain_aircraft& nums, bool is_LBAS, UIntFast calc_enemy_num)
{
  UIntFast fighter_power = 0;
  // specified const
  const auto& is_join = (is_LBAS ? type.participate_LBAS : type.participate_st1);
  for (UIntFast i = 0; i < calc_enemy_num; i++) {
    for (UIntFast j = 0; j < MAX_EQUIP; j++) {
      if (is_join[i][j]) {
        fighter_power += table_enemy_fighter_power[nums.num_remain_aircraft[i][j]][type.stat_AA[i][j]];
      }
    }
  }
  return fighter_power;
}

// 制空状態を計算
auto calc_AW::calc_stat_AW(UIntFast self_fighter_power, UIntFast enemy_fighter_power)
{
  if (self_fighter_power >= enemy_fighter_power * 3) {
    return stat_AW::SUPREMANCY;
  }
  else if (self_fighter_power * 2 >= enemy_fighter_power * 3) {
    return stat_AW::SUPERIORITY;
  }
  else if (self_fighter_power * 3 >= enemy_fighter_power * 2) {
    return stat_AW::PSRITY;
  }
  else if (self_fighter_power * 3 >= enemy_fighter_power) {
    return stat_AW::DENIAL;
  }
  else {
    return stat_AW::INCAPABILITY;
  }
}


struct easy_LBAS_stat {
  UIntFast fighter_power;
  UIntFast targeting_num;
};
namespace AWCCC_simulate {
  static void threading_FP_after_LBAS(std::vector<easy_LBAS_stat>& LBAS_stat, std::vector<UIntFast>& result, aircraft_type& type, UIntFast num_calc, UIntFast num_thread = 4, UIntFast calc_enemy_num = MAX_WARSHIP);
}

calc_AW::calc_AW()
{
  std::random_device rd{};
  mt = std::mt19937_64(rd());
}

void AWCCC_simulate::threading_FP_after_LBAS(std::vector<easy_LBAS_stat>& LBAS_stat,
                                             std::vector<UIntFast>& result,
                                             aircraft_type& type,
                                             UIntFast num_calc,
                                             UIntFast num_thread,
                                             UIntFast calc_enemy_num)
{
  auto num = remain_aircraft(type);
  auto max_FP = calc_AW::calc_enemy_fighter_power(type, num, false, calc_enemy_num);
  result.resize(max_FP + 1, 0);
  std::vector<std::vector<UIntFast>> part_result(num_thread);
  std::vector<std::thread> threads;
  std::vector<calc_AW> t_calc(num_thread);

  // range-based for
  for (auto& item: part_result) {
    item.resize(max_FP + 1, 0);
  }
  /*
  [ Note:
    Change log:
      - Use vector::reserve
      - Use vector::emplace_back
      - remove no effect std::move
      - Use lambda expression instead of free function
  -- end note ]
  */
  threads.reserve(num_thread);
  for (UIntFast i = 0; i < num_thread; i++) {
    threads.emplace_back( // C++14 initialized lambda capture
      std::thread([ &object_calc_AW = t_calc[i],      // by reference
                    &type = type,                      // by reference
                    &LBAS_stat = LBAS_stat,            // by reference
                    &result = part_result[i],          // by reference
                    num_calc = num_calc / num_thread,  // by copy
                    calc_enemy_num = 6                // by copy
                  ]()mutable
    {
      for (UIntFast i = 0; i < num_calc; i++) {
        remain_aircraft remain(type);
        stat_AW stat;
        for (auto&& LBAS: LBAS_stat) {
          auto targeting_num = LBAS.targeting_num;
          auto fighter_power = LBAS.fighter_power;
          for (UIntFast j = 0; j < targeting_num; j++) {
            stat = calc_AW::calc_stat_AW(fighter_power, calc_AW::calc_enemy_fighter_power(type, remain, true, calc_enemy_num));
            object_calc_AW.calc_enemy_aircraft_decrease(type, remain, stat, true, calc_enemy_num);
          }
        }
        result[calc_AW::calc_enemy_fighter_power(type, remain, false, calc_enemy_num)] += 1;
      }
    }));
  }
  // range-based for
  for (auto& th: threads) {
    th.join();
  }
  for (UIntFast i = 0; i < result.size(); i++) {
    for (UIntFast j = 0; j < num_thread; j++) {
      result[i] += part_result[j][i];
    }
  }

}

// 敵の航空機を減衰させる
void calc_AW::calc_enemy_aircraft_decrease(aircraft_type& type, remain_aircraft& nums, stat_AW stat, bool is_LBAS, UIntFast calc_enemy_num)
{
  // pretty nested conditional operator 
  auto& dist
      = stat == stat_AW::SUPREMANCY
        ? rand_enemy_remain_supremancy
      : stat == stat_AW::SUPERIORITY
        ? rand_enemy_remain_superiority
      : stat == stat_AW::PSRITY
        ? rand_enemy_remain_parity
      : stat == stat_AW::INCAPABILITY
        ? rand_enemy_remain_incapability
      : // else
        rand_enemy_remain_denial;

  // specified const
  const auto& is_join = is_LBAS ? type.participate_LBAS : type.participate_st1;
  auto&& num_remain_aircraft = nums.num_remain_aircraft;

  for (UIntFast i = 0; i < calc_enemy_num; i++) {
    for (UIntFast j = 0; j < MAX_EQUIP; j++) {
      if (is_join[i][j]) {
        auto&& elem = num_remain_aircraft[i][j];
        auto&& t = table_num_remain_enemy[elem];
        elem = t[dist(mt)][dist(mt)];
      }
    }
  }
}


} // cranberries::aws

int main() {
  namespace aws = cranberries::aws;
  std::cout << aws::calc_AW::table_num_remain_enemy[100][0][0] << std::endl;

  std::vector<aws::easy_LBAS_stat> LBAS(3);
  for (auto&& target : LBAS) {
    target.fighter_power = 412;
    target.targeting_num = 2;
  }
  std::vector<UIntFast> result{};
  constexpr UIntFast num_thread = 4;

  aws::aircraft_type type;


  for (UIntFast i = 0; i < 6; i++) {
    type.aircraft_num_start[i] = { 69,46,46,46 };
    type.participate_LBAS[i] = { true,true,true,true };
    type.participate_st1[i] = { true,true,true,true };
    type.stat_AA[i] = { 11,9,5,6 };
  }
  auto num = aws::remain_aircraft(type);
  std::cout << aws::calc_AW::calc_enemy_fighter_power(type, num, false, 6) << std::endl;
  constexpr UIntFast times_simulation = 1000000;
  auto s = std::chrono::system_clock::now();

  aws::AWCCC_simulate::threading_FP_after_LBAS(LBAS, result, type, times_simulation, num_thread, 6);

  auto e = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count() << "ms" << std::endl;
  UIntFast undercount = 0;

  // for range-based for and structured binding declarations.
  std::array<std::pair<bool, const UIntFast>, 5> percounts
    {
      std::make_pair( false, 50 ),
      std::make_pair( false, 70 ),
      std::make_pair( false, 90 ),
      std::make_pair( false, 95 ),
      std::make_pair( false, 99 )
    };

  /*
  [ Note:
    using C++17 features below;
      - Initializers in if statements.
      - Structured binding declarations.
  -- end note ]
  */
  for (UIntFast i = 0; undercount < times_simulation; i++) {
    undercount += result[i];
    for (auto& percount : percounts) {
      if (auto&[flag, percent] = percount; // C++17 here
          undercount * 100 >= times_simulation * percent && !flag)
      {
        std::cout << percent << "%:" << i << "\t";
        flag = true;
      }
    }
  }
  getchar();
  return 0;
}



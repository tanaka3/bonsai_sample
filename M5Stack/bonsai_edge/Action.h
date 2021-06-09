#pragma once

/**
 * Bonsaiからの指示情報
 * ※基本的には、Bonsainのシミュレータをほぼそのまま移植しています。
 */
class Action{
public:
  int catch_position_x = 0;   // 追う駒のX座標
  int catch_position_y = 0;   // 追う駒のY座標
};

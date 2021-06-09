#pragma once
#include <Arduino_JSON.h>

/**
 * 現在の状態
 * ※基本的には、Bonsainのシミュレータをほぼそのまま移植しています。
 */
class State{
public:
  int run_position_x;   // 逃げる駒のX座標
  int run_position_y;   // 逃げる駒のY座標

  int catch_position_x; // 追う駒のX座標
  int catch_position_y; // 追う駒のY座標

  double distance;      // 距離

  State(){};

  /**
   * JSON情報を出力
   */
  JSONVar toJSON(){
    JSONVar state;
    state["run_position_x"] = run_position_x;
    state["run_position_y"] = run_position_y;
    state["catch_position_x"] = catch_position_x;
    state["catch_position_y"] = catch_position_y;
    state["distance"] = distance;

    return state;
  }
};

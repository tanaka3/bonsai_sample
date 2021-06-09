#pragma once
#include "Config.h"
#include "Action.h"
#include "State.h"

/**
 * 移動先の算出など
 * ※基本的には、Bonsainのシミュレータをほぼそのまま移植しています。
 */
class Model{
private:
  const int LEFT_X = -3;      // toioプレイマットの左端座標
  const int RIGHT_X = 3;      // toioプレイマットの右端座標    
  const int TOP_Y = 2;        // toioプレイマットの上端座標
  const int BOTTOM_Y = -2;    // toioプレイマットの下端座標

  // 逃げる駒の移動先
  const int RUN_DIRECTION[4][2] = {
    {  0,   1 },
    { -1,   0 },
    {  1,   0 },
    {  0,  -1 }
  };

public:
  /**
   * 位置や距離の一時保存用
   */
  class Position{    
  public:
     int x;             // X座標
     int y;             // Y座標
     double distance;   // 距離

     Position(int x, int y, double distance){
       this->x = x;
       this->y = y;
       this->distance = distance;
     }
     
     Position(){
       reset();
     }

     void reset(){
       x = 0;
       y = 0;
       distance = 0;
     }
  };

  int move_type = 0;    // 逃げる駒の方法( 0:追う駒から一番遠い場所を選択 1:移動先からランダムで選択
  
  int run_position_x;   // 逃げる駒のX座標
  int run_position_y;   // 逃げる駒のY座標

  int catch_position_x; // 追う駒のX座標
  int catch_position_y; // 追う駒のY座標
  
  double distance();    // 距離

  Model();                                            
  void reset();                                       // 状態のリセット
  void start(Config config);                          // 追いかけっこの開始
  void step(Action action);                           // 1ターン毎の移動先を計算

  State state();                                      // Stateの出力
  
private:
   double distance(int x1, int y1, int x2, int y2);   // 距離の算出
};
  

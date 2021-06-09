#pragma once

/**
 * 環境設定情報
 * ※基本的には、Bonsainのシミュレータをほぼそのまま移植しています。
 */
class Config{
public:
  int run_position_x;   // 逃げる駒のX座標
  int run_position_y;   // 逃げる駒のY座標

  int catch_position_x; // 追う駒のX座標
  int catch_position_y; // 追う駒のY座標

  Config(){
    reset();
  }

  /**
   * 初期状態にリセットする
   */
  void reset(){
    run_position_x = -2;
    run_position_y = 0;
    catch_position_x = 2;
    catch_position_y = 0;
  }
};

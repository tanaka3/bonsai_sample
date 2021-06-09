#include <Arduino.h>
#include <math.h>
#include <vector>
#include "Model.h"

Model::Model(){
  reset();
}

/**
 * 状態のリセット
 */
void Model::reset(){
  run_position_x = 0;
  run_position_y = 0;
  catch_position_x = 0;
  catch_position_y = 0;  
}

/**
 * 追いかけっこの開始
 * @param config 初期設定
 */
void Model::start(Config config){
  reset();
  run_position_x = config.run_position_x;
  run_position_y = config.run_position_y;
  catch_position_x = config.catch_position_x;
  catch_position_y = config.catch_position_y;  
}

/**
 * 1ターン毎の移動先を計算
 * @param action 追う駒の移動先
 */
void Model::step(Action action){
 
  //指定方向に移動させる
  int catch_x = catch_position_x + action.catch_position_x;
  int catch_y = catch_position_y + action.catch_position_y;

  //toioプレイマット内の場合は、移動先として確定する
  if (!(catch_x < LEFT_X || catch_x > RIGHT_X) &&
    !(catch_y < BOTTOM_Y || catch_y > TOP_Y)){
      
      catch_position_x = catch_x;
      catch_position_y = catch_y;
  }

  std::vector<Position> targets;

  //逃げる駒の移動先を探す
  for(int i=0; i< 4; i++){

    Position position;
    position.x = run_position_x + RUN_DIRECTION[i][0];
    position.y = run_position_y + RUN_DIRECTION[i][1];

    if(position.x <LEFT_X || position.x > RIGHT_X){
      continue;
    }

    if(position.y < BOTTOM_Y || position.y > TOP_Y){
      continue;
    }

    if(position.x == catch_position_x && position.y == catch_position_y){
      continue;
    }
    
    position.distance = distance(position.x, position.y, catch_position_x, catch_position_y);

    targets.push_back(position);
  }

  //移動先を決定する
  switch(move_type){
    //ランダム
    case 1:
    {
        randomSeed(millis());
        int index = rand()% targets.size();  
        run_position_x = targets.at(index).x;
        run_position_y = targets.at(index).y;
        break;
    }
      
    //学習時のロジック
    default:
    {
      //最大距離の情報を見つける
      double max_distance = 0;
      for(int i=0; i<targets.size(); i++){
        if(max_distance < targets[i].distance){
          max_distance = targets[i].distance;
        }
      }

      //最大距離の一覧を取得
      std::vector<Position> maxs;
      for(int i=0; i<targets.size(); i++){
        if(targets[i].distance == max_distance){
          maxs.push_back(targets[i]);
        }
      }

      //最大距離の一覧からランダムで選択
      randomSeed(millis());
      int index = rand()% maxs.size();
      
      run_position_x = maxs.at(index).x;
      run_position_y = maxs.at(index).y;    
    }
    break;
  }
}

/**
 * 現在の環境情報を出力する
 */
State Model::state(){
  State state = State();
    
  state.run_position_x = run_position_x;
  state.run_position_y = run_position_y;
  state.catch_position_x = catch_position_x;
  state.catch_position_y = catch_position_y;
  state.distance = distance();

  return state;
}

/**
 * 現在の距離を算出する
 */
double Model::distance(){
  return distance(run_position_x, run_position_y, catch_position_x, catch_position_y);
}

/**
 * 距離を算出する
 */
double Model::distance(int x1, int y1, int x2, int y2){
  return sqrt((pow(x1 - x2, 2) + pow(y1 - y2, 2)));
}

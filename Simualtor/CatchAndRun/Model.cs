using System;
using System.Collections.Generic;
using System.Linq;

namespace CatchAndRun
{
    /// <summary>
    /// 実際のシミュレータの計算を行うクラス
    /// </summary>
    public class Model
    {
        /// <summary>
        /// 移動先の情報保持用
        /// </summary>
        private class Posiotn
        {

            /// <summary>
            /// X座標
            /// </summary>
            public int x;

            /// <summary>
            /// Y座標
            /// </summary>
            public int y;

            /// <summary>
            /// 距離
            /// </summary>
            public double distance;


            /// <summary>
            /// コンストラクタ
            /// </summary>
            /// <param name="x">X座標</param>
            /// <param name="y">Y座標</param>
            /// <param name="distance">距離</param>
            public Posiotn(int x, int y, double distance)
            {
                this.x = x;
                this.y = y;
                this.distance = distance;

            }
        }

        /// <summary>
        /// Toioプレイマットの左座標
        /// </summary>
        static readonly int LEFT_X =  -3;

        /// <summary>
        ///  Toioプレイマットの右座標
        /// </summary>
        static readonly int RIGHT_X = 3;

        /// <summary>
        ///  Toioプレイマットの上座標
        /// </summary>
        static readonly  int TOP_Y = 2;

        /// <summary>
        ///  Toioプレイマットの下座標
        /// </summary>
        static readonly int BOTTOM_Y = -2;


        /*        
        //2マス移動時の移動先
        static int[,] RUN_DIRECTION = new int[8, 2]
        {
            { -1,  1 },
            {  0,  2 },
            {  1,  1 },
            { -2,  0 },
            {  2,  0 },
            { -1, -1 },
            {  0, -2 },
            {  1,  1 }

        };*/

        /// <summary>
        /// 逃げる駒の移動先
        /// </summary>
        static int[,] RUN_DIRECTION = new int[4, 2]
        {
            {  0,  1 },
            { -1,  0 },
            {  1,  0 },
            {  0, -1 },
        };

        /// <summary>
        /// 逃げる駒のX座標
        /// </summary>
        internal int run_position_x;

        /// <summary>
        /// 逃げる駒のY座標
        /// </summary>
        internal int run_position_y;

        /// <summary>
        /// 追う駒のX座標
        /// </summary>
        internal int catch_position_x;

        /// <summary>
        /// 追う駒のY座標
        /// </summary>
        internal int catch_position_y;


        /// <summary>
        /// 距離
        /// </summary>
        public double distance { 
            get
            {
                return Distance(run_position_x , run_position_y,  catch_position_x, catch_position_y);

            } 
        }


        /// <summary>
        /// 計算結果（Bonsaiへの出力値）
        /// </summary>
        public State State
        {
            get
            {
                return new State(this);
            }
        }


        /// <summary>
        /// 停止してるかどうか
        /// </summary>
        public bool Halted
        {
            get
            {
                return  State.distance <= 1;
            }
        }


        /// <summary>
        /// コンストラクタ
        /// </summary>
        public Model()
        {
        }

        /// <summary>
        /// 状態をリセットする
        /// </summary>
        public void Reset()
        {
            run_position_x = 0;
            run_position_y = 0;
            catch_position_x = 0;
            catch_position_y = 0;
        }

        /// <summary>
        /// シミュレータ開始時
        /// </summary>
        /// <param name="config">環境設定</param>
        public void Start(Config config)
        {
            Reset();

            //駒の初期値を環境設定の情報に合わせる
            run_position_x = config.run_position_x;
            run_position_y = config.run_position_y;
            catch_position_x = config.catch_position_x;
            catch_position_y = config.catch_position_y;
        }

        /// <summary>
        /// 学習の実行
        /// </summary>
        /// <param name="action">追う駒の行動（Bonsaiからの入力値）</param>
        public void Step(Action action)
        {
            //指定方向に移動させる
            int catch_x = catch_position_x + action.catch_position_x;
            int catch_y = catch_position_y + action.catch_position_y;

            //枠から外れる場合は動かない
            if (!(catch_x < LEFT_X || catch_x > RIGHT_X) &&
                 !(catch_y < BOTTOM_Y || catch_y > TOP_Y))
            {
                    catch_position_x = catch_x;
                    catch_position_y = catch_y;
            }

            //逃げる駒の移動先を算出する
            List<Posiotn> targets = new List<Posiotn>();
            for(int i=0; i< RUN_DIRECTION.GetLength(0); i++)
            {

                int run_x = run_position_x + RUN_DIRECTION[i, 0];
                int run_y = run_position_y + RUN_DIRECTION[i, 1];

                if(run_x <LEFT_X || run_x > RIGHT_X)
                {
                    continue;
                }

                if(run_y < BOTTOM_Y || run_y > TOP_Y)
                {
                    continue;
                }

                if(run_x == catch_position_x && run_y == catch_position_y)
                {
                    continue;
                }

                targets.Add(new Posiotn(run_x, run_y, Distance(run_x, run_y, catch_position_x, catch_position_y)));

            }

            //一番距離の離れた移動先を見つける
            var maxs = targets.Where(info => info.distance == targets.Max(max_info => max_info.distance));

            //複数ある場合はランダムで選ぶ
            Random rnd = new Random();
            var run = maxs.ElementAt(rnd.Next(maxs.Count()));

            //移動先を決定
            run_position_x = run.x;
            run_position_y = run.y;

        }

        /// <summary>
        /// 2点間の距離を求める
        /// </summary>
        /// <param name="x1"></param>
        /// <param name="y1"></param>
        /// <param name="x2"></param>
        /// <param name="y2"></param>
        /// <returns></returns>
        private double Distance(int x1, int y1, int x2, int y2)
        {
            return Math.Sqrt((Math.Pow(x1 - x2, 2) + Math.Pow(y1 - y2, 2)));
        }
    }
}

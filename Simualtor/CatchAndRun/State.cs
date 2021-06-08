using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CatchAndRun
{
    /// <summary>
    /// シミュレータからの出力
    /// </summary>
    public class State
    {
        /// <summary>
        /// 逃げる駒のX座標
        /// </summary>
        public int run_position_x;
        /// <summary>
        /// 逃げる駒のY座標
        /// </summary>
        public int run_position_y;

        /// <summary>
        /// 追う駒のX座標
        /// </summary>
        public int catch_position_x;
        /// <summary>
        /// 追う駒のY座標
        /// </summary>
        public int catch_position_y;

        /// <summary>
        /// 駒の距離
        /// </summary>
        public double distance;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public State()
        {        
        }


        /// <summary>
        /// シミュレータの計算結果を格納
        /// </summary>
        /// <param name="model"></param>
        public State(Model model)
        {
            run_position_x = model.run_position_x;
            run_position_y = model.run_position_y;
            catch_position_x = model.catch_position_x;
            catch_position_y = model.catch_position_y;
            distance = model.distance;
        }
    }
}

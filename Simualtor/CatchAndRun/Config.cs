using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CatchAndRun
{
    /// <summary>
    /// 環境の設定値
    /// </summary>
    public class Config
    {
        /// <summary>
        /// 逃げる駒のX座標
        /// </summary>
        public int run_position_x = -2;
        /// <summary>
        /// 逃げる駒のY座標
        /// </summary>
        public int run_position_y = 0;

        /// <summary>
        /// 追う駒のX座標
        /// </summary>
        public int catch_position_x = 2;
        /// <summary>
        /// 追う駒のY座標
        /// </summary>
        public int catch_position_y = 0;
    }
}

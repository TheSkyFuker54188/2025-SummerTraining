var otherSnakeScore = new Array(); //得分
var time = 0; //游戏时间
var choice = new Array(); //每次游戏选择的学号数组
var gameid; //随机生成每一局id，用于后台地图命名
var functionId;
var timeFuncId;
var speed = 2; //蛇移动速度
var otherSnakeNumber; //蛇数
var bonusNumber = 68; //豆子数
var wallNumber = 32; //墙数
var boxNumber = 0;
var canvas; //画图
var context;
var falloff = 0;
var score;
var width; //画布宽度
var height; //画布长度

var TRAP_COST = 10; // cost of trap
var SHIELD_COST = 20; // cost of shield
var SHIELD_CD = 300; // cd of shield

var BLOCK_COST = 30; // cost of block
var BLOCK_RADIUS = 1; // permitted range
var BLOCK_LIFE = 10; // lifetime of block
var BLOCK_DROP_RADIUS = 2; // permitted range
var block = new Array(); // block thrown
var map_str = "";

var TIME_LIMIT = 255;

var otherSnakeX = new Array(); //蛇横坐标
var otherSnakeY = new Array(); //蛇纵坐标
var otherSnakeDirection = new Array(); //蛇方向
var otherSnakePreDirection = new Array(); //每一节蛇前一节的方向
var otherSnakeInitLength = new Array(); //每条蛇初始长度
var otherSnakeAddLength = new Array(); 	// 每条蛇的附加长度，来自额外豆子
var otherSnakeLength = new Array(); //每条蛇当前长度
var otherSnakeShieldNums = new Array(); //每条蛇拥有盾牌数
var otherSnakeShieldCDs = new Array(); //每条蛇盾牌的冷却时间
var otherSnakeShieldTimes = new Array(); //每条蛇盾牌剩余时间
var bonusX = new Array(); //豆子横坐标
var bonusY = new Array(); //豆子纵坐标
var boxX = new Array(); //尸体横坐标
var boxY = new Array(); //尸体纵坐标
var bonusValue = new Array(); //豆子种类
var refresh_times = -1; //刷新次数记录，每10次刷新调用一次后台
// var refresh_times = 0; //1.0
var map = new Array(); //地图
var error_student_id = new Array(); //出错学生的id
var snake_colors = ["red", "darkorange", "gold", "teal", "steelblue",
    "hotpink", "aqua", "goldenrod", "chocolate", "darkgreen",
    "dimgray", "dodgerblue", "rosybrown", "indigo", "lightsteelblue",
    "sandybrown", "yellowgreen", "peru", "lime", "olive",
    "fuchsia", "blueviolet", "darkblue", "lawngreen", "coral", "maroon", "lightcoral", "plum", "orangered"]; //蛇颜色
var bonus_color = ["#FF4500", "#76EE00", "#AEEEEE", "#EE82EE", /*"#FFD700",*/ "#C0C0C0"]; //豆子颜色
var box_value = new Array(); // 尸体得分
var box_color = "#3366cc"; // 尸体颜色
var bonus_value = [-1, -2, -3, -5, -100, /*-1000,*/ -10000]; //豆子得分
var game_over = false; //记录游戏结束
//计算每局对战ID
var date = new Date();
var game = date.getTime();

// const fs = require('fs');

//蛇身体移动
function othersnake_body_move(i) {  // 第i条蛇
    for (var j = 1; j < otherSnakeLength[i]; j++) {
        if (otherSnakeDirection[i][j] == 0) {  // 左
            otherSnakeX[i][j] -= speed;
        }
        else if (otherSnakeDirection[i][j] == 1) {  // 下
            otherSnakeY[i][j] -= speed;
        }
        else if (otherSnakeDirection[i][j] == 2) {  // 右
            otherSnakeX[i][j] += speed; 
        }
        else {                                      // 上
            otherSnakeY[i][j] += speed;
        }
    }
}

var snakeStop = new Array();

//蛇移动
function othersnake_move() {
    for (var i = 0; i < otherSnakeNumber; i++) {  // 第i条蛇
        //当其运动到整格点
        if (otherSnakeShieldTimes[i] > 0) {  // 盾牌时间消耗
            otherSnakeShieldTimes[i] -= 1;
        }
        if (otherSnakeShieldCDs[i] > 0) {  // 盾牌cd减少
            otherSnakeShieldCDs[i] -= 1;
        }
        if (snakeStop[i] > 0) {
            --snakeStop[i];
            continue;
        }
        if ((otherSnakeX[i][0] - 3) % 20 == 0 && (otherSnakeY[i][0] - 3) % 20 == 0) {
            //向左走
            if (otherSnakePreDirection[i][0] == 0 && otherSnakeDirection[i][0] != 2) {
                for (var j = otherSnakeLength[i] - 1; j > 0; j--) {
                    otherSnakeDirection[i][j] = otherSnakeDirection[i][j - 1];  
                }                                                               
                otherSnakeDirection[i][0] = otherSnakePreDirection[i][0];
                otherSnakeX[i][0] -= speed;
                othersnake_body_move(i);
            }
            //向上走
            else if (otherSnakePreDirection[i][0] == 1 && otherSnakeDirection[i][0] != 3) {
                for (var j = otherSnakeLength[i] - 1; j > 0; j--) {
                    otherSnakeDirection[i][j] = otherSnakeDirection[i][j - 1];
                }
                otherSnakeDirection[i][0] = otherSnakePreDirection[i][0];
                otherSnakeY[i][0] -= speed;
                othersnake_body_move(i);
            }
            //向右走
            else if (otherSnakePreDirection[i][0] == 2 && otherSnakeDirection[i][0] != 0) {
                for (var j = otherSnakeLength[i] - 1; j > 0; j--) {
                    otherSnakeDirection[i][j] = otherSnakeDirection[i][j - 1];
                }
                otherSnakeDirection[i][0] = otherSnakePreDirection[i][0];
                otherSnakeX[i][0] += speed;
                othersnake_body_move(i);
            }
            //向下走
            else if (otherSnakePreDirection[i][0] == 3 && otherSnakeDirection[i][0] != 1) {
                for (var j = otherSnakeLength[i] - 1; j > 0; j--) {
                    otherSnakeDirection[i][j] = otherSnakeDirection[i][j - 1];
                }
                otherSnakeDirection[i][0] = otherSnakePreDirection[i][0];
                otherSnakeY[i][0] += speed;
                othersnake_body_move(i);
            }
            else {
                //如果其给出了完全相反的方向，则忽略该请求，保持原方向移动
                for (var j = otherSnakeLength[i] - 1; j > 0; j--) {
                    otherSnakeDirection[i][j] = otherSnakeDirection[i][j - 1];
                }
                //向左
                if (otherSnakeDirection[i][0] == 0) {
                    otherSnakeX[i][0] -= speed;
                }
                //向上
                else if (otherSnakeDirection[i][0] == 2) {
                    otherSnakeX[i][0] += speed;
                }
                //向右
                else if (otherSnakeDirection[i][0] == 1) {
                    otherSnakeY[i][0] -= speed;
                }
                //向下
                else if (otherSnakeDirection[i][0] == 3) {
                    otherSnakeY[i][0] += speed;
                }
                othersnake_body_move(i);
            }
        }
        //不在整格点时
        else {
            if (otherSnakeDirection[i][0] == 0) {
                otherSnakeX[i][0] -= speed;
            }
            else if (otherSnakeDirection[i][0] == 2) {
                otherSnakeX[i][0] += speed;
            }
            else if (otherSnakeDirection[i][0] == 1) {
                otherSnakeY[i][0] -= speed;
            }
            else if (otherSnakeDirection[i][0] == 3) {
                otherSnakeY[i][0] += speed;
            }
            othersnake_body_move(i);
        }
    }
}

//通过判断蛇的方向增加蛇的长度
function add_snake(snakeID) {
    var finalX = Math.floor((otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] - 3) / 20);
    var finalY = Math.floor((otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] - 3) / 20);
    console.log(finalX)
    console.log(finalY)
    // 末端向左，向右增加长度
    if (otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] == 0) {
        if(finalX == 39){  // 右边出界向上下增长
            if(finalY == 29){   // 下边也出界向上增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] - 20;

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]] - 20;
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] - 20;
                
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 3;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 3;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
            else{  // 向下增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] + 20;

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]] + 20;
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] + 20;

                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 1;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 1;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
        }
        // if((finalX == 39 && finalY == 29) || (map[(finalX + 1)  * 30 + finalY] == 2 && map[finalX  * 30 + finalY+1] == 2)){  // 右边出界向上下增长
        //         otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
        //         otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] - 20;
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 3;//1.0
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 3;
        //         otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }else if((finalX == 39) || (map[(finalX + 1)  * 30 + finalY] == 2)){  // 向下增长
        //         otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
        //         otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] + 20;
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 1;//1.0
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 1;
        //         otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }
        else{  // 向右增长
            otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] + 20;
            otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        }
    }
    // 末端向上，向下增加长度
    else if (otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] == 1) {
        if(finalY == 29){  // 下边出界向左右增长
            if(finalX == 39){   // 右边也出界向左增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] - 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]] - 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]];
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] - 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1];

                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 2;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 2;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
            else{  // 向右增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] + 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]] + 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]];
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] + 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1];

                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 0;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 0;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
        }
        // if((finalX == 39 && finalY == 29) || (map[(finalX + 1)  * 30 + finalY] == 2 && map[finalX  * 30 + finalY+1] == 2)){  // 下边出界向左右增长
        //         otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] - 20;
        //         otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 2;//1.0
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 2;
        //         otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }else if(finalY == 29 || map[finalX  * 30 + finalY+1] == 2){  // 向右增长
        //         otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] + 20;
        //         otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 0;//1.0
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 0;
        //         otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }
        else{  // 向下增长
            otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] + 20;
            otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        }
    }
    // 末端向右，向左增加长度
    else if (otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] == 2) {
        if(finalX == 0){  // 左边出界向上下增长
            if(finalY == 29){   // 下边也出界向上增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] - 20;

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]] - 20;
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] - 20;              

                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 3;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 3;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
            else{  // 向下增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] + 20;

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]] + 20;
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1];
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] + 20;

                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 1;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 1;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
        }
        // if((finalX == 0 && finalY == 29) || (map[(finalX - 1)  * 30 + finalY] == 2 && map[finalX  * 30 + finalY+1] == 2)){  // 左边出界向上下增长
        //     otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
        //     otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] - 20;
        //     otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 3;//1.0
        //     otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 3;
        //     otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }else if((finalX == 0) || (map[(finalX - 1)  * 30 + finalY] == 2)){  // 向下增长
        //     otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
        //     otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] + 20;
        //     otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 1;//1.0
        //     otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 1;
        //     otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }
        else{  // 向左增长
            otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] - 20;
            otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        }
    }
    // 末端向下，向上增加长度
    else {
        if(finalY == 0){  // 上边出界向左右增长
            if(finalX == 39){   // 右边也出界向左增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] - 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]] - 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]];
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] - 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1];

                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 2;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 2;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
            else{  // 向右增长
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] + 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];

                //修正蛇分离
                otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID]] + 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID]];
                otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeX[snakeID][otherSnakeLength[snakeID]-1] + 20;
                otherSnakeY[snakeID][otherSnakeLength[snakeID]-1] = otherSnakeY[snakeID][otherSnakeLength[snakeID]-1];

                otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 0;//1.0
                otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 0;
                otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            }
        }
        // if((finalX == 39 && finalY == 0) || (map[(finalX + 1)  * 30 + finalY] == 2 && map[finalX  * 30 + finalY-1] == 2)){  // 上边出界向左右增长
        //         otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] - 20;
        //         otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 2;//1.0
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 2;
        //         otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }else if((finalY == 0) || (map[finalX  * 30 + finalY-1] == 2)){  // 向右增长
        //         otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1] + 20;
        //         otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1];
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1] = 0;//1.0
        //         otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = 0;
        //         otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        // }
        else{  // 向上增长
            otherSnakeX[snakeID][otherSnakeLength[snakeID]] = otherSnakeX[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakeY[snakeID][otherSnakeLength[snakeID]] = otherSnakeY[snakeID][otherSnakeLength[snakeID] - 1] - 20;
            otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
            otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = otherSnakeDirection[snakeID][otherSnakeLength[snakeID] - 1];
        }
    }
    otherSnakeLength[snakeID] += 1;
}

function shorten_snake(snakeID) {
    otherSnakeX[snakeID][otherSnakeLength[snakeID]] = -1;
    otherSnakeY[snakeID][otherSnakeLength[snakeID]] = -1;
    otherSnakeDirection[snakeID][otherSnakeLength[snakeID]] = -1;
    otherSnakePreDirection[snakeID][otherSnakeLength[snakeID]] = -1;
    otherSnakeLength[snakeID] -= 1;
}

//吃豆子，通过计算豆子所在的格子和蛇头所在的格子来判断豆子是否被吃掉
function eat_bonus() {
    for (var i = 0; i < bonusNumber+wallNumber; i++) {
        var bonus_x = (bonusX[i] - 13) / 20;
        var bonus_y = (bonusY[i] - 13) / 20;
        for (var j = 0; j < otherSnakeNumber; j++) {
            if (otherSnakeLength[j] != 0) {
                var o_s_x = Math.floor((otherSnakeX[j][0] - 3) / 20);
                var o_s_y = Math.floor((otherSnakeY[j][0] - 3) / 20);
                if (o_s_x == bonus_x && o_s_y == bonus_y) {  // 第i颗豆子被吃掉了
                    if (bonusValue[i] != 7){
                        bonusX[i] = -10;
                        bonusY[i] = -10;
                        map[o_s_x * 30 + o_s_y] = 0;
                    }
                    
                    if (bonusValue[i] < 4) { // 得分豆
                        otherSnakeScore[j] += Math.abs(bonus_value[bonusValue[i]]);
                    }  
                    else if (bonusValue[i] == 4) {  // 增长豆
                        otherSnakeAddLength[j] += 2;
                    } else if (bonusValue[i] == 5) {  // 陷阱豆
                        if (otherSnakeScore[j] > TRAP_COST) {
                            otherSnakeScore[j] -= TRAP_COST;
                        } else {
                            otherSnakeScore[j] = 0;
                        }
                    } 
                    // else if (bonusValue[i] == 6) {  // 盾牌豆
                    //     otherSnakeShieldNums[j] += 1;
                    // } 
                    else if (bonusValue[i] == 7) {  // 围墙
                        // snake_die(j, 'tried to crash the wall',-7);
                        snake_die(j, 'tried to crash the wall');
                        continue;
                    } 
                    else { }
                    // $("#score" + j).html( "P("+otherSnakeScore[i].toString()+") S(" + otherSnakeShieldNums[i].toString() + ") T(" + otherSnakeShieldTimes[i].toString()); /// TODO: Change to table
                    // var add_length =  Math.floor(otherSnakeScore[j] / 10);  // 判断得分是否够增长
                    var add_length =  Math.floor(otherSnakeScore[j] / 20);
                    // console.log("add_length:", add_length);
                    var change_length = otherSnakeInitLength[j] + add_length + otherSnakeAddLength[j] - otherSnakeLength[j];
                    if (change_length > 0){
                        for (var t = 0; t < change_length; t++) {
                            add_snake(j);
                        }
                    } else if (change_length < 0){
                        for (var t = 0; t < Math.abs(change_length); t++) {
                            shorten_snake(j);
                        }
                    }
                }
            }
        }
    }
}

function eat_box() {
    for (var i = 0; i < boxNumber; i++) {
        var box_x = (boxX[i] - 13) / 20;
        var box_y = (boxY[i] - 13) / 20;
        for (var j = 0; j < otherSnakeNumber; j++) {
            if (otherSnakeLength[j] != 0) {
                var o_s_x = Math.floor((otherSnakeX[j][0] - 3) / 20);
                var o_s_y = Math.floor((otherSnakeY[j][0] - 3) / 20);
                if (o_s_x == box_x && o_s_y == box_y) {
                    boxX[i] = -10;
                    boxY[i] = -10;
                    map[o_s_x * 30 + o_s_y] = 0;
                    otherSnakeScore[j] += Math.abs(box_value[i]);
                    // $("#score" + j).html( "P("+otherSnakeScore[i].toString()+") S(" + otherSnakeShieldNums[i].toString() + ") T(" + otherSnakeShieldTimes[i].toString());
                    // var add_length = otherSnakeScore[j] / 10;
                    var add_length = otherSnakeScore[j] / 20;
                    for (var t = 0; t < (otherSnakeInitLength[j] + add_length + otherSnakeAddLength[j]) - otherSnakeLength[j]; t++) {
                        add_snake(j);
                    }
                }
            }
        }
    }
}

//判断死亡，当蛇头碰到其他蛇的身体，该蛇死亡，当两蛇头相碰，均死亡
function check_death() {
    var flag = 1;
    for (var i = 0; i < otherSnakeNumber; i++) {

        if (otherSnakeLength[i] > 0 && otherSnakeShieldTimes[i] == 0) {


            var flag_other = 1;
            var my_s_x = Math.floor((otherSnakeX[i][0] - 3) / 20); //i的头
            var my_s_y = Math.floor((otherSnakeY[i][0] - 3) / 20);

            for (var j in block) { // block
                if (block[j].life <= 0)
                    continue;
                var bx = Math.floor((block[j].x - 13) / 20);
                var by = Math.floor((block[j].y - 13) / 20);
                if (Math.abs(bx - my_s_x) + Math.abs(by - my_s_y) <= BLOCK_RADIUS) {
                    snake_die(i, 'killed by block from', block[j].owner);
                    console.log(i.toString() + " Crashed into block");
                    flag_other = 0;
                    break;
                }
            }

            for (let j = 0; j < otherSnakeNumber && flag_other == 1; j++) {
                if (j == i) {
                    continue;
                }
                for (var k = 0; k < otherSnakeLength[j]; k++) { //遍历j的全身
                    var o_s_x = Math.floor((otherSnakeX[j][k] - 3) / 20);
                    var o_s_y = Math.floor((otherSnakeY[j][k] - 3) / 20);
                    if (my_s_x == o_s_x && my_s_y == o_s_y) { // 如果i碰到了j

                        if (k == 0) { // 且碰到了j的头
                            //otherSnakeLength[j] = 0;
                            if (otherSnakeShieldTimes[j] == 0) {
                                snake_die(j, 'crashed into the body of', i);
                            }
                        }
                        //otherSnakeLength[i] = 0;
                        snake_die(i, 'crashed into the body of', j);
                        flag_other = 0;
                        break;
                    }
                }
            }
        }
    }
    var count = 0;
    for (i = 0; i < otherSnakeNumber; i++) {
        if (otherSnakeLength[i] == 0) {
            count++;
        }
    }
    if (count == otherSnakeNumber) {
        game_over = true;
    }
}

function add_box(x, y, val) {
    map[x*30 + y] = 1;
    boxX[boxNumber] = x * 20 + 13;
    boxY[boxNumber] = y * 20 + 13;
    box_value[boxNumber] = val;
    boxNumber += 1;
}

function snake_die(idx, info, killer) {
    console.log(choice[idx] + " DIED");
    $('#messages').prepend(`
        <div class="ui large message">
            <p><span style="color:white; background-color: ${snake_colors[idx]} !important; padding: 0 5px;text-shadow: -0.04em -0.04em 0 #000, 0.04em -0.04em 0 #000, -0.04em 0.04em 0 #000, 0.04em 0.04em 0 #000;"><b>${idx.toString().padStart(2, '0')}</b></span>${info ? '&nbsp;' + info : ''}${typeof killer !== 'undefined' ? `&nbsp;<span style="background-color:${snake_colors[killer]}!important; color: white; padding: 0 5px;text-shadow: -0.04em -0.04em 0 #000, 0.04em -0.04em 0 #000, -0.04em 0.04em 0 #000, 0.04em 0.04em 0 #000;"><b>${killer.toString().padStart(2, '0')}</b></span>` : ''}</p>
        </div>
    `);
    var s_score = otherSnakeScore[idx];
    for (var i = 0; i < otherSnakeLength[idx]; i++) {
        if (otherSnakeX[idx][i] < 0 || otherSnakeX[idx][i] > width - 10 || otherSnakeY[idx][i] < 0 || otherSnakeY[idx][i] > height - 10 ) continue;
        if (s_score <= 0) break;
        var s_x = Math.floor((otherSnakeX[idx][i] - 3) / 20);
        var s_y = Math.floor((otherSnakeY[idx][i] - 3) / 20);
        if (s_x < 0 || s_x >= 40 || s_y < 0 || s_y >= 30 ) continue;//1.0
        if (map[s_x*30 + s_y] != 0) continue;
        // if (s_score >= 10) {
        //     add_box(s_x, s_y, 10);
        //     s_score -= 10;
        // }
        if (s_score >= 20) {
            add_box(s_x, s_y, 20);
            s_score -= 20;
        }
        else {
            add_box(s_x, s_y, s_score);
            s_score = 0;
        }
    }
    error_student_id[idx] = 1;
    otherSnakeLength[idx] = 0;
}

//判断是否撞墙
function check_wall() {
    for (var i = 0; i < otherSnakeNumber; i++) {
        if (otherSnakeLength[i] > 0) {
            if (otherSnakeX[i][0] < 0 || otherSnakeX[i][0] > width - 10 || otherSnakeY[i][0] < 0 || otherSnakeY[i][0] > height - 10) {
                snake_die(i, 'tried to explore the outside world');
            }
        }
    }
    var count = 0;
    for (i = 0; i < otherSnakeNumber; i++) {
        if (otherSnakeLength[i] == 0) {
            count++;
        }
    }
    if (count == otherSnakeNumber) {
        game_over = true;
    }
}

//计时
function set_time() {
    time += 1;
    if (time > TIME_LIMIT) {
        game_over = true;
    }
    $('#time').html(time);
}

//补充豆子，随着时间增长，豆子逐渐往中间生成
function update_bonus() {
    // update_wall(1);
    // for (var i = 0; i < bonusNumber; i++) {
    //     if (time < 80) {
    //         if (bonusX[i] == -10 || bonusY[i] == -10) {
    //             bonusX[i] = 13 + Math.floor((i * ((width - 20) / bonusNumber)) / 20) * 20;
    //             bonusY[i] = 13 + Math.floor((Math.random() * (height - 20)) / 20) * 20;
    //             random_num(i);
    //         }
    //     }
    //     if (time >= 80 && time < 120) {
    //         if (bonusX[i] == -10 || bonusY[i] == -10) {
    //             bonusX[i] = 193 + Math.floor((i * ((width - 20) / bonusNumber)) / 20) * 20;
    //             bonusY[i] = 13 + Math.floor((Math.random() * (height - 20)) / 20) * 20;
    //             random_num(i);
    //         }
    //     }
    //     if (time > 120) {
    //         if (bonusX[i] == -10 || bonusY[i] == -10) {
    //             bonusX[i] = 193 + Math.floor((i * ((width - 20) / bonusNumber)) / 20) * 20;
    //             // bonusY[i] = 93 + Math.floor((i * (400 / bonusNumber)) / 20) * 20;
    //             bonusY[i] = 93 + Math.floor((Math.random() * 400) / 20) * 20;
    //             random_num(i);
    //         }
    //     }
    // }

    //豆子逐渐向中心生成
    var centerX = 13 + width / 2;
    var centerY = 13 + height / 2;

    for (var i = 0; i < bonusNumber; i++) {
        if (bonusX[i] == -10 && bonusY[i] == -10) {
            var maxOffsetX = Math.max(100, centerX - time*1.5); // 随着时间减小生成范围
            var maxOffsetY = Math.max(100, centerY - time*1.5); // 随着时间减小生成范围

            // 确保生成的坐标在有效范围内
            var minX = Math.max(0, centerX - maxOffsetX);
            var maxX = Math.min(width-20, centerX + maxOffsetX);//1.0
            var minY = Math.max(0, centerY - maxOffsetY);
            var maxY = Math.min(height-20, centerY + maxOffsetY);//1.0

            while(true){
                var x = Math.floor((minX + (Math.random() * (maxX - minX))) / 20) * 20;
                var y = Math.floor((minY + (Math.random() * (maxY - minY))) / 20) * 20;
                if(map[x/20 * 30 + y/20] == 0) {
                    // 生成新的豆子位置
                    bonusX[i] = 13+x;
                    bonusY[i] = 13+y;
                    random_num(i);
                    map[x/20 * 30 + y/20] = 1;
                    break;
                }
            }
        }
    }
    // update_wall();
}

//生成围墙
function update_wall(flag){
    // var centerX = 13 + Math.floor(width / 2 /20)*20; // 中心的X坐标
    // var centerY = 13 + Math.floor(height / 2/20)*20; // 中心的Y坐标
    // // console.log(centerX);
    // console.log(flag);
    // var sideLength = 240; // 围墙的边长
    // var halfSide = sideLength / 2;
    // // 点间距
    // var pointDistance = 20;
    
    // // 用于存储边上所有点的数组
    // var i = bonusNumber;
    
    // // 上边，从左到右
    // for (var x = centerX - halfSide; x < centerX + halfSide; x += pointDistance) {
    //     if(x == centerX) continue;
    //     if(flag == 1 &&  bonusX[i] != -10) {
    //         i++;
    //         continue;
    //     }
    //     bonusX[i] = x;
    //     bonusY[i] = centerY - halfSide;
    //     bonusValue[i] = 7;
    //     map[(bonusX[i]-13)/20 * 30 + (bonusY[i]-13)/20] = 1;
    //     i++;
    // }
    
    // // 右边，从上到下
    // for (var y = centerY - halfSide; y < centerY + halfSide; y += pointDistance) {
    //     // points.push({ x: centerX + halfSide, y: y });
    //     if(y == centerY) continue;
    //     if(flag == 1 &&  bonusX[i] != -10) {
    //         i++;
    //         continue;
    //     }
    //     bonusX[i] = centerX + halfSide;
    //     bonusY[i] = y;
    //     bonusValue[i] = 7;
    //     map[(bonusX[i]-13)/20 * 30 + (bonusY[i]-13)/20] = 1;
    //     i++;
    // }
    
    // // 下边，从右到左
    // for (var x = centerX + halfSide; x > centerX - halfSide; x -= pointDistance) {
    //     // points.push({ x: x, y: centerY + halfSide });
    //     if(x == centerX) continue;
    //     if(flag == 1 &&  bonusX[i] != -10) {
    //         i++;
    //         continue;
    //     }
    //     bonusX[i] = x;
    //     bonusY[i] = centerY + halfSide;
    //     bonusValue[i] = 7;
    //     map[(bonusX[i]-13)/20 * 30 + (bonusY[i]-13)/20] = 1;
    //     i++;
    // }
    
    // // 左边，从下到上
    // for (var y = centerY + halfSide; y > centerY - halfSide; y -= pointDistance) {
    //     // points.push({ x: centerX - halfSide, y: y });
    //     if(y == centerY) continue;
    //     if(flag == 1 &&  bonusX[i] != -10) {
    //         i++;
    //         continue;
    //     }
    //     bonusX[i] = centerX - halfSide;
    //     bonusY[i] = y;
    //     bonusValue[i] = 7;
    //     map[(bonusX[i]-13)/20 * 30 + (bonusY[i]-13)/20] = 1;
    //     i++;
    // }
    var centerX = 13 + Math.floor(width / 2 / 20) * 20; // 中心的X坐标
    var centerY = 13 + Math.floor(height / 2 / 20) * 20; // 中心的Y坐标
    console.log(flag);
    var radius = 120; // 围墙的半径
    var pointDistance = 20; // 点间距
    
    // 用于存储边上所有点的数组
    var i = bonusNumber;
    var circumference = 2 * Math.PI * radius;
    var totalPoints = Math.floor(circumference / pointDistance / 2) * 2; // 确保总点数为偶数
    var angleIncrement = 360 / totalPoints;
    
    // 计算圆周上的点
    for (var j = 0; j < totalPoints; j++) {
        var angle = j * angleIncrement;
        var radians = angle * (Math.PI / 180);
        var x = centerX + radius * Math.cos(radians);
        var y = centerY + radius * Math.sin(radians);
    
        // 将x和y四舍五入到最近的20的倍数
        x = Math.round((x - 13) / 20) * 20 + 13;
        y = Math.round((y - 13) / 20) * 20 + 13;
    
        // 排除中心点以及开口位置的点
        if ((x == centerX && y == centerY) || 
        ((Math.abs(x - centerX) < pointDistance || Math.abs(y - centerY) < pointDistance))){
            continue;
        }
    
        if (flag == 1 && bonusX[i] != -10) {
            i++;
            continue;
        }
    
        bonusX[i] = x;
        bonusY[i] = y;
        bonusValue[i] = 7;
        map[(bonusX[i] - 13) / 20 * 30 + (bonusY[i] - 13) / 20] = 2;
        i++;
    }
    
    // print(i)
    console.log(centerX);
    console.log(centerY);
}

//随机生成豆子颜色及分值
function random_num(bonusID) {

    var num = Math.random();

    w1 = parseFloat($("#col1").val())
    w2 = parseFloat($("#col2").val())
    w3 = parseFloat($("#col3").val())
    w4 = parseFloat($("#col4").val())
    w5 = parseFloat($("#col5").val())
    w6 = parseFloat($("#col6").val())
    // if(  Math.abs(w1 + w2 + w3 + w4 + w5 + w6 - 1)  )
    if (Math.abs(w1 + w2 + w3 + w4 + w5 + w6 - 1) > 1e-3) {
        alert("豆子权重和不等于1，非法！")
        return
    }
    num1 = w1
    num2 = num1 + w2
    num3 = num2 + w3
    num4 = num3 + w4
    num5 = num4 + w5

    if (num <= num1) {
        bonusValue[bonusID] = 0;
    }
    else if (num > num1 && num <= num2) {
        bonusValue[bonusID] = 1;
    }
    else if (num > num2 && num <= num3) {
        bonusValue[bonusID] = 2;
    }
    else if (num > num3 && num <= num4) {
        bonusValue[bonusID] = 3;
    }
    else if (num > num4 && num <= num5) {
        bonusValue[bonusID] = 4;
    } else {
        bonusValue[bonusID] = 5;
    }
    // if(num <= 0.1) {
    // 	bonusValue[bonusID] = 0;
    // }else{
    // 	bonusValue[bonusID] = 5;
    // }
}

//开始
function btn_begin() {
    game_over = false;
    time = 0;
    //助教的变态蛇长度为25
    for (var i = 0; i < otherSnakeNumber; i++) {
        if (choice[i] == "1015202401" || choice[i] == "1015202402" || choice[i] == "1015202403") {
            otherSnakeInitLength[i] = 25;
        }
        else {
            otherSnakeInitLength[i] = 5;
        }
    }
    //变量初始化
    for (var i = 0; i < otherSnakeNumber; i++) {
        otherSnakeLength[i] = otherSnakeInitLength[i];
        otherSnakeScore[i] = 0;
        otherSnakeAddLength[i] = 0;
        error_student_id[i] = 0;
        otherSnakeShieldNums[i] = 5;
        otherSnakeShieldCDs[i] = 0;
        otherSnakeShieldTimes[i] = 100;
    }
    for (var i = 0; i < otherSnakeNumber; i++) {
        otherSnakeScore[i] = 0;
        if (choice[i] == "2016202202") {
            var basicX = 723;
            var basicY = 203;
            var basicDir = 0;
            for (var j = 0; j < otherSnakeLength[i]; j++) {
                otherSnakeX[i][j] = basicX + j * 20;
                otherSnakeY[i][j] = basicY;
                otherSnakeDirection[i][j] = basicDir;
                otherSnakePreDirection[i][j] = basicDir;
            }
        }
        else if (choice[i] == "admin") {
            var basicX = 83;
            var basicY = 203;
            var basicDir = 2;
            for (var j = 0; j < otherSnakeLength[i]; j++) {
                otherSnakeX[i][j] = basicX - j * 20;
                otherSnakeY[i][j] = basicY;
                otherSnakeDirection[i][j] = basicDir;
                otherSnakePreDirection[i][j] = basicDir;
            }
        }
        else {
            //计算出生点
            // var g = 0;
            // var l = 0;
            // var nn = 13;
            // if (otherSnakeNumber <= 13) {
            //     g = (Math.floor((720 / otherSnakeNumber) / 20)) * 20;
            // }
            // else {
            //     nn = Math.floor(otherSnakeNumber / 2);
            //     g = (Math.floor((720 / nn) / 20)) * 20;
            //     l = (Math.floor((720 / (otherSnakeNumber - nn)) / 20)) * 20;
            //     // console.log(g);
            //     // console.log(l);
            // }
            // var basicX = 0;
            // var basicY = 0;
            // var basicDir = 1;
            // if (i <= nn) {
            //     if (i % 2 == 0) {
            //         basicX = 43 + i * g;
            //         basicY = 123;
            //         basicDir = 1;
            //     }
            //     else {
            //         basicX = 43 + i * g;
            //         basicY = 283;
            //         basicDir = 3;
            //     }
            // }
            // else {
            //     if (i % 2 == 0) {
            //         basicX = 43 + (i - nn) * l;
            //         basicY = 343;
            //         basicDir = 1;
            //     }
            //     else {
            //         basicX = 43 + (i - nn) * l;
            //         basicY = 503;
            //         basicDir = 3;
            //     }
            // }
            // //根据出生点生成蛇
            // for (var j = 0; j < otherSnakeLength[i]; j++) {
            //     if (basicDir == 0) {
            //         otherSnakeX[i][j] = basicX + j * 20;
            //         otherSnakeY[i][j] = basicY;
            //     }
            //     else if (basicDir == 1) {
            //         otherSnakeX[i][j] = basicX;
            //         otherSnakeY[i][j] = basicY + j * 20;
            //     }
            //     else if (basicDir == 2) {
            //         otherSnakeX[i][j] = basicX - j * 20;
            //         otherSnakeY[i][j] = basicY;
            //     }
            //     else {
            //         otherSnakeX[i][j] = basicX;
            //         otherSnakeY[i][j] = basicY - j * 20;
            //     }
            //     otherSnakeDirection[i][j] = basicDir;
            //     otherSnakePreDirection[i][j] = basicDir;
            // }

            //生成蛇的初始位置围成一个圈
            var centerX = 403; // 圆心的X坐标
            var centerY = 303; // 圆心的Y坐标
            var radius = 200; // 圆的半径
            
            // 计算每条蛇的初始角度
            var angleStep = 360 / otherSnakeNumber;
            // 生成一个包含索引的数组
            var indices = [];
            for (var i = 0; i < otherSnakeNumber; i++) {
                indices.push(i);
            }

            // 对索引数组进行随机排序
            indices.sort(function() { return 0.5 - Math.random(); });

            // 按照随机顺序生成蛇
            for (var k = 0; k < otherSnakeNumber; k++) {
                var i = indices[k];
                var theta = k * angleStep * Math.PI / 180; // 将角度转换为弧度
            
                // 初始位置
                var basicX = Math.round((centerX + radius * Math.cos(theta)) / 20) * 20+3;
                var basicY = Math.round((centerY + radius * Math.sin(theta)) / 20) * 20+3;
            
                // 根据位置设置初始方向
                var basicDir = 0;
                if (theta >= 0 && theta < Math.PI / 2) {
                    basicDir = 0; // 左
                } else if (theta >= Math.PI / 2 && theta < Math.PI) {
                    basicDir = 1; // 上
                } else if (theta >= Math.PI && theta < 3 * Math.PI / 2) {
                    basicDir = 2; // 右
                } else {
                    basicDir = 3; // 下
                }
            
                // 根据出生点生成蛇
                for (var j = 0; j < otherSnakeLength[i]; j++) {
                    if (basicDir == 0) {
                        otherSnakeX[i][j] = basicX + j * 20;
                        otherSnakeY[i][j] = basicY;
                    } else if (basicDir == 1) {
                        otherSnakeX[i][j] = basicX;
                        otherSnakeY[i][j] = basicY + j * 20;
                    } else if (basicDir == 2) {
                        otherSnakeX[i][j] = basicX - j * 20;
                        otherSnakeY[i][j] = basicY;
                    } else {
                        otherSnakeX[i][j] = basicX;
                        otherSnakeY[i][j] = basicY - j * 20;
                    }
                    otherSnakeDirection[i][j] = basicDir;
                    otherSnakePreDirection[i][j] = basicDir;
                }
            }
            
        }
    }
    canvas = document.getElementById("canvas");
    width = canvas.width;
    height = canvas.height;
    context = canvas.getContext('2d');
    //初始化地图
    for (var i = 0; i < 40; i++) {
        for (var j = 0; j < 30; j++) {
            map[i * 30 + j] = 0;
        }
    }
    //初始化豆子
    update_wall(0);
    for (var i = 0; i < bonusNumber; i++) {
        while(true){
            var x = Math.floor((i * ((width - 20) / bonusNumber)) / 20) * 20;
            var y = Math.floor((Math.random() * (height - 20)) / 20) * 20;
            // console.log(x/20);
            // console.log(y/20);
            if(map[x/20 * 30 + y/20] == 0) {
                bonusX[i] = 13 + x;
                bonusY[i] = 13 + y;
                random_num(i);
                map[x/20 * 30 + y/20] = 1;
                break;
            }
        }
    }
    $('#score').html(score);
    $('#time').html(time);
    var colorstring = '';
    //初始化图例
    for (var i = 0; i < otherSnakeNumber; i++) {
        colorstring += '<tr>';
        colorstring += "<td style=\"background-color:" + snake_colors[i] + "!important; text-shadow: 0 0 10px white;\"><b style=\"background-color: white;\">" + i.toString().padStart(2, '0') + '::' + choice[i].toString() + "</b></div>";
        if (otherSnakeLength[i] === 0)
            colorstring += "<td id=\"score" + i + "\"><b>" + otherSnakeScore[i].toString() + "</b></td>";
        else
            colorstring += "<td id=\"score" + i + "\">" + otherSnakeScore[i].toString() + "</td>";
        // colorstring += "<td id=\"shield" + i + "\">" + otherSnakeShieldNums[i].toString() + "</td>";
        colorstring += "<td id=\"shieldCD" + i + "\">" + Math.floor(otherSnakeShieldCDs[i] / 10).toString() + "</td>";
        colorstring += "<td id=\"shieldlife" + i + "\">" + Math.floor(otherSnakeShieldTimes[i] / 10).toString() + "</td>";
        colorstring += '</tr>';
    }
    $('#note').html(colorstring);
    draw();
    ++refresh_times;
    //刷新画图，50ms
}

function transform_to_map() { // 发给用户
    map_str = (TIME_LIMIT - time + 1).toString() + '\n';

    bonus_array = [];
    for (var i = 0; i < bonusNumber+wallNumber; i++) { //豆子
        if (bonusX[i] == -10 || bonusY[i] == -10) {
            continue;
        }
        var bonus_y = (bonusX[i] - 13) / 20;
        var bonus_x = (bonusY[i] - 13) / 20;
        if (bonusValue[i] < 4)
            bonus_array.push({ x: bonus_x, y: bonus_y, value: Math.abs(bonus_value[bonusValue[i]]) });
        else
            bonus_array.push({ x: bonus_x, y: bonus_y, value: -(bonusValue[i] - 3) });
            
        // map[bonus_y * 40 + bonus_x] = bonusValue[i] + 2; // 4 -> 6; 5 -> 7
    }
    for (var i = 0; i < boxNumber; i++) { //尸体
        if (boxX[i] == -10 || boxY[i] == -10) {
            continue;
        }
        var box_y = (boxX[i] - 13) / 20;
        var box_x = (boxY[i] - 13) / 20;
        bonus_array.push({ x: box_x, y: box_y, value: Math.abs(box_value[i]) });
    }
    map_str += bonus_array.length + '\n';
    for (let bonus of bonus_array) {
        map_str += '' + bonus.x + ' ' + bonus.y + ' ' + bonus.value + '\n';
    }

    // block_array = []
    // for (var i in block) {
    //     if (block[i].x == -10 || block[i].y == -10 || block[i].life <= 0) {
    //         continue;
    //     }
    //     var blk_y = (block[i].x - 13) / 20;
    //     var blk_x = (block[i].y - 13) / 20;
    //     block_array.push({ x: blk_x, y: blk_y, rad: BLOCK_RADIUS, life: Math.floor(block[i].life / 10) });
    // }
    // map_str += block_array.length + '\n';
    // for (let block of block_array) {
    //     map_str += '' + block.x + ' ' + block.y + ' ' + block.rad + ' ' + block.life + '\n';
    // }

    map_str += otherSnakeLength.filter(x => x > 0).length + '\n';
    for (var i = 0; i < otherSnakeNumber; i++) { //蛇
        if (otherSnakeLength[i] > 0) {
            map_str += choice[i] + ' '
                + otherSnakeLength[i] + ' '
                + otherSnakeScore[i] + ' '
                + otherSnakeDirection[i][0] + ' '
                // + otherSnakeShieldNums[i] + ' '
                + Math.floor(otherSnakeShieldCDs[i] / 10) + ' '
                + Math.floor(otherSnakeShieldTimes[i] / 10) + '\n';

            for (var j = 0; j < otherSnakeLength[i]; j++) {
                var o_s_y = Math.floor((otherSnakeX[i][j] - 3) / 20);
                var o_s_x = Math.floor((otherSnakeY[i][j] - 3) / 20);
                map_str += o_s_x + ' ' + o_s_y + '\n';
            }
        }
    }
    // fs.writeFile('map.txt', map_str, (err) => {
    //     if (err) {
    //         console.error('Error writing to file', err);
    //     } else {
    //         console.log('File has been saved');
    //     }
    // });
}

var gamestart = false;

//画图
async function draw() {
    if (game_over)
        console.log("game_over = true");
    if (game_over && falloff == 2) {
        let rank = [];
        for (let i = 0; i < otherSnakeNumber; i++)
            rank.push(i);
        rank.sort((a, b) => otherSnakeScore[b] - otherSnakeScore[a]);
        let finalScore = Array(otherSnakeNumber).fill(0);
        finalScore[rank[0]] = 8;
        for (let i = 1; i < otherSnakeNumber; i++)
            finalScore[rank[i]] = otherSnakeScore[rank[i]] === otherSnakeScore[rank[i - 1]] ? finalScore[rank[i - 1]] : Math.max(8 - i, 0);
        let finalArray = Array.from(finalScore, (v, i) => `${choice[i]} ${otherSnakeScore[i]}\t${v}`).sort();
        console.log(finalArray.map(v => v.split(' ')[0]).join('\n'));
        console.log(finalArray.map(v => v.split(' ')[1].split('\t')[0]).join('\n'));
        console.log(finalArray.map(v => v.split(' ')[1].split('\t')[1]).join('\n'));
        console.log(finalArray)
        $('#messages').prepend(`
            <div class="ui positive large message">
                <div class="header">Game Over!</div>
            </div>
        `);
    }
    else {
        if (game_over) {
            falloff++;
        }
        //画桌布
        context.clearRect(0, 0, width, height);
        context.save();
        context.fillStyle = "#262626";
        context.strokeStyle = "#4682B4";
        context.fillRect(3, 3, width - 3, height - 3);
        context.lineWidth = 3;
        context.strokeRect(0, 0, width, height);
        context.lineWidth = 1;
        //画格子
        for (var i = 0; i < 30; i++) {
            context.moveTo(3, 3 + i * 20);
            context.lineTo(803, 3 + i * 20);
        }
        for (var i = 0; i < 40; i++) {
            context.moveTo(3 + i * 20, 3);
            context.lineTo(3 + i * 20, 603);
        }
        context.strokeStyle = "#FFFFFF";
        context.stroke();
        //画豆子
        for (var i = 0; i < bonusNumber; i++) {

            if (bonusValue[i] == 5) {
                var img = document.getElementById("trap")
                context.drawImage(img, bonusX[i] - 10, bonusY[i] - 10, 20, 20)
                continue;
            }

            if (bonusValue[i] == 4) {
                var img = document.getElementById("douzi")
                context.drawImage(img, bonusX[i] - 10, bonusY[i] - 10, 20, 20)
                continue;
            }

            context.beginPath();
            context.fillStyle = bonus_color[bonusValue[i]];
            context.arc(bonusX[i], bonusY[i], 10, 0, Math.PI * 2, false);
            context.closePath();
            context.fill();
        }
        //画围墙
        for (var i = bonusNumber; i < bonusNumber + wallNumber; i++) {
            var img = document.getElementById("wall")
            context.drawImage(img, bonusX[i] - 10, bonusY[i] - 10, 20, 20)
        }
        // draw box
        for (var i = 0; i < boxNumber; i++) {
            context.beginPath();
            context.fillStyle = box_color;
            context.arc(boxX[i], boxY[i], 10, 0, Math.PI * 2, false);
            context.closePath();
            context.fill();
        }
        //画蛇
        context.globalAlpha = 0.8;
        for (var i = 0; i < otherSnakeNumber; i++) {
            for (var j = otherSnakeLength[i] - 1; j >= 0; j--) {
                context.fillStyle = snake_colors[i];
                context.fillRect(otherSnakeX[i][j], otherSnakeY[i][j], 20, 20);
                if (j == 0 && otherSnakeShieldTimes[i] == 0) {
                    context.strokeStyle = "#C0FF3E";
                    context.lineWidth = 3;
                    context.strokeRect(otherSnakeX[i][j] + 1, otherSnakeY[i][j] + 1, 17, 17);
                } else if (j == 0 && otherSnakeShieldTimes[i] > 0) {
                    context.strokeStyle = "#4285f4";
                    context.lineWidth = 3;
                    context.strokeRect(otherSnakeX[i][j] + 1, otherSnakeY[i][j] + 1, 17, 17);
                }
                if (j === 0) {
                    let dump = context.globalAlpha;
                    context.globalAlpha = 1;
                    context.fillStyle = 'white';
                    context.font = '15px Consolas, monospace';
                    context.fillText(i.toString().padStart(2, '0'), otherSnakeX[i][j] + 2, otherSnakeY[i][j] + 15);
                    context.globalAlpha = dump;
                }
            }
        }
        context.globalAlpha = 1;
        // draw block
        for (var i in block) {
            if (block[i].life > 0) {
                context.beginPath();
                context.fillStyle = snake_colors[block[i].owner];
                context.arc(block[i].x, block[i].y, 10, 0, Math.PI * 2, false);
                context.fill();
                var blk_x = (block[i].x - 13) / 20;
                var blk_y = (block[i].y - 13) / 20;
                for (var dx = -BLOCK_RADIUS; dx <= BLOCK_RADIUS; ++dx) {
                    for (var dy = -BLOCK_RADIUS; dy <= BLOCK_RADIUS; ++dy) {
                        if (Math.abs(dx) + Math.abs(dy) <= BLOCK_RADIUS) {
                            if (blk_x + dx >= 40 || blk_x + dx < 0)
                                continue;
                            if (blk_y + dy >= 30 || blk_y + dy < 0)
                                continue;
                            context.beginPath();
                            context.fillStyle = snake_colors[block[i].owner];
                            context.arc(block[i].x + dx * 20, block[i].y + dy * 20, 10, 0, Math.PI * 2, false);
                            context.fill();
                        }
                    }
                }
            }
        }

        var colorstring = "";
        //初始化图例
        var rank = new Array();
        for (var i = 0; i < otherSnakeNumber; i++)
            rank.push(i);
        rank.sort(function (a, b) { return otherSnakeScore[b] - otherSnakeScore[a]; });
        for (var i = 0; i < otherSnakeNumber; i++) {
            colorstring += '<tr>';
            colorstring += "<td style=\"background-color:" + snake_colors[rank[i]] + "!important;\"><b style=\"background-color: white;\">" + rank[i].toString().padStart(2, '0') + '::' + choice[rank[i]].toString() + "</b></div>";
            if (otherSnakeLength[rank[i]] === 0)
                colorstring += "<td id=\"score" + rank[i] + "\" style=\"color:red;\"><b>" + otherSnakeScore[rank[i]].toString() + "</b></td>";
            else
                colorstring += "<td id=\"score" + rank[i] + "\">" + otherSnakeScore[rank[i]].toString() + "</td>";
            // colorstring += "<td id=\"shield" + rank[i] + "\">" + otherSnakeShieldNums[rank[i]].toString() + "</td>";
            colorstring += "<td id=\"shieldCD" + rank[i] + "\">" + Math.floor(otherSnakeShieldCDs[rank[i]] / 10).toString() + "</td>";
            colorstring += "<td id=\"shieldlife" + rank[i] + "\">" + Math.floor(otherSnakeShieldTimes[rank[i]] / 10).toString() + "</td>";
            colorstring += '</tr>';
        }
        $('#note').html(colorstring);
        // console.log(refresh_times)
        //如果刷新了10次，向后台请求数据，由于格子大小为20，每次移动2，10次正好走满一格
        if (refresh_times % 10 == 0 ) {
            set_time();
            //只在整格点判断蛇是否相碰，否则可能由于一点点位置变化导致判断错误
            check_death();
            check_wall();
            refresh_times = 0;

            transform_to_map();
            console.log("--------------------------------")
            console.log("本次的输入为:")
            console.log(map_str);
            let target = [];
            for (let idx in choice) {
                if (error_student_id[idx] === 0) {
                    target.push(choice[idx]);
                }
            }
            let response = await fetch('/api/exec', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ uid: target, input: map_str })
            })
                .then(res => res.json());
            if (!response.success) {
                alert(`FATAL: ${response.error}`);
                return;
            }
            let data = response.data;
            console.log(JSON.stringify(data, null, 2));
            for (result of data) {
                let sid = result.sid;
                var loc = $.inArray(sid, choice);
                if (result.error) {
                    snake_die(loc, 'failed making decision');
                    continue;
                }
                if (result.result && result.result.status !== 1) {
                    if (result.result.status !== 2 || result.result.time > 200000000) {
                        let sandboxStatus = ["Unknown", "OK", "TimeLimitExceeded", "MemoryLimitExceeded", "RuntimeError", "Cancelled", "OutputLimitExceeded"];
                        console.log('SID ' + sid + ' : ' + sandboxStatus[result.result.status]);
                        snake_die(loc, 'failed making decision');
                        continue;
                    }
                }
                // 处理用户输入
                var oplist = result.output.split(' ');  
                if (loc != -1) {
                    if (oplist.length == 1) {
                        var dir = parseInt(result.output);
                        if (dir !== 0 && dir !== 1 && dir !== 2 && dir !== 3 && dir !== 4) {
                            snake_die(loc, 'made an illegal operation');
                        }
                        else {
                            otherSnakePreDirection[loc][0] = dir;
                            if (dir == 4) {  // use shield
                                // if (otherSnakeShieldNums[loc] > 0) {
                                //     --otherSnakeShieldNums[loc];
                                //     otherSnakeShieldTimes[loc] = Math.max(50 + 10, otherSnakeShieldTimes[loc]);
                                //     snakeStop[loc] = 10;
                                // }
                                if (otherSnakeShieldCDs[loc] == 0 && otherSnakeScore[loc] >= SHIELD_COST) {//1.0
                                    // --otherSnakeShieldNums[loc];
                                    otherSnakeShieldCDs[loc] = SHIELD_CD;
                                    otherSnakeShieldTimes[loc] = Math.max(50 + 10, otherSnakeShieldTimes[loc]);
                                    snakeStop[loc] = 10;
                                    otherSnakeScore[loc] -= SHIELD_COST;
                                }
                            }
                        }
                    }else if (oplist.length >= 2){
                        console.log(result.output)
                        var dir = parseInt(oplist[0]);
                        if (dir !== 0 && dir !== 1 && dir !== 2 && dir !== 3 && dir !== 4) {
                            snake_die(loc, 'made an illegal operation');
                        }
                        else {
                            otherSnakePreDirection[loc][0] = dir;
                            if (dir == 4) {  // use shield
                                // if (otherSnakeShieldNums[loc] > 0) {
                                //     --otherSnakeShieldNums[loc];
                                //     otherSnakeShieldTimes[loc] = Math.max(50 + 10, otherSnakeShieldTimes[loc]);
                                //     snakeStop[loc] = 10;
                                // }
                                if (otherSnakeShieldCDs[loc] == 0 && otherSnakeScore[loc] >= SHIELD_COST) {//1.0
                                    // --otherSnakeShieldNums[loc];
                                    otherSnakeShieldCDs[loc] = SHIELD_CD;
                                    otherSnakeShieldTimes[loc] = Math.max(50 + 10, otherSnakeShieldTimes[loc]);
                                    snakeStop[loc] = 10;
                                    otherSnakeScore[loc] -= SHIELD_COST;
                                }
                            }
                        }
                    }
                    // else {  // 障碍物部分
                    //     if (oplist.length != 3 || parseInt(oplist[0]) != 5) {
                    //         snake_die(loc, 'made an illegal operation');
                    //     }
                    //     else {
                    //         var dx = parseInt(oplist[1]);
                    //         var dy = parseInt(oplist[2]);
                    //         otherSnakePreDirection[loc][0] = 5;
                    //         if (otherSnakeScore[loc] >= BLOCK_COST) {
                    //             snakeStop[loc] = 10;
                    //             otherSnakeScore[loc] -= BLOCK_COST;
                    //             otherSnakeShieldTimes[loc] = Math.max(20 + 10, otherSnakeShieldTimes[loc]);
                    //             if (Math.abs(dx) + Math.abs(dy) <= BLOCK_DROP_RADIUS) {
                    //                 var pos = { x: otherSnakeX[loc][0] + 10 + dy * 20, y: otherSnakeY[loc][0] + 10 + dx * 20, life: BLOCK_LIFE * 10, owner: loc };
                    //                 block.push(pos);
                    //             }
                    //         }
                    //     }
                    // }
                }
            }
        }
        for (var i in block) {
            if (block[i].life > 0)
                --block[i].life;
        }
        othersnake_move();
        eat_bonus();
        eat_box();
        if (time % 10 == 0) {
            update_bonus();
        }
        refresh_times++;
        if (gamestart)
            setTimeout(draw, 30);
    }
}

function shuffle(array) {
    var currentIndex = array.length, randomIndex;

    // While there remain elements to shuffle...
    while (0 !== currentIndex) {

        // Pick a remaining element...
        randomIndex = Math.floor(Math.random() * currentIndex);
        currentIndex--;

        // And swap it with the current element.
        [array[currentIndex], array[randomIndex]] = [
            array[randomIndex], array[currentIndex]];
    }

    return array;
}

//判断选择了多少选手，并记录学号
function checkChoice() {
    var checklist = $(".checkbox").checkbox('is checked');
    var choice_num = 0;
    console.log(checklist.length)
    for (var i = 0; i < checklist.length; i++) {
        if (checklist[i] == true) {
            choice_num++;
            choice.push($(".checkbox").children()[i * 2].name);
        }
    }
    console.log(choice_num)

    // const username = document.getElementById('username');
    // const username = usernameElement.textContent.trim();
    // console.log('Username:', username);

    // shuffle(choice);
    if (choice_num > 29) {
        alert("你最多只能选29个选手呃...");
    }
    // if (choice_num > 10) {
    //     alert("你最多只能选10个选手呃...");
    // }
    else if (choice_num < 1) {
        alert("你总得选点什么 ...");
    }
    if (choice_num <= 29) {
    // if (choice_num <= 10) {
        otherSnakeNumber = choice_num;
        for (var i = 0; i < otherSnakeNumber; i++) {
            otherSnakeX[i] = new Array();
            otherSnakeY[i] = new Array();
            otherSnakeDirection[i] = new Array();
            otherSnakePreDirection[i] = new Array();
        }
        btn_begin();
        $("#add_stu").addClass("disabled");
        $("#confirm").addClass("disabled");
        $("#start").removeClass("disabled");
    }
}

function startGame() {
    gamestart = true;
    $("#start").addClass("disabled");
    draw();
}

function addStus() {
    snos_str = $("#snos").val()
    snos = snos_str.split(" ")
    // console.log(snos)
    for (var i = 0; i < snos.length; i++) {
        $(":checkbox[name='" + snos[i] + "']").prop("checked", true);
    }
}
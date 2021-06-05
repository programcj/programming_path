/***
* 自定义区域绘图与编辑 + 图片显示 
* 另外称呼：电子围栏
* ES5
* 注意： 不能使用 style 中的 width, height来定义 <canvas> 标签
*/
class jCVS {
    //实例属性
    constructor(options) {
        this.color = [
            '#FFFFFF', //白
            '#000000', //黑，
            '#FF0000',  //红色
            '#FF7F00', //橙色
            '#FFFF00', //黄色
            '#00FF00', //绿色
            '#00FFFF', //青色
            '#0000FF', //蓝色
            '#8B00FF' //紫色
        ];
        this.canvas = options.canvas;
        this.ctx = options.canvas.getContext('2d');
        this.callbk_AddRectIndex = options.callfun_AddRectIndex;
        this.callfun_DelRectIndex = options.callfun_DelRectIndex;

        if (options.hasOwnProperty("flag_rectfill")) {
            this.flag_rectfill = options.flag_rectfill;
        }

        this.allrects = [ // 电子围栏\人脸坐标
            {
                name: "测试",
                points: [{ x: 10, y: 10 }, { x: 20, y: 10 }, { x: 20, y: 50 }, { x: 10, y: 50 }],
            },
            {
                name: "测试", //标题
                //当前是固定的点,基于输入的canvas的宽高中的像素点
                points: [{ x: 10, y: 10 }, { x: 20, y: 10 }, { x: 20, y: 50 }, { x: 10, y: 50 }], //边线的点
                strokeStyle: '#0000FF',  //边线颜色
                fillStyle: 'rgba(192, 80, 77, 0.7)', //填充颜色
            }
        ];
        this.curr_point = null; //当前点
        this.curr_rects_index = -1; //当前编辑区域索引
        this.mouse_point = null;  //当前鼠标位置
        this.flag_draw_create = false;  //是否处于创建点
        this.bkimg = null; //背景图片
        this.flag_grabbing = false; //拖拽
        this.enable_draw_flag = true;
        this.Initial();
    }

    //----Y坐标点装换， 防止绘制到图片外
    YPointReplace = (y) => {
        if (y < this.y) {
            y = this.y
        }
        else if (y > this.iHeight * this.scale + this.y) {
            y = this.iHeight * this.scale + this.y
        }
        return y
    };

    //----X坐标点装换， 防止绘制到图片外
    XPointReplace = (x) => {
        if (x < this.x) {
            x = this.x
        }
        else if (x > this.iWidth * this.scale + this.x) {
            x = this.iWidth * this.scale + this.x
        }
        return x
    };

    //----获取更新鼠标在当前展示画板中的位置
    GetMouseInCanvasLocation = (e) => {
        this.mouseX = this.XPointReplace(e.layerX || e.offsetX);
        this.mouseY = this.YPointReplace(e.layerY || e.offsetY);
    };

    Initial = () => {
        let _cvs = this;

        this.canvas.onmousedown = function (evt) {
            //events.button==0  鼠标左键
            //events.button==1  鼠标中键 
            //events.button==2  鼠标右键
            if (_cvs.enable_draw_flag == false)
                return;

            if (evt.button == 0) {
                _cvs.GetMouseInCanvasLocation(evt);
                var x = _cvs.mouseX;
                var y = _cvs.mouseY;
                //var x = evt.pageX - this.offsetLeft;
                // var y = evt.pageY - this.offsetTop;

                if (_cvs.flag_draw_create == false)  //开始绘制标志 或判断去修改
                {
                    if (_cvs.curr_rects_index >= 0 && _cvs.curr_rects_index < _cvs.allrects.length) {
                        _cvs.flag_grabbing = false;

                        for (var i = 0; i < _cvs.allrects[_cvs.curr_rects_index].points.length; i++) {
                            var px = _cvs.allrects[_cvs.curr_rects_index].points[i].x;
                            var py = _cvs.allrects[_cvs.curr_rects_index].points[i].y;
                            //判断 x y 是否是这个点
                            // 使用勾股定理计算鼠标当前位置是否处于当前点上
                            var distanceFromCenter = Math.sqrt(Math.pow(px - x, 2) + Math.pow(py - y, 2));
                            if (distanceFromCenter <= 6) {
                                console.debug("开启拖拽");
                                _cvs.allrects[_cvs.curr_rects_index].points[i].flag_grabbing = true;
                                _cvs.flag_grabbing = true;
                            }
                        }

                        if (_cvs.flag_grabbing) {
                            _cvs.canvas.style.cursor = "grabbing";
                            return;
                        }
                    }

                    _cvs.curr_rects_index = _cvs.allrects.length;
                    console.debug("创建点", _cvs.curr_rects_index, "数量", _cvs.allrects.length);
                    _cvs.addRectItem("新建" + _cvs.allrects.length, [{ x: x, y: y }], '#0000FF', 'rgba(255, 0, 0, 0.3)');

                    _cvs.flag_draw_create = true; //开始绘制新的点
                    _cvs.canvas.style.cursor = "crosshair";

                } else {

                    if (_cvs.curr_point != null) {
                        if (Math.abs(x - _cvs.curr_point.x) < 20 && Math.abs(y - _cvs.curr_point.y) < 20) {
                            return;  //不能是上次的点
                        }
                    }

                    if (_cvs.curr_rects_index >= 0 && _cvs.curr_rects_index < _cvs.allrects.length)
                        _cvs.allrects[_cvs.curr_rects_index].points.push({ x: x, y: y });
                }

                _cvs.curr_point = { x: x, y: y }; //当前点击位置
                //需要拖动位置
                _cvs.draw();
            }

            if (evt.button == 2) {
                _cvs.stopDrawNewRect(false);
                return;
            }
        };


        this.canvas.onmousemove = function (evt) {
            if (_cvs.enable_draw_flag == false)
                return;

            // var x = evt.pageX - this.offsetLeft;
            // var y = evt.pageY - this.offsetTop;
            _cvs.GetMouseInCanvasLocation(evt);
            var x = _cvs.mouseX;
            var y = _cvs.mouseY;
            _cvs.mouse_point = { x: x, y: y }; //当前点

            if (_cvs.flag_grabbing) { //拖拽
                if (_cvs.curr_rects_index >= 0 && _cvs.curr_rects_index < _cvs.allrects.length)
                    for (var i = 0; i < _cvs.allrects[_cvs.curr_rects_index].points.length; i++) {
                        if (_cvs.allrects[_cvs.curr_rects_index].points[i].flag_grabbing == true) {
                            _cvs.allrects[_cvs.curr_rects_index].points[i].x = x;
                            _cvs.allrects[_cvs.curr_rects_index].points[i].y = y;
                        }
                    }
            } else {
                //判断点是否在这个区域
                // if (_cvs.flag_draw_create == false) { //没有绘图时
                //     for (var index = 0; index < _cvs.allrects.length; index++) {
                //         for (var i = 0; i < _cvs.allrects[index].points.length; i++) {
                //             if (pointinrects(x, y, _cvs.allrects[index].points)) {
                //                 _cvs.curr_rects_index = index;
                //                 break;
                //             }
                //         }
                //     }
                // }
            }
            _cvs.draw();
        }

        this.canvas.onmouseup = function (evt) {
            if (_cvs.enable_draw_flag == false)
                return;

            if (_cvs.flag_grabbing == true) {//停止拖拽
                if (_cvs.curr_rects_index >= 0 && _cvs.curr_rects_index < cvs.allrects.length) {
                    for (var i = 0; i < _cvs.allrects[_cvs.curr_rects_index].points.length; i++) {
                        if (_cvs.allrects[_cvs.curr_rects_index].points[i].flag_grabbing == true) {
                            _cvs.allrects[_cvs.curr_rects_index].points[i].flag_grabbing = false;   //停止拖拽
                            if (_cvs.callbk_AddRectIndex)
                                _cvs.callbk_AddRectIndex(_cvs.curr_rects_index);
                        }
                    }
                }

                _cvs.canvas.style.cursor = ""; //还原鼠标
                _cvs.flag_grabbing = false;    //停止拖拽
            }
        }

        this.canvas.addEventListener('contextmenu', function (evt) {
            console.debug("canvas.contextmenu");
            return false;
        });

        //按键按下
        window.addEventListener('keydown', function (event) {
            //  var keyID = e.keyCode ? e.keyCode : e.which;
            let code;
            console.debug("你的按键", event.key, event.keyCode, event.keyIdentifier);

            if (event.key !== undefined) {
                code = event.key;
            } else if (event.keyIdentifier !== undefined) {
                code = event.keyIdentifier;
            } else if (event.keyCode !== undefined) {
                code = event.keyCode;
            }

            if (code == 27 || code == "Escape")  //ESC
                _cvs.stopDrawNewRect(false);
            if (code == 8 || code == 46 || code == "Backspace" || code == "Delete")  //删除键： 停止并删除当前绘制
                _cvs.stopDrawNewRect(true);
        }, true);
    }

    stopDrawNewRect = (delflag) => {
        var _cvs = this;

        if (delflag == true) { //需要删除当前的区域
            //区域已经创建
            if (_cvs.curr_rects_index >= 0 && _cvs.curr_rects_index < _cvs.allrects.length) {
                _cvs.allrects.splice(_cvs.curr_rects_index, 1);
                var index = _cvs.curr_rects_index;
                _cvs.curr_rects_index = -1;

                if (_cvs.callfun_DelRectIndex != null && _cvs.flag_draw_create == false)
                    _cvs.callfun_DelRectIndex(index);
            }
            _cvs.curr_rects_index = -1;
            //_cvs.allrects.pop();
        } else {
            if (_cvs.flag_draw_create) { //是否是开始
                if (_cvs.curr_rects_index >= 0 && _cvs.curr_rects_index < _cvs.allrects.length) {
                    if (_cvs.allrects[_cvs.curr_rects_index].points.length < 3) { //2个点不能组成一个区域
                        _cvs.curr_rects_index = -1;
                        _cvs.allrects.pop();
                    } else {
                        //区域绘制完成
                        if (_cvs.callbk_AddRectIndex)
                            _cvs.callbk_AddRectIndex(_cvs.curr_rects_index);
                    }
                }
            } else {
                _cvs.curr_rects_index = -1;
            }
        }

        _cvs.canvas.style.cursor = ""; //还原鼠标
        _cvs.flag_draw_create = false; //停止绘制
        _cvs.draw();
    }

    draw = () => {
        //清理
        var _cvs = this;
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

        if (this.bkimg != null)
            this.ctx.drawImage(this.bkimg, 0, 0, this.canvas.width, this.canvas.height);

        for (var i = 0; i < this.allrects.length; i++) {
            var rect = this.allrects[i];

            //画区域
            _cvs.ctx.beginPath();

            rect.points.forEach(function (point) {
                _cvs.ctx.lineTo(point.x, point.y);
            });

            //如果是当前选择编辑区域，则绘制到鼠标位置
            if (_cvs.curr_rects_index == i && _cvs.mouse_point != null && _cvs.flag_draw_create) {
                _cvs.ctx.lineTo(_cvs.mouse_point.x, _cvs.mouse_point.y);
            }

            _cvs.ctx.lineTo(rect.points[0].x, rect.points[0].y);

            if (rect.strokeStyle) {
                _cvs.ctx.strokeStyle = rect.strokeStyle;
                _cvs.ctx.stroke(); //绘线
            }

            //区域填充
            if (_cvs.curr_rects_index == i) {
                _cvs.ctx.fillStyle = 'rgba(255, 0, 0, 0.7)';
                _cvs.ctx.fill(); //填充
            } else if (rect.fillStyle) {
                _cvs.ctx.fillStyle = rect.fillStyle;
                _cvs.ctx.fill(); //填充
            }

            //标题区
            _cvs.ctx.font = "bold 14px Arial";
            //_cvs.ctx.textAlign = "center";
            if (rect.strokeStyle) {
                _cvs.ctx.fillStyle = rect.strokeStyle;
            } else {
                _cvs.ctx.fillStyle = "#0000ff";
            }
            _cvs.ctx.fillText(rect.name, rect.points[0].x, rect.points[0].y - 3);

            //是否是当前编辑区域
            if (_cvs.curr_rects_index == i) {
                rect.points.forEach(function (point) { //点 绘圆
                    _cvs.ctx.beginPath();
                    _cvs.ctx.arc(point.x, point.y, 6, 0, 2 * Math.PI);
                    _cvs.ctx.fillStyle = "#0000ff";
                    _cvs.ctx.fill();
                });
            }
        }

        ////绘制鼠标所在点
        if (_cvs.mouse_point != null) {
            _cvs.ctx.beginPath();
            _cvs.ctx.arc(_cvs.mouse_point.x, _cvs.mouse_point.y, 2, 0, Math.PI * 2, false);
            _cvs.ctx.fillStyle = "#0000ff";
            _cvs.ctx.fill();
        }
    }

    /** 
       * 添加基于分辨率百分比的位置
       */
    setBkImgUrl = (url) => {
        //this.bkimg = new Image();
        //this.bkimg.src = url;
        var _cvs = this;

        var img = new Image();
        img.src = url;
        if (img.complete) {
            this.bkimg = img;
            this.draw();
        } else {
            img.onload = function () {
                _cvs.bkimg = img;
                _cvs.draw();

                console.debug("图片加载成功!", img.width, img.height);
            };
            img.onerror = function () {
                window.alert('图片加载失败，请重试');
            };
        }
    }

    /** 绘背景图 */
    drawBkImage = (img) => {
        this.bkimg = img;
        this.draw();
    }

    /** 添加一个区域: 标题，区域点线左边 strokeStyle='#0000FF', fillStyle='rgba(255, 0, 0, 0.3)' */
    addRectItem = (name, points, strokeStyle, fillStyle, bindObj) => {
        var index = this.allrects.length;

        //左上  右上   右下  左下, 四个点
        this.allrects.push({
            name: name,
            points: points, //坐标，是否需要排序，最左，最右
            strokeStyle: strokeStyle, //边线颜色
            fillStyle: fillStyle,  //填充颜色
            bindObj: bindObj //绑定的数据
        });
    }

    /**
     * 添加基于w与h分辨率位置的区域：固定块区域百分比, 上，下，左，右
     * @param {name} 名称
     */
    addRectBBB = (name, top, down, left, right, width, height, strokeStyle, fillStyle) => {
        //原始分辨率中的区域
        var x = width * left / 100;
        var y = height * top / 100;
        var x2 = width - width * right / 100;   // 50/100=x2/width x2=50*width/100
        var y2 = height - height * down / 100;

        var w = x2 - x;
        var h = y2 - y;

        //计算目标分辨率中的区域
        //x , w -> width;    a/c=b/d => ad=bc
        //y , h -> height;

        x = x * this.canvas.width / width;
        y = y * this.canvas.height / height;
        w = w * this.canvas.width / width;
        h = h * this.canvas.height / height;

        if (fillStyle == null)
            fillStyle = 'rgba(192, 80, 77, 0.5)';

        this.addRectItem(name, [
            { x: x, y: y },
            { x: x + w, y: y },
            { x: x + w, y: y + h },
            { x: x, y: y + h },
        ],
            strokeStyle,
            fillStyle
        );
        this.draw();
    }

    /** 添加来自其它分辨率的坐标(非百分比坐标) */
    addRectOthreWH = (name, x, y, w, h, width, height, strokeStyle, fillStyle) => {
        x = x * this.canvas.width / width;
        y = y * this.canvas.height / height;
        w = w * this.canvas.width / width;
        h = h * this.canvas.height / height;

        this.color.length;

        this.addRectItem(name,
            [
                { x: x, y: y },
                { x: x + w, y: y },
                { x: x + w, y: y + h },
                { x: x, y: y + h },
            ],
            strokeStyle, //边线颜色
            fillStyle,  //填充颜色
        );
    }
    /** 清理所有区域 */
    clearAllRect = () => {
        this.allrects = [];
        this.curr_rects_index = -1;
        this.draw();
    }
    /** 设定当前区域 */
    setSelectIndex = (index) => {
        this.curr_rects_index = index;
        this.draw();
    }

    /** 设定区域名字 */
    setRectName = (index, name) => {
        if (index >= 0 && index < this.allrects.length) {
            this.allrects[index].name = name;
        }
    }

    /** 删除区域 依据名称 */
    rmRectFromName = (name) => {
        for (var i = 0; i < this.allrects.length; i++) {
            var item = this.allrects[i];
            if (name == item.name) {
                this.allrects.splice(i, 1);
                i--;
            }
        }
        this.draw();
        //this.allrects[index].name = name;
    }

    /** 绑定数据 */
    setRectBindObj = (index, obj) => {
        if (index >= 0 && index < this.allrects.length) {
            this.allrects[index].bindObj = obj;
        }
    }

    getRectBindObj = (index) => {
        return this.allrects[index].bindObj;
    }


    /** 获取百分比区域 */
    getPercentageRects = () => {
        var rects = [];

        //需要转换坐标
        this.allrects.forEach((value, index, array) => {

            var points = [];
            value.points.forEach((point) => {
                //转换成百分比坐标 
                points.push(xpoint2bpoint(point.x, point.y, cvs.canvas.width, cvs.canvas.height));
            });

            rects.push({ name: value.name, points: points });
        });

        return rects;
    }

    /** 添加百分比坐标区域 */
    addPercentageRectItem = (name, bpoints, bindObj) => {
        var points = [];

        bpoints.forEach((v, index) => {
            points.push(bpoint2xpoint(v.bx, v.by, this.canvas.width, this.canvas.height));
        });

        this.addRectItem(name,
            points,
            '#0000FF',
            'rgba(192, 80, 77, 0.7)',
            bindObj
        );
        this.draw();
    }
}

/** 百分比点转换成像素点(px) */
function bpoint2xpoint(bx, by, width, height) {
    let x = bx * width / 100;   //?/width= bx/100   ?*100=bx*width
    let y = by * height / 100;
    return { x: x, y: y };
}

/** 像素点转百分比点  x/width= ?/100  x*100/width    11/80 = 13/100   13= 11/80*100; //11*100/80*/
function xpoint2bpoint(x, y, width, height) {
    let bx = x / cvs.canvas.width * 100;
    let by = y / cvs.canvas.height * 100;

    return {
        bx: bx.toFixed(2),
        by: by.toFixed(2)
    };
}

function min(a, b) {
    if (a > b) return b;
    return a;
}

function max(a, b) {
    if (a > b) return a;
    return b;
}

/** 判断点是否在这个区域 */
function pointinrects(x, y, pt) {
    let nCross = 0;
    let nCount = pt.length;
    let p = { x: x, y: y };

    for (let i = 0; i < nCount; i++) {
        let p1 = pt[i];
        let p2 = pt[(i + 1) % nCount];
        if (p1.y == p2.y) {
            if (p.y == p1.y && p.x >= min(p1.x, p2.x) && p.x <= max(p1.x, p2.x))
                return true;
            continue;
        }
        if (p.y < min(p1.y, p2.y) || p.y > max(p1.y, p2.y))
            continue;

        let x = (p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;

        if (x > p.x)
            nCross++;
        else if (x == p.x)
            return true;
    }
    if (nCross % 2 == 1)
        return true;
    return false;
}
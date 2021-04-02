/**
 * Websocket通信
 */
class jWebsocket {
    constructor(options) {
        this.url = options.url;
        this.funDrawImage = null;
        this.callbk_ProtoReqMsg = null;
        this.stat = "offline";

        if (options.hasOwnProperty("funDrawImage"))
            this.funDrawImage = options.funDrawImage;
        if (options.hasOwnProperty("funProtoReqMsg"))
            this.callbk_ProtoReqMsg = options.funProtoReqMsg;

        this.open();
    }

    /** open */
    open = (url) => {
        if (url == null)
            url = this.url;
        this.url = url;
        if (this.url == null)
            return;
        if (this.ws != null) {
            this.ws.close();
            this.ws = null;
        }
        this.ws = new WebSocket(this.url);
        this.ws.binaryType = 'arraybuffer';
        this.ws.onopen = (event) => {
            console.debug("open...:", this.url);
            this.stat = "online";
        };
        this.ws.onclose = (event) => {
            console.debug("onclose", event, this.url);
            this.stat = "offline";   //连接断开，是否重试？
        };
        this.ws.onerror = (event) => {
            console.debug("onerror", event, this.url);
            this.stat = "offline";
        };
        this.ws.onmessage = (event) => {
            this.handleMsg(event);
        };
    }

    close = () => {
        if (this.ws != null) {
            this.ws.close();
            this.ws = null;
        }
    }

    /** 处理消息 */
    handleMsg = (event) => {
        if (event.data instanceof ArrayBuffer) {
            var blob = event.data;
            var array = new Uint8Array(blob);
            //
            if (array[0] == 123) { //123='{' JSON
                var jsonstring = Uint8Array2String(array, 0);
                var exIndex = Uint8ArrayFindIndex(array, 0, 0);
                var exlength = 0;
                var exdata = null;

                if (exIndex != -1) {
                    exIndex = exIndex + 1 + 4;
                    exlength = array.length - exIndex - 4;
                    exdata = blob.slice(exIndex, exIndex + exlength);
                }
                var jsonobj = JSON.parse(jsonstring);              

                if (this.callbk_ProtoReqMsg != null)
                    this.callbk_ProtoReqMsg(jsonobj, exdata, exlength);

            } else {
                if (array[0] == 1) {
                    let dv = new DataView(array.buffer);
                    let type = dv.getUint8(0, true);
                    let time_id = Number(dv.getBigUint64(1, true));

                    if (type == 1) {
                        var dataString = Uint8Array2String(array, 9); //从第9位开始
                        var json = JSON.parse(dataString);
                        var i = Uint8ArrayFindIndex(array, 0, 9);

                        if (i < array.length && json.action == "ImgFaceRects") {
                            var imgdata = array.slice(i + 1);
                            var imgbase64 = arrayBufferToBase64(imgdata);

                            //回调
                            if (this.funDrawImage != null) {
                                this.funDrawImage(json.width, json.height, json.rects, imgbase64);
                            }
                        }
                    }
                }
            }
        }
    }

    /** 发送消息 */
    sendLxMessage = (obj) => {
        this.ws.send(JSON.stringify(obj));
    }

}

function Uint8ArrayFindIndex(array, v, index) {
    for (var i = index; i < array.length; i++) {
        if (array[i] == v)
            return i;
    }
    return -1;
}

/**
 * 从第index开始转换成字符串，直到末尾或为0
 */
function Uint8Array2String(array, index) {
    var dataString = "";
    var char2, char3;
    var i = index;

    while (i < array.length) {
        if (array[i] == 0)
            break;
        var c = array[i++];
        switch (c >> 4) {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                // 0xxxxxxx
                dataString += String.fromCharCode(c);
                break;
            case 12: case 13:
                // 110x xxxx 10xx xxxx
                char2 = array[i++];
                dataString += String.fromCharCode(((c & 0x1F) << 6) | (char2 & 0x3F));
                break;
            case 14:
                // 1110 xxxx 10xx xxxx 10xx xxxx
                char2 = array[i++];
                char3 = array[i++];
                dataString += String.fromCharCode(((c & 0x0F) << 12) |
                    ((char2 & 0x3F) << 6) |
                    ((char3 & 0x3F) << 0));
                break;
        }
    }
    return dataString;
}

function arrayBufferToBase64(buffer) {
    var binary = '';
    var bytes = new Uint8Array(buffer);
    var len = bytes.byteLength;
    for (var i = 0; i < len; i++) {
        binary += String.fromCharCode(bytes[i]);
    }
    return window.btoa(binary);
}

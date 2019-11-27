-- wireshark lua 分析websocket协议内容为json数据
--
local cj=Proto("cj", "cj proto")

local f_proto = ProtoField.uint8("cj.protocol", "Protocol", base.DEC, vs_protos)
local f_dir = ProtoField.uint8("cj.direction", "Direction", base.DEC, { [1] = "incoming", [0] = "outgoing"})
local f_text = ProtoField.string("cj.text", "Text")

cj.fields = { f_proto, f_dir, f_text }

local data_dis = Dissector.get("data")

local fields = {}

local args = {"websocket","data" }

for i, arg in ipairs(args) do
    fields[i] = Field.new(arg)
end


function cj.dissector(buf, pktinfo, tree)

	local iswebsocket=false
    
    local websocketfield={ fields[1]() }
    local dataf={ fields[2]() }

    if #websocketfield> 0 then
        local finfo=dataf[1]

        if finfo ~=nil then
            local v=finfo.value
            local tvbuf=ByteArray.tvb(v)
    
            print(typeof(v), "value:" , v)
    
            pktinfo.cols.protocol:set("CJ")
            pktinfo.cols.info="this is cj proto"
    
            local subtree = tree:add(cj, tvbuf)
            local json=Dissector.get("json");
             
            json:call(tvbuf, pktinfo, tree)
        end
    end
end

-- local disstcp=DissectorTable.get("tcp.port")
-- disstcp:add(1234, cj)

--[[
--遍历所有解析器
for k,v in pairs(DissectorTable.list()) do
	print(">>" .. v)
end

--子解码器获取
local dis=DissectorTable.get("media_type")
print(typeof(dis))

local dis=dis:get_dissector("image/jpg")
print(typeof(dis))
DissectorTable.get("sctp.ppi"):get_dissector(3), -- m3ua

    c中 register_dissector 的解码器才能通过Dissector获取
        dissector_add_string("name","..", handle); 这样的只能通过 DissectorTable.get("name"):get_dissector(..)
        register_dissector_table("name") 这里注册到 DissectorTable

        dissector_try_uint_new(subdissector_table, low_port, ...) tcp端口协议子解码

        --packet-http.c 解析media_type时，获取dissector       
        dissector_get_string_handle(
                    media_type_subdissector_table
                    
local list=Field.list() --能获取所有的

]]

register_postdissector(cj)



do
    -- calling tostring() on random FieldInfo's can cause an error, so this func handles it
    local function getstring(finfo)
        local ok, val = pcall(tostring, finfo)
        if not ok then val = "(unknown)" end
        return val
    end
    
    -- Create a new dissector
    MQTToverWebsocket = Proto("MQTToverWebsocket", "MQTT over Websocket")
    mqtt_dissector = Dissector.get("mqtt")
    -- The dissector function
    function MQTToverWebsocket.dissector(buffer, pinfo, tree)
        local fields = { all_field_infos() }
        local websocket_flag = false
        for i, finfo in ipairs(fields) do
            if (finfo.name == "websocket") then
                websocket_flag = true
            end
            if (websocket_flag == true and finfo.name == "data") then
                local str1 = getstring(finfo)
                local str2 = string.gsub(str1, ":", "")
                local bufFrame = ByteArray.tvb(ByteArray.new(str2))
                mqtt_dissector = Dissector.get("mqtt")
                --mqtt_dissector:call(finfo.source, pinfo, tree) #9 BUG
                mqtt_dissector:call(bufFrame, pinfo, tree)
                --mqtt_dissector:call(finfo.value, pinfo, tree)
                websocket_flag = false
                pinfo.cols.protocol = "MQTT over Websocket"
            end
    end
        
        --ws_dissector_table = DissectorTable.get("ws.port")
        --ws_dissector_table:add("443",mqtt_dissector)
    end
    -- Register the dissector
    --ws_dissector_table = DissectorTable.get("ws.port")
    --ws_dissector_table:remove(443, mqtt_dissector)
    --ws_dissector_table:add(443, MQTTPROTO)
    --ws_dissector_table:add_for_decode_as(mqtt_dissector)
    --register_postdissector(MQTToverWebsocket)
end
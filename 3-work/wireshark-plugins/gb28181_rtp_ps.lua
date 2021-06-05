--[[
	创建一个新的协议结构 foo_proto
	第一个参数是协议名称会体现在过滤器中
	第二个参数是协议的描述信息，无关紧要
--]]
local foo_proto = Proto("GB28181-RTP", "GB28181 RTP Protolcol")
 
--[[
	下面定义字段
	base表示以什么方式展现，有base.DEC, base.HEX, base.OCT, base.DEC_HEX, base.DEC_HEX or base.HEX_DEC
--]]
local rptps_version = ProtoField.uint8("rptps.version", "版本", base.HEX)
local rptps_padding = ProtoField.uint8("rptps.padding", "padding", base.HEX)
local rptps_extension = ProtoField.uint8("rptps.extension", "extension", base.HEX)
local rptps_csrc_len = ProtoField.uint8("rptps.csrc_len", "csrc_len", base.HEX)
           
local rptps_marker = ProtoField.uint8("rptps.marker", "marker", base.DEC)
local rptps_payload = ProtoField.uint8("rptps.payload", "payload", base.DEC)
           
local rptps_seq = ProtoField.uint16("rptps.seq", "seq", base.DEC)
local rptps_timestamp = ProtoField.uint32("rptps.timestamp", "timestamp", base.DEC)
local rptps_ssrc = ProtoField.uint32("rptps.ssrc", "ssrc", base.DEC)

local rptps_data = ProtoField.bytes("rptps.data","Data")
 
-- 将字段添加都协议中
foo_proto.fields = {
      rptps_version,
      rptps_padding,
      rptps_extension,
      rptps_csrc_len,
                   
      rptps_marker,
      rptps_payload,
                    
      rptps_seq ,
      rptps_timestamp,
      rptps_ssrc,
      
      rptps_data
}
 

----------------------------------------------
local ps_proto = Proto("rtp-ps", "GB28181 RTP PS Protolcol")

-- 第一个参数是过滤器过滤中显示的字段
-- 第二个参数是协议解析时显示的字段
-- 第三个参数是解析成number时显示的格式，十进制显示还是16进制显示
-- 第四个参数是解析出的结果，通过table转换成对应的值，如果不需要转换，则置为nil
-- 第五个参数是会将待解析的入参进行与操作，取对应的bit位进行后续的解析。当一个字节中存在多个字段解析时是必要的

local ps_ps_head = ProtoField.uint32("ps.head", "PS head", base.HEX_DEC)
local ps_sh_head = ProtoField.uint32("sh.head", "SH head", base.HEX_DEC)

local ps_ps_lenght=ProtoField.uint8("ps.length", "length", base.DEC, nil, 0x07)

local ps_sh_length=ProtoField.uint16("sh.length", "SH length", base.DEC)
local ps_psm_head=ProtoField.uint32("psm.head", "psm head", base.HEX_DEC)

local ps_psm_length=ProtoField.uint16("psm.length", "length", base.DEC)

local ps_pes_head=ProtoField.uint32("pes.head", "pes head", base.HEX_DEC)

local PES_packet_length=ProtoField.uint16("pes.packlen", "pack length", base.HEX_DEC)
local ps_data=ProtoField.bytes("rptps.data","Data")

ps_proto.fields= {
	ps_ps_head,
	ps_ps_lenght,
	ps_sh_head,
	ps_sh_length,	
	ps_psm_head,
	ps_psm_length,
	ps_pes_head,
	PES_packet_length,
	ps_data
}

--[[

对于包含IDR帧的PS包的内容为："PS包起始码（0x00 00 00 01 BA）" + "系统头" + "PSM" +"PES header" + "h264 stream"
对于包含非IDR帧的PS包的内容为："PS包起始码（0x00 00 00 01 BA）" +"PES header" + "h264 stream"

sh: 0x000001BB
video header: 00 00 01 E0
audio header: 00 00 01 C0

--]]

function ps_proto.dissector(buf, pinfo, treeitem)
	-- 设置一些 UI 上面的信息
	local headid=buf(offset, 4):uint()
	
	local foo_tree = treeitem:add(ps_proto, buf:range(buf_len))
	local offset = 0

	local info=""
	local pes_packet_length=0
	
	foo_tree:add("[length:" .. buf:len() .. "]")
	
	
	if headid==0x0001BA then
		foo_tree:add(ps_ps_head, buf(0,4))
		foo_tree:add(buf(4,1),"unit:" .. buf(4,1):uint())

		foo_tree:add(ps_ps_lenght, buf(13,1))
		
		offset=14+buf(13,1):bitfield(5,3)
		
		info="ps"
		
		headid=buf(offset, 4):uint()
		
		if headid== 0x000001BB then
				info=info .. ",sh"
				
				local tree_sh=foo_tree:add(ps_sh_head, buf(offset,4))
				offset=offset+4
			
				local sh_length=buf(offset, 2)
				tree_sh:add(ps_sh_length,sh_length)
				
				offset=offset+2+sh_length:uint()
				
				---------------------------------
				headid=buf(offset, 4):uint()
				
				if headid== 0x000001BC then	
								info = info .. ",pes-video"
								local tree_psm=foo_tree:add(ps_psm_head, buf(offset,4))
								offset=offset+4
								tree_psm:add(ps_psm_length, buf(offset,2) )
								
								offset=offset+buf(offset,2):uint()+2
								
								------------------------
								headid=buf(offset, 4):uint()
								if headid== 0x000001E0 or headid==0x000001C0 then	
									local tree_pes=foo_tree:add(ps_pes_head, buf(offset,4))
									offset=offset+4
									
									pes_packet_length = buf(offset,2):uint()
									tree_pes:add(PES_packet_length, buf(offset,2))	
													
									info=info .. "(" .. pes_packet_length .. ")"							
								end
								-------------------------------
				end
				---------------------------------
		elseif headid== 0x000001E0 or headid==0x000001C0 then	
					
			local tree_pes=foo_tree:add(ps_pes_head, buf(offset,4))

			foo_tree:add(buf(offset+3,1), "PS ID:" .. buf(offset+3,1):uint())
			offset=offset+4

			pes_packet_length = buf(offset,2):uint()
			tree_pes:add(PES_packet_length, buf(offset,2))
			offset=offset+2		
			foo_tree:add(buf(offset,2), "PES:" .. buf(offset,2):uint())
			offset=offset+2
			
			peslen=buf(offset,1):uint()
			foo_tree:add(buf(offset,1), "PES Len:" .. peslen)
			offset=offset+1
			foo_tree:add(buf(offset,peslen), "PES Ex:")

			videolen=pes_packet_length-peslen-3

			offset=offset+peslen	

			foo_tree:add(ps_data, buf(offset, videolen))

			info=info .. ",pes" .. "(" .. pes_packet_length .. ")"			
		else		
				info = "data"		
				foo_tree:add(ps_data, buf(0, buf:len()))
		end
		
	else
		info = "data"		
		foo_tree:add(ps_data, buf(0, buf:len()))
	end
	
	pinfo.cols.protocol:set("rtp-ps")
	pinfo.cols.packet_len:set("0")
	pinfo.cols.info:set(info .. ", all length:" .. buf:len() )
	
end

--[[
	下面定义 foo 解析器的主函数，这个函数由 wireshark调用
	第一个参数是 buf 类型，表示的是需要此解析器解析的数据
	第二个参数是 Pinfo 类型，是协议解析树上的信息，包括 UI 上的显示
	第三个参数是 TreeItem 类型，表示上一级解析树
--]]
function foo_proto.dissector(buf, pinfo, treeitem)
	local offset = 0
	local buf_len = buf:len()
  
	local seq=buf(2,2):uint()
	local payload=buf(1, 1):bitfield(1,7)
	local mark=buf(1, 1):bitfield(0,1)
	
	
	local rtp_buf = buf(12,buf_len-12):tvb()
	local data_len= rtp_buf:len()
	
	-- 设置一些 UI 上面的信息
	pinfo.cols.protocol:set("GB28181-RTP")
	pinfo.cols.info:set(payload .. " RTP Protolcol seq:" .. seq .. ",data length:" .. buf_len .. "," .. data_len .. ",mark:" .. mark)
	
	-- 在上一级解析树上创建 foo 的根节点
	local foo_tree = treeitem:add(foo_proto, buf:range(buf_len))
	
	foo_tree:add("RTP all len:" .. buf_len .. ", data:" .. data_len)
	
	foo_tree:add(rptps_version,buf(0, 1):bitfield(0,2))
	foo_tree:add(rptps_padding,buf(0, 1):bitfield(3,1))
	foo_tree:add(rptps_extension,buf(0, 1):bitfield(5,1))
	foo_tree:add(rptps_csrc_len,buf(0, 1):bitfield(6,1))

	foo_tree:add(rptps_marker,buf(1, 1):bitfield(0,1))
	foo_tree:add(rptps_payload, buf(1, 1):bitfield(1,7))

	foo_tree:add(rptps_seq, buf(2,2))
	foo_tree:add(rptps_timestamp, buf(5,4))
	foo_tree:add(rptps_ssrc, buf(8,4))
  
	foo_tree:add(rptps_data, buf(12, buf_len-12))
	
	
	local rtp_ps_diss=Dissector.get("rtp-ps")
	
	rtp_ps_diss:call(rtp_buf, pinfo, treeitem)
	
	--local data_dis = Dissector.get("data")
	--data_dis:call(rtp_buf, pinfo, treeitem)
	
	print("....")
	
end
 
-- 向 wireshark 注册协议插件被调用的条件
local udp_port = DissectorTable.get("udp.port")
udp_port:add(6000, foo_proto)

local udp_port = DissectorTable.get("udp.port")
udp_port:add(6001, ps_proto)


--------------------------------------------------
local function getstring(finfo)
    local ok, val = pcall(tostring, finfo)
    if not ok then val = "(unknown)" end
    return val
end
    
local rtp_ps_port = Proto("call-rtp-ps","call rtp ps")

function rtp_ps_port.dissector(buffer, pinfo, tree)
 	local fields = { all_field_infos() }
	local websocket_flag = false
		
    for i, finfo in ipairs(fields) do
        		
        	-- print("name:" .. finfo.name)
        		
            if (finfo.name == "rtp") then
                websocket_flag = true
            end
            
            if (websocket_flag == true and finfo.name == "rtp.payload") then
                local str1 = getstring(finfo)
                local str2 = string.gsub(str1, ":", "")
                local bufFrame = ByteArray.tvb(ByteArray.new(str2))
                   
                rtp_diss = Dissector.get("rtp-ps")
                rtp_diss:call(bufFrame, pinfo, tree)
                
                websocket_flag = false
                pinfo.cols.protocol = "rtp to ps"
            end
    end
end

register_postdissector(rtp_ps_port)

--[[
local t = Dissector.list()

for _,name in ipairs(t) do
    print(name)
end

--查看所有支持的table
local dt = DissectorTable.list()

for _,name in ipairs(dt) do
    print(name)
end
--]]

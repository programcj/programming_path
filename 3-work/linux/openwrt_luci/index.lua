-- 需要在controller目录下新建立目录cj
module("luci.controller.cj.index", package.seeall)

function index()

		local root = node()
		if not root.target then
			root.target = alias("cj")
			root.index = true
		end
	
		local page   = node("cj")
		page.target  = firstchild()
		page.title   = _("cj")
		page.order   = 10
		page.ucidata = true
		page.index = true
	
		-- Empty services menu to be populated by addons
		
		entry({"cj","info"},call("action_info"),_("Info"), 22 ).index = true
		entry({"cj","upapp"},call("action_upapp"),_("update app ipk"), 90)
		entry({"cj","setinfo"},call("action_setinfo"),_("set info"), 91)
end

function action_info()
	-- luci.sys.exec("ifconfig")
	
	luci.http.prepare_content("application/json")
	luci.http.write_json({ info= luci.sys.exec("uname -a") , timestring=os.date("%Y-%m-%d %H:%M:%S") })
	
end

function action_upapp()
	local sys = require "luci.sys"
	local fs  = require "luci.fs"
	local ipkg = require("luci.model.ipkg")
	
	local restore_cmd = "tar -xzC/ >/dev/null 2>&1"
	local image_tmp   = "/tmp/app.ipk"
	local  metaname = ""
	
	local fp
	
	luci.http.setfilehandler(
		function(meta, chunk, eof)
			
			if not meta  then return end
		
			if not fp then
					metaname = meta.name
					fp = io.open(image_tmp, "w")
			end
			
			if chunk then
				fp:write(chunk)
			end
			
			if eof then
				fp:close()
				
				local install = { }
				local stdout  = { "" }
				local stderr  = { "" }
				local out, err
								
				install[image_tmp], out, err = ipkg.install(image_tmp)
				stdout[#stdout+1] = out
				stderr[#stderr+1] = err
				
				luci.http.prepare_content("application/json")
				luci.http.write_json({ 
						file= image_tmp, 
						name =metaname ,
						timestring=os.date("%Y-%m-%d %H:%M:%S"),
						size = nixio.fs.stat(image_tmp).size,
						install   = install,
						stdout    = table.concat(stdout, ""),
						stderr    = table.concat(stderr, "")
					})
			end
			
		end
	)
	
	  if luci.http.formvalue("ipkname")  then
         return
     end
     
end


function action_setinfo()
	local sn = luci.http.formvalue("sn")
	local macwan = luci.http.formvalue("macwan")
	local maclan = luci.http.formvalue("maclan")
	local msg ="result:"
	
	if sn then
	
	else
		msg = msg .. "[not sn]"
	end
	
	if macwan then
	
	else
		msg = msg .. "[not macwan]"
	end
	
	if maclan then
	
	else
		msg = msg .. "[not maclan]"
	end
	
	luci.http.prepare_content("application/json")
	luci.http.write_json({
		info= luci.sys.exec("uname -a") ,
		sn=sn,
		macwan=macwan,
		maclan=maclan,
		timestring=os.date("%Y-%m-%d %H:%M:%S"),
		msg=msg
	})
end

-- http://192.168.1.106/cgi-bin/luci/cj/info
-- curl -F "ipkname=@ubus-cj.c;filename=image.jpg;type=application/octet-stream" http://192.168.1.106/cgi-bin/luci/cj/upapp
-- http://192.168.1.106/cgi-bin/luci/cj/setinfo

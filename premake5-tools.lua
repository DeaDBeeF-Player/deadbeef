-- pkgconfig functions

function pkgconfig (pkgname)
  links { pkgconfig_libs (pkgname) }
  includedirs { pkgconfig_includedirs (pkgname) }
  libdirs { pkgconfig_libdirs (pkgname) }
end

function pkgconfig_check (pkgname)
  command = "pkg-config " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    --print ("pkg-config failed for " .. pkgname)
    return nil
  else
  	return true
  end
end

function pkgconfig_includedirs (pkgname)
  command = "pkg-config --cflags-only-I " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    error ("pkg-config failed for " .. pkgname)
  end
  parts = string.explode(returnval, " ")
  tab2 = {}
  for i, v in ipairs(parts) do
    tab2[i] = string.sub (v, 3)
  end
  return tab2
end

function pkgconfig_libdirs (pkgname)
  command = "pkg-config --libs-only-L " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    error ("pkg-config failed for " .. pkgname)
  end
  parts = string.explode (returnval, " ")
  tab2 = {}
  for i, v in ipairs(parts) do
    tab2[i] = string.sub (v, 3)
  end
  return tab2
end

function pkgconfig_libs (pkgname)
  command = "pkg-config --libs-only-l " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    error ("pkg-config failed for " .. pkgname)
  end
  parts = string.explode (returnval, " ")
  tab2 = {}
  for i, v in ipairs(parts) do
    tab2[i] = string.sub (v, 3)
    tab2[i] = tab2[i] .. " " -- fix problems when project name is same as library
    -- this will favor linking with library than with itself
  end
  return tab2
end

-- common options
local nls_value = nil
function nls ()
	if nls_value == nil then
		newoption {
			trigger = "nls",
			value = "VALUE",
			description = "compile program with native language support",
			default = "enabled",
			allowed = {
			  { "disabled",    "Disabled" },
			  { "enabled",  "Enabled" }
			}
		}
		nls_value = _OPTIONS["nls"]
	end
	if nls_value == "disabled" then
		return false
	else
		return nls_value
	end
end

newoption {
	trigger = "skip-checks",
	description = "omit all library checks",
}

if _OPTIONS["help"] ~= nil then
	_OPTIONS["skip-checks"] = true
end

-- option functions
options_dic = {};
local options_pkgs_missing = {}
function add_option (name)
	local a = string.gsub(name, "plugin%-", "")
	if a ~= name then
		a = a .. " plugin"
	end
	newoption {
		trigger = name,
		value = "auto",
		description = "build " .. a,
		default = "auto",
		allowed = {
		  { "disabled",    "Disabled" },
		  { "enabled",  "Enabled" },
		  { "auto",  "Auto-detect" }
		}
	}
end

function option (name, ...)
	add_option (name)
	-- if not disabled check for libs
	local a = {}
	local b = 0;
	if _OPTIONS["skip-checks"] ~= nil then
  		return nil
  	end
	if  _OPTIONS[name] ~= "disabled" then
		-- for each set
		if ... == nil then
			-- print("\27[93m" .. "WARN: " ..  "\27[39m" .. name .. " did not define any libraries to check for, enabling anyway")
			options_dic[name] = "yes"
			return true
		end
		local set = 1
		for j,v in ipairs({...}) do
		    parts = string.explode(v, " ")
		    -- for each package
		    for i, v in ipairs(parts) do
		    	if pkgconfig_check (v) == nil then
					if #{...} == 1 then
						print("\27[93m" .. "pkg-config did not found package " .. v .. " required by " ..  name .. "\27[39m")
					end
			   		set = 0
			   		table.insert (options_pkgs_missing, v)
			   		a[v] = false
			   	else
			   		a[v] = true
			   	end
		    end
		    --print (set)
		    if set ~= 0 then
		    	b = b + 1
		    end
	    end
		if _OPTIONS[name] == "enabled" and set == 0 then
			error ("pkg-config did not found package " .. v .. " required by " ..  name)
   		end
   		if b == 0 then
   			a = nil
   			options_dic[name] = "no"
   		else
   			options_dic[name] = "yes"
   		end
   	else
   		-- disabled
        options_dic[name] = "no"
   		return nil
	end
	--print ("a = "..a)
	return a
end

function option_nofail (name, ...)
	add_option (name)
	-- if not disabled check for libs
	local a = {}
	local b = 0;
	if  _OPTIONS[name] ~= "disabled" then
		-- for each set
		for j,v in ipairs({...}) do
		    parts = string.explode(v, " ")
		    local set = 1
		    -- for each package
		    for i, v in ipairs(parts) do
		    	if pkgconfig_check (v) == nil then
			   		print("\27[93m" .. "pkg-config did not found package " .. v .. " required by " ..  name .. "\27[39m")
			   		set = 0
			   		a[v] = false
			   	else
			   		a[v] = true
			   	end
		    end
		    --print (set)
		    if set ~= 0 then
		    	b = b + 1
		    end
	    end
		if _OPTIONS[name] == "enabled" and set == 0 then
			print ("\27[93m" .. "pkg-config did not found package " .. v .. " required by " ..  name .. "\27[39m")
   		end
   		if b == 0 then
   			a = nil
   		end
	end
	--print ("a = "..a)
	return a
end

function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

function print_options ()
	if _OPTIONS["skip-checks"] ~= nil then
  		return true
  	end
	test = options_pkgs_missing
	hash = {}
	res = {}
	for _,v in ipairs(test) do
		if (not hash[v]) then
	    	res[#res+1] = v -- you could print here instead of saving to result table if you wanted
	    	hash[v] = true
	   	end
	end
	local pkgs = ""
   	for i,v in ipairs(res) do
   		pkgs = pkgs .. " " .. v
   	end
	print ("Missing packages for full compile:" .. pkgs .. "\n")
   	print ("Plugin summary:\n")
   	sorted = {}
   	for n in pairs(options_dic) do table.insert(sorted, n) end
    table.sort(sorted)
    for i,n in ipairs(sorted) do
   		local a = string.gsub(n, "plugin%-", "")
		if a ~= name then
			print ("\t" .. a ..": " .. options_dic[n])
		end

    end
end

function get_version ()
	fp = io.open ("PORTABLE_VERSION","r")
	io.input (fp)
	local ver = io.read()
	io.close (fp)
	return ver
end

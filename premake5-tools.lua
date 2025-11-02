-- pkgconfig functions

-- Add libs to project based on pkgconfig
function pkgconfig (pkgname)
  links { pkgconfig_libs (pkgname) }
  includedirs { pkgconfig_includedirs (pkgname) }
  libdirs { pkgconfig_libdirs (pkgname) }
  buildoptions { pkgconfig_cflags (pkgname) }
end

-- Returns true if package is installed
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

-- Returns dirs to include for pkgname
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

-- Returns dirs to libs for pkgname
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

-- Returns libs for pkgname
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
    if (v ~= nil and v ~= '') then
      tab2[i] = string.sub (v, 3)
      tab2[i] = tab2[i] .. " " -- fix problems when project name is same as library
      -- this will favor linking with library than with itself
    end
  end
  return tab2
end

-- Returns cflags for pkgname
function pkgconfig_cflags (pkgname)
  command = "pkg-config --cflags-only-other " .. pkgname
  returnval = os.outputof (command)
  if (returnval == nil)
  then
    error ("pkg-config failed for " .. pkgname)
  end
  parts = string.explode (returnval, " ")
  tab2 = {}
  for i, v in ipairs(parts) do
    tab2[i] = v
  end
  return tab2
end

-- Common options
local nls_value = nil

-- Skip checks option
newoption {
	trigger = "skip-checks",
	description = "omit all library checks",
}

-- Skip checks if help is being shown
if _OPTIONS["help"] ~= nil then
	_OPTIONS["skip-checks"] = true
end


-- nls() adds option for choosing nls and returns true if it got enabled
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

-- Option functions

-- Array (dictonary) that stores options and its value (for example 'option' -> 'yes')
options_dic = {};

-- Missing packages list
local options_pkgs_missing = {}

-- add_option(name) simply adds option for 'name'
local function add_option (name)
	-- strip 'plugin-' from beginning and make a = 'plugname plugin'
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

-- Option function
-- Adds option to argument and checks for libraries
-- Returns true if both argument and libraries are satisfied
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

function is_enabled (name)
	return options_dic[name] == "yes"
end

-- Prints out summary of options and their states
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

-- Returns deadbeef version which is stored in ./build_data/VERSION
-- WINDOWS: Return current date
function get_version ()
	if _OPTIONS["version-override"] ~= nil then
		date = os.date("%Y-%m-%d")
		fp = io.open ("build_data/VERSION","w")
		io.output (fp)
		io.write(date)
		io.close (fp)
		print(date)
		return date
	end
	fp = io.open ("build_data/VERSION","r")
	io.input (fp)
	local ver = io.read()
	io.close (fp)
	return ver
end

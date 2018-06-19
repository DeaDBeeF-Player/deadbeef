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

-- option functions

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
			error ("pkg-config did not found package " .. v .. " required by " ..  name)
   		end
   		if b == 0 then
   			a = nil
   		end
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

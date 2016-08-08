local posix = require("posix")

-- Targets:
--
-- {
--   fullname = full name of target
--   dir = target's build directory
--   outs = target's object files
--   is = { set of rule types which made the target }
-- }

local emitter = {}
local rules = {}
local targets = {}
local buildfiles = {}
local globals
local cwd = "."
local vars = {}
local parente = {}

-- Forward references
local loadtarget

local function print(...)
	local function print_no_nl(list)
		for _, s in ipairs(list) do
			if (type(s) == "table") then
				io.stderr:write("{")
				for k, v in pairs(s) do
					print_no_nl({k})
					io.stderr:write("=")
					print_no_nl({v})
					io.stderr:write(" ")
				end
				io.stderr:write("}")
			else
				io.stderr:write(tostring(s))
			end
		end
	end

	print_no_nl({...})
	io.stderr:write("\n")
end

local function assertString(s, i)
	if (type(s) ~= "string") then
		error(string.format("parameter %d must be a string", i))
	end
end

local function concat(...)
	local r = {}

	local function process(list)
		for _, t in ipairs(list) do
			if (type(t) == "table") and not t.is then
				process(t)
			else
				r[#r+1] = t
			end
		end
	end
	process({...})

	return r
end

local function inherit(high, low)
	local o = {}
	setmetatable(o, {
		__index = function(self, k)
			local x = high[k]
			if x then
				return x
			end
			return low[k]
		end
	})
	for k, v in pairs(high) do
		local _, _, kk = k:find("^%+(.*)$")
		if kk then
			o[kk] = concat(low[kk], v)
		end
	end
	return o
end

local function asstring(o)
	local t = type(o)
	if (t == "nil") then
		return ""
	elseif (t == "string") then
		return o
	elseif (t == "number") then
		return o
	elseif (t == "table") then
		if o.is then
			return asstring(o.outs)
		else
			local s = {}
			for _, v in pairs(o) do
				s[#s+1] = asstring(v)
			end
			return table.concat(s, " ")
		end
	else
		error(string.format("can't turn values of type '%s' into strings", t))
	end
end

local function concatpath(...)
	local p = table.concat({...}, "/")
	return (p:gsub("/+", "/"):gsub("^%./", ""):gsub("/%./", "/"))
end

-- Returns a list of the targets within the given collection; the keys of any
-- keyed items are lost. Lists and wildcards are expanded.
local function targetsof(...)
	local o = {}

	local function process(items)
		for _, item in pairs(items) do
			if (type(item) == "table") then
				if item.is then
					-- This is a target.
					o[#o+1] = item
				else
					-- This is a list.
					process(item)
				end
			elseif (type(item) == "string") then
				-- Filename!
				if item:find("^%+") then
					item = cwd..item
				elseif item:find("^%./") then
					item = concatpath(cwd, item)
				end
				o[#o+1] = loadtarget(item)
			else
				error(string.format("member of target list is not a string or a target"))
			end
		end
	end

	process({...})
	return o
end

local function filenamesof(...)
	local targets = targetsof(...)

	local f = {}
	for _, r in ipairs(targets) do
		if (type(r) == "table") and r.is then
			if r.outs then
				for _, o in ipairs(r.outs) do
					f[#f+1] = o
				end
			end
		elseif (type(r) == "string") then
			f[#f+1] = r
		else
			error(string.format("list of targets contains a %s which isn't a target",
				type(r)))
		end
	end
	return f
end

local function targetnamesof(...)
	local targets = targetsof(...)

	local f
	for _, r in pairs(targets) do
		if (type(r) == "table") and r.is then
			f[#f+1] = r.fullname
		elseif (type(r) == "string") then
			f[#f+1] = r
		else
			error(string.format("list of targets contains a %s which isn't a target",
				type(r)))
		end
	end
	return f
end

local function dotocollection(files, callback)
	if (#files == 1) and (type(files[1]) == "string") then
		return callback(files[1])
	end

	local o = {}
	local function process(files)
		for _, s in ipairs(files) do
			if (type(s) == "table") then
				if s.is then
					error("passed target to a filename manipulation function")
				else
					process(s)
				end
			else
				local b = callback(s)
				if (b ~= "") then
					o[#o+1] = b
				end
			end
		end
	end
	process(files)
	return o
end


local function abspath(...)
	return dotocollection({...},
		function(filename)
			assertString(filename, 1)
			if not filename:find("^[/$]") then
				filename = concatpath(posix.getcwd(), filename)
			end
			return filename
		end
	)
end


local function basename(...)
	return dotocollection({...},
		function(filename)
			assertString(filename, 1)
			local _, _, b = filename:find("^.*/([^/]*)$")
			if not b then
				return filename
			end
			return b
		end
	)
end


local function dirname(...)
	return dotocollection({...},
		function(filename)
			assertString(filename, 1)
			local _, _, b = filename:find("^(.*)/[^/]*$")
			if not b then
				return ""
			end
			return b
		end
	)
end

local function replace(files, pattern, repl)
	return dotocollection(files,
		function(filename)
			return filename:gsub(pattern, repl)
		end
	)
end

local function fpairs(...)
	return ipairs(filenamesof(...))
end

local function matching(collection, pattern)
	local o = {}
	dotocollection(collection,
		function(filename)
			if filename:find(pattern) then
				o[#o+1] = filename
			end
		end
	)
	return o
end
		
-- Selects all targets containing at least one output file that matches
-- the pattern (or all, if the pattern is nil).
local function selectof(targets, pattern)
	local targets = targetsof(targets)
	local o = {}
	for k, v in pairs(targets) do
		if v.is and v.outs then
			local matches = false
			for _, f in pairs(v.outs) do
				if f:find(pattern) then
					matches = true
					break
				end
			end
			if matches then
				o[#o+1] = v
			end
		end
	end
	return o
end

local function uniquify(...)
	local s = {}
	return dotocollection({...},
		function(filename)
			if not s[filename] then
				s[filename] = true
				return filename
			end
		end
	)
end

local function startswith(needle, haystack)
	return haystack:sub(1, #needle) == needle
end

local function emit(...)
	local n = select("#", ...)
	local args = {...}

	for i=1, n do
		local s = asstring(args[i])
		io.stdout:write(s)
		if not s:find("\n$") then
			io.stdout:write(" ")
		end
	end
end

local function templateexpand(list, vars)
	vars = inherit(vars, globals)

	local o = {}
	for _, s in ipairs(list) do
		o[#o+1] = s:gsub("%%%b{}",
			function(expr)
				expr = expr:sub(3, -2)
				local chunk, e = load("return ("..expr..")", expr, "text", vars)
				if e then
					error(string.format("error evaluating expression: %s", e))
				end
				local value = chunk()
				if (value == nil) then
					error(string.format("template expression '%s' expands to nil (probably an undefined variable)", expr))
				end
				return asstring(value)
			end
		)
	end
	return o
end
	
local function loadbuildfile(filename)
	if not buildfiles[filename] then
		buildfiles[filename] = true

		local fp, data, chunk, e
		io.stderr:write("loading ", filename, "\n")
		fp, e = io.open(filename)
		if not e then
			data, e = fp:read("*a")
			fp:close()
			if not e then
				local thisglobals = {}
				thisglobals._G = thisglobals
				setmetatable(thisglobals, {__index = globals})
				chunk, e = load(data, "@"..filename, "text", thisglobals)
			end
		end
		if e then
			error(string.format("couldn't load '%s': %s", filename, e))
		end

		local oldcwd = cwd
		cwd = dirname(filename)
		chunk()
		cwd = oldcwd
	end
end

loadtarget = function(targetname)
	if targets[targetname] then
		return targets[targetname]
	end

	local target
	if not targetname:find("%+") then
		local files
		if targetname:find("[?*]") then
			files = posix.glob(targetname)
			if not files then
				error(string.format("glob '%s' matches no files", targetname))
			end
		else
			files = {targetname}
		end

		target = {
			outs = files,
			is = {
				__implicitfile = true
			}
		}
		targets[targetname] = target
	else
		local _, _, filepart, targetpart = targetname:find("^([^+]*)%+([%w-_]+)$")
		if not filepart or not targetpart then
			error(string.format("malformed target name '%s'", targetname))
		end
		if (filepart == "") then
			filepart = cwd
		end
		local filename = concatpath(filepart, "/build.lua")
		loadbuildfile(concatpath(filename))

		target = targets[targetname]
		if not target then
			error(string.format("build file '%s' contains no target '%s'",
				filename, targetpart))
		end
	end

	return target
end

local typeconverters = {
	targets = function(propname, i)
		if (type(i) == "string") then
			i = {i}
		elseif (type(i) ~= "table") then
			error(string.format("property '%s' must be a target list", propname))
		end

		local m = {}
		for k, v in pairs(i) do
			local ts = targetsof(v)
			if (type(k) == "number") then
				for _, t in ipairs(ts) do
					m[#m+1] = t
				end
			else
				if (#ts ~= 1) then
					error(string.format("named target '%s' can only be assigned from a single target", k))
				else
					m[k] = ts[1]
				end
			end
		end
		return m
	end,

	strings = function(propname, i)
		if (type(i) == "string") then
			i = {i}
		elseif (type(i) ~= "table") then
			error(string.format("property '%s' must be a string list", propname))
		end
		return concat(i)
	end,

	boolean = function(propname, i)
		if (type(i) ~= "boolean") then
			error(string.format("property '%s' must be a boolean", propname))
		end
		return i
	end,

	string = function(propname, i)
		if (type(i) ~= "string") then
			error(string.format("property '%s' must be a string", propname))
		end
		return i
	end,

	table = function(propname, i)
		if (type(i) ~= "table") then
			error(string.format("property '%s' must be a table", propname))
		end
		return i
	end,

	object = function(propname, i)
		return i
	end,
}
	
local function definerule(rulename, types, cb)
	if rulename and rules[rulename] then
		error(string.format("rule '%s' is already defined", rulename))
	end

	types.name = { type="string" }
	types.cwd = { type="string", optional=true }
	types.vars = { type="table", default={} }

	for propname, typespec in pairs(types) do
		if not typeconverters[typespec.type] then
			error(string.format("property '%s' has unrecognised type '%s'",
				propname, typespec.type))
		end
	end

	local rulecwd = cwd
	local rule = function(e)
		local definedprops = {}
		for propname, _ in pairs(e) do
			definedprops[propname] = true
		end

		local args = {}
		for propname, typespec in pairs(types) do
			if not e[propname] then
				if not typespec.optional and (typespec.default == nil) then
					error(string.format("missing mandatory property '%s'", propname))
				end

				args[propname] = typespec.default
			else
				args[propname] = typeconverters[typespec.type](propname, e[propname])
				definedprops[propname] = nil
			end
		end

		local propname, _ = next(definedprops)
		if propname then
			error(string.format("don't know what to do with property '%s'", propname))
		end

		if not args.cwd then
			args.cwd = cwd
		end
		args.fullname = args.cwd.."+"..args.name

		local oldparente = parente
		parente = args
		args.vars = inherit(args.vars, oldparente.vars)
		local result = cb(args) or {}
		parente = oldparente

		result.is = result.is or {}
		if rulename then
			result.is[rulename] = true
		end
		result.fullname = args.fullname

		if targets[arg.fullname] and (targets[arg.fullname] ~= result) then
			error(string.format("target '%s' is already defined", args.fullname))
		end
		targets[result.fullname] = result
		return result
	end

	if rulename then	
		if rules[rulename] then
			error(string.format("rule '%s' is already defined", rulename))
		end
		rules[rulename] = rule
	end
	return rule
end

-----------------------------------------------------------------------------
--                              DEFAULT RULES                              --
-----------------------------------------------------------------------------

local function install_make_emitter()
	emit("hide = @\n")
	function emitter:rule(name, ins, outs)
		emit(".INTERMEDIATE:", name, "\n")
		for i = 1, #ins do
			emit(name..":", ins[i], "\n")
		end
		for i = 1, #outs do
			emit(outs[i]..":", name, ";\n")
		end
		emit(name..":\n")

		local dirs = uniquify(dirname(outs))
		if (#dirs > 0) then
			emit("\t@mkdir -p", dirs, "\n")
		end
	end

	function emitter:phony(name, ins, outs)
		emit(".PHONY:", name, "\n")
		self:rule(name, ins, outs)
	end

	function emitter:label(...)
		local s = table.concat({...}, " ")
		emit("\t@echo", s, "\n")
	end

	function emitter:exec(commands)
		for _, s in ipairs(commands) do
			emit("\t$(hide)", s, "\n")
		end
	end

	function emitter:endrule()
		emit("\n")
	end
end

local function install_ninja_emitter()
	emit("rule build\n")
	emit("  command = $command\n")
	emit("\n")

	local function unmake(collection)
		return dotocollection({collection},
			function(s)
				return s:gsub("%$%b()",
					function(expr)
						return "${"..expr:sub(3, -2).."}"
					end
				)
			end
		)
	end

	function emitter:rule(name, ins, outs)
		if (#outs == 0) then
			emit("build", name, ": phony", unmake(ins), "\n")
		else
			emit("build", name, ": phony", unmake(outs), "\n")
			emit("build", unmake(outs), ": build", unmake(ins), "\n")
		end
	end

	function emitter:label(...)
	end

	function emitter:exec(commands)
		emit("  command =", table.concat(unmake(commands), " && "), "\n")
	end

	function emitter:endrule()
		emit("\n")
	end
end

definerule("simplerule",
	{
		ins = { type="targets" },
		outs = { type="strings" },
		deps = { type="targets", default={} },
		label = { type="string", optional=true },
		commands = { type="strings" },
		vars = { type="table", default={} },
	},
	function (e)
		emitter:rule(e.fullname, filenamesof(e.ins, e.deps), e.outs)
		emitter:label(e.fullname, " ", e.label or "")

		local vars = inherit(e.vars, {
			ins = filenamesof(e.ins),
			outs = filenamesof(e.outs)
		})

		emitter:exec(templateexpand(e.commands, vars))
		emitter:endrule()

		return {
			outs = e.outs
		}
	end
)

definerule("installable",
	{
		map = { type="targets", default={} },
	},
	function (e)
		local deps = {}
		local commands = {}
		local srcs = {}
		local dests = {}
		for dest, src in pairs(e.map) do
			if src.is.installable then
				if (type(dest) ~= "number") then
					error("can't specify a destination filename when installing an installable")
				end
				deps[#deps+1] = src.fullname
			elseif (type(dest) == "number") then
				error("only references to other installables can be missing a destination")
			else
				local f = filenamesof(src)
				if (#f ~= 1) then
					error("installable can only cope with targets emitting single files")
				end

				deps[#deps+1] = src.fullname
				dests[#dests+1] = dest
				commands[#commands+1] = "cp "..f[1].." "..dest
			end
		end

		emitter:rule(e.fullname, deps, dests)
		emitter:label(e.fullname, " ", e.label or "")
		if (#commands > 0) then
			emitter:exec(commands)
		end
		emitter:endrule()

		return {
			outs = dests
		}
	end
)

-----------------------------------------------------------------------------
--                               MAIN PROGRAM                              --
-----------------------------------------------------------------------------

local function parse_arguments(argmap, arg)
	local i = 1
	local files = {}

	local function unrecognisedarg(arg)
		argmap[" unrecognised"](arg)
	end

	while (i <= #arg) do
		local o = arg[i]
		local op

		if (o:byte(1) == 45) then
			-- This is an option.
			if (o:byte(2) == 45) then
				-- ...with a -- prefix.
				o = o:sub(3)
				local fn = argmap[o]
				if not fn then
					unrecognisedarg("--"..o)
				end
				i = i + fn(arg[i+1], arg[i+2])
			else
				-- ...without a -- prefix.
				local od = o:sub(2, 2)
				local fn = argmap[od]
				if not fn then
					unrecognisedarg("-"..od)
				end
				op = o:sub(3)
				if (op == "") then
					i = i + fn(arg[i+1], arg[i+2])
				else
					fn(op)
				end
			end
		else
			files[#files+1] = o
		end
		i = i + 1
	end

	argmap[" files"](files)
end

globals = {
	posix = posix,

	abspath = abspath,
	asstring = asstring,
	basename = basename,
	concat = concat,
	concatpath = concatpath,
	cwd = function() return cwd end,
	definerule = definerule,
	dirname = dirname,
	emit = emit,
	filenamesof = filenamesof,
	fpairs = fpairs,
	include = loadbuildfile,
	inherit = inherit,
	print = print,
	replace = replace,
	matching = matching,
	selectof = selectof,
	startswith = startswith,
	uniquify = uniquify,
	vars = vars,
}
setmetatable(globals,
	{
		__index = function(self, k)
			local rule = rules[k]
			if rule then
				return rule
			else
				return _G[k]
			end
		end
	}
)

vars.cflags = {}
parente.vars = vars

setmetatable(_G,
	{
		__index = function(self, k)
			local value = rawget(_G, k)
			if not value then
				error(string.format("access of undefined variable '%s'", k))
			end
			return value
		end
	}
)
		
do
	local emitter_type = install_make_emitter
	parse_arguments(
		{
			["make"] = function()
				emitter_type = install_make_emitter
				return 1
			end,

			["ninja"] = function()
				emitter_type = install_ninja_emitter
				return 1
			end,

			[" unrecognised"] = function(arg)
				error(string.format("unrecognised argument '%s'", arg))
			end,

			[" files"] = function(files)
				emitter_type()
				for _, f in ipairs(files) do
					loadbuildfile(f)
				end
			end
		},
		{...}
	)
end


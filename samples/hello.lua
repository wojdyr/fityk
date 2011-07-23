#!/usr/bin/env lua

-- First, check if the script is called from stand-alone Lua interpreter.
-- The interpreter embedded in libfityk sets global F (an instance of
-- fityk.Fityk) which can be used to access and change the state of the program.
-- In other Lua interpreters we must import fityk module (it can be named
-- liblua-fityk to avoid conflicts and you may need to rename it to fityk.so)
if type(F) ~= 'userdata' or swig_type(F) ~= "fityk::Fityk *" then
    print("we are NOT inside fityk...")
    require("fityk")
    F = fityk.Fityk()
    -- in fityk messages from the program and from F:out go to the output
    -- window, in cfityk to console output, in stand-alone Lua we must
    -- set the output
    F:redir_messages(io.stdout)
    -- the output can be disabled using redir_messages(nil)
end

-- basic checks
print("fityk is", swig_type(fityk)) -- table
print("fityk.Fityk is", swig_type(fityk.Fityk)) -- function

F:out("libfityk version: " .. F:get_info("version"))
F:out("ln(2) = " .. F:calculate_expr("ln(2)"))

-- check error handling
status, err = pcall(function() F:execute("fit") end)
if status == false then
    F:out("Error caught: " .. err)
end

if type(arg) == 'table' then
    directory = string.match(arg[0], "^.*[/\\]")
else
    directory = ""
end
filename = directory .. "nacl01.dat"

F:execute("@0 < '"..filename.."'")
F:out("Data info: "..F:get_info("data", 0))
F:execute("guess %gauss = Gaussian")
F:out("Fitting "..filename.." ...")
F:execute("fit")
F:out("WSSR="..F:get_wssr())
F:out("Gaussian center: "..F:calculate_expr("%gauss.center"))
F:execute("info state >tmp_save.fit")


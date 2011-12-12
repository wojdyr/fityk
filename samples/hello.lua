-- Sample of Lua scripting in Fityk.
--
-- Lua interpreter embedded in libfityk sets global F (an instance of
-- fityk.Fityk) which can be used to access and change the state of the program.
print("swig_type(F): ", swig_type(F)) -- -> stdout

F:out("libfityk version: " .. F:get_info("version")) -- -> fityk output
F:out("ln(2) = " .. F:calculate_expr("ln(2)"))

F:out("Testing error handling...")
status, err = pcall(function() F:execute("fit") end)
if status == false then
    F:out("OK. Error caught: " .. err)
end

-- try to find directory of this file
if type(arg) == 'table' then
    directory = string.match(arg[0], "^.*[/\\]") or ""
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


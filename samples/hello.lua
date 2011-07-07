#!/usr/bin/env lua

-- If this script is run in stand-alone Lua interpreter,
-- it needs fityk module (ln -sf /usr/lib/liblua-fityk.so fityk.so)
-- and the line below:
--require("fityk")

print("fityk is", swig_type(fityk)) -- table
print("fityk.Fityk is", swig_type(fityk.Fityk)) -- function

-- The interpreter embedded in libfityk sets global F (instance of fityk.Fityk)
-- which can be used as to access and change the state of the program.
-- Here, we define a separate engine:
ftk = fityk.Fityk()

print("fityk.Fityk() gives", swig_type(ftk)) -- fityk::Fityk *

print("libfityk version:", ftk:get_info("version"))
print("ln(2) =", ftk:calculate_expr("ln(2)"))

-- check error handling
status, err = pcall(function() ftk:execute("fit") end)
if status == false then
    print("Error: " .. err)
end


GaussianFitter = { f = ftk }

function GaussianFitter:read_data(filename)
    self.f:execute("@0 < '"..filename.."'")
    self.filename = filename
    print("Data info:", self.f:get_info("data", 0))
end

function GaussianFitter:run()
    self.f:execute("guess %gauss = Gaussian")
    print("Fitting "..self.filename.." ...")
    self.f:execute("fit")
    print("WSSR="..self.f:get_wssr())
    print("Gaussian center: ".. self.f:calculate_expr("%gauss.center"))
end

ftk:redir_messages(io.stdout)
-- ftk:redir_messages(nil) -- nil disables output
GaussianFitter:read_data("nacl01.dat")
GaussianFitter:run()
ftk:execute("info state >tmp_save.fit")


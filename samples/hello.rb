#!/usr/bin/env ruby

# Example of using libfityk from ruby.
# To run this example, you need fityk extension for Ruby: after compiling
# fityk go to src/, run "make ruby" and copy swig/fityk.so.

require 'fityk'

class GaussianFitter < Fityk::Fityk
    def initialize(filename)
        super()
        raise "File `#{filename}' not found." unless File::exists? filename
        @filename = filename
        execute("@0 < '#{filename}'")
        puts "Data info: " + get_info("data", 0)
    end

    def run()
        execute("guess %gauss = Gaussian")
        puts "Fitting #{@filename}..."
        execute("fit")
        puts "WSSR=%g" % get_wssr()
        puts "Gaussian center: %.5g" % calculate_expr("%gauss.center")
    end

    def save_session(filename)
        execute("info state >'%s'" % filename)
    end
end

f = Fityk::Fityk.new
puts f.get_info("version")
puts "ln(2) = %.9f" % f.calculate_expr("ln(2)")
f = nil

gauss = GaussianFitter.new("nacl01.dat")
gauss.run()
gauss.save_session("tmp_save.fit")


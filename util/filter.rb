#!/usr/bin/env ruby

# Debug tool bpmelli 7/14/17
# Print out radii and values for the given coeff and level (default VTC0, level 4)

require 'optparse'

class ArgParser < Hash
  def initialize(argv)
    OptionParser.new do |opts|
      opts.banner = 'Usage: filter -l <level> -coeff <coeff> [-o <filtered file>] <csv file>+'
      opts.on('-o', '--output <output file>',   'output file') { |v| self[:output] = v }
      opts.on('-l', '--level <level>',   'level')              { |v| self[:level] = v }
      opts.on('-c', '--coeff <coefficient>',   'coefficient')  { |v| self[:coeff] = v }
      opts.on('-t', '--time <time index>',   'time')           { |v| self[:time] = v }
      
    end.parse!
  end
end

opts = ArgParser.new(ARGV)

opts[:level] = 4      unless opts.has_key?(:level)
opts[:coeff] = 'VTC0' unless opts.has_key?(:coeff)

if opts.has_key?(:output)
  out = File.open(opts[:output], 'w')
else
  out = $stdout.dup
end

out.puts "# #{opts[:coeff]} at level #{opts[:level]} at each radius"
time_index = -1

ARGF.each do |line|
  if line =~ /^# Vortex time:/
    time_index += 1
    next if opts.has_key?(:time) and time_index != opts[:time].to_i
    out.puts line
  end
  next if opts.has_key?(:time) and time_index != opts[:time].to_i
  if line =~ /^#{opts[:level]},(\d+),#{opts[:coeff]},(.*)/
    out.puts "#{$1}\t#{$2}"
  end
end

out.close

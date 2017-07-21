#!/usr/bin/env ruby

# Read a NetCDF file of coefficients
# extract given coefficient at the given time index and level

require 'pp'
require 'optparse'
require 'numru/netcdf'
include NumRu


class ArgParser < Hash
  def initialize(argv)
    OptionParser.new do |opts|
      opts.banner = 'Usage: coeff2ncdf -i <xml file> -o <NetCDF file>'
      opts.on('-i', '--input <netcdf file>', 'netcdf File') { |v| self[:input] = v }
      opts.on('-c', '--coeff <string>', 'coeff name')       { |v| self[:coeff] = v }
      opts.on('-l', '--level <n>', 'level number')          { |v| self[:level] = v }
      opts.on('-t', '--time <n>', 'time index')             { |v| self[:time] = v }
    end.parse!
  end
end

opts = ArgParser.new(ARGV)

# Make sure the required options are given

[:input, :coeff, :level, :time].each do |arg|
  unless opts.has_key?(arg)
    puts "option '#{arg}' is required. Use the -h option for usage"
    exit 1
  end
end

level = opts[:level].to_i
time  = opts[:time].to_i
coeff = opts[:coeff]

puts "Extracting '#{coeff}' at level #{level}, time #{time}"

nc = NumRu::NetCDF.open(opts[:input], 'r')
vals = nc.var(coeff)
values = vals.get
nc.close

shape = values.shape
pp shape

radius_max   = shape[0] - 1
level_max  = shape[1] - 1
time_max = shape[2] - 1

puts "level: #{level_max}, radius: #{radius_max}, time: #{time_max}"

(0..time_max).each do |t|
  next unless t == time
  puts "time: #{t}"
  (0..level_max).each do |l|
    next unless level == l
    puts "   level: #{l}"
    (0..radius_max).each do |radius|
      puts "    (#{time}, #{level}, #{radius} )  #{values[radius, level, time]}"
    end
  end
end

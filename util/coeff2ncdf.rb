#!/usr/bin/env ruby

# Read a vortrac *_simplexlist.xml file
# Write a NetCDF file

require 'date'
require 'optparse'
require 'numru/netcdf'
include NumRu

require 'date'
require 'pp'

FILL_VALUE = -999.0

class ArgParser < Hash
  def initialize(argv)
    OptionParser.new do |opts|
      opts.banner = 'Usage: coeff2ncdf -i <xml file> -o <NetCDF file>'
      opts.on('-i', '--input <xml file>', 'xml File')           { |v| self[:input] = v }
      opts.on('-o', '--output <NetCDF file>',   'NetCDF file')  { |v| self[:output] = v }
      opts.on('-d', '--debug <coeff, time, level>',   'debug string')  { |v| self[:debug] = v }
    end.parse!
  end
end

# Keys to these hashes are the parameter values.
# Value is their index.

h_coeffs = {}
h_levels = {}
h_radii = {}

# No need to use a hash for the time values since we can just append them
# as we find them

times  = []

# Push the item if it isn't already in the table
# Return the value of the index (which is its position in the table

def push(table, value)
  table[value] = table.size unless table.has_key?(value)
  return table[value]
end

# This is going to be a hash of 3 dimentional table of values.
# It is expanded and filled as needed by the store function
#
# This will end up as [time][level][radius] in the netcdf file

coeffHash = Hash.new { |h, k| h[k] = [] }

# Create an array if it doesn't exist

def create_entry(table, time, level, radius)
  table[time] = [] if table[time].nil?
  table[time][level] = [] if table[time][level].nil?
end

# Store a coefficient value

def store(coeffs, time, level, radius, coeff, value)
  create_entry(coeffs[coeff], time, level, radius)
  coeffs[coeff][time][level][radius] = value
end

# Fill up holes in the arrays

def fill(table, times, levels, radii, fill_value)
    (0..times - 1).each do |time|
      (0..levels - 1).each do |level|
        (0..radii - 1).each do |radius|
          create_entry(table, time, level, radius)
          table[time][level][radius] = fill_value if
            table[time][level][radius].nil?
        end
      end
    end
end

# h_radii might look like [ 1.5 -> 0, 2.0 -> 1, 1.0 -> 2 ]
# h_levels might also be out of order. We want to return an array where levels and radii are incrementing

#        puts "time #{time} level #{level} radius #{radius}"
#        puts "   h_levels[level] #{h_levels[level]} h_radii[radius] #{h_radii[radius]}"

def reorder(table, times, h_levels, h_radii)
  # retval = NArray.float(times.size, h_levels.size, h_radii.size)
  retval = []
  l_keys = h_levels.keys.sort
  r_keys = h_radii.keys.sort
  (0..times.size - 1).each do |time|
    (0..h_levels.size - 1).each do |level|
      (0..h_radii.size - 1).each do |radius|
        create_entry(retval, time, level, radius)
        retval[time][level][radius] = table[time][h_levels[l_keys[level]]][h_radii[r_keys[radius]]]
        # retval[time, level, radius] = table[time][h_levels[l_keys[level]]][h_radii[r_keys[radius]]]
      end
    end
  end
  retval
end

def debug(str, coeffs)  # coeff, time, level
  unless str =~ /([^,\s]+)\s*,\s*([^,\s]+)\s*,\s*([^,\s]+)/
    puts "Invalid debug string '#{str}'"
    return
  end

  coeff = $1
  time = $2.to_i
  level = $3.to_i

  puts "#{coeff}, time #{time}, level #{level}"
  
  unless coeffs.has_key?(coeff)
    puts "Illegal coeff '#{coeff}'"
    return
  end

  # data looks like coeffs["VTC0"][radius][level][time]

  max_time = values.size - 1
  max_level = values[0].size - 1
  max_radius = values[0][0].size - 1
  
  values = coeffs[coeff]
  vals = []

  (0..max_time).each do |itime|
    next unless itime == time
    (0..max_level).each do |ilevel|
      next unless ilevel == level
      (0..max_radius).each do |iradius|
        vals.push coeffs[coeff][itime][ilevel][iradius]
      end
    end
  end
  (0..vals.size - 1).each do |index|
    puts "#{index}\t#{vals[index]}"
  end
end

# Main program

opts = ArgParser.new(ARGV)

unless opts.has_key?(:input)
  puts '-i option is required. Use the -h option for usage'
  exit 1
end

unless opts.has_key?(:output)
  puts '-o option is required. Use the -h option for usage'
  exit 1
end

unless File.file?(opts[:input])
  puts "Cannot read #{opts[:input]})"
  exit 1;
end

time_index = -1

# Process the file one line at a time

File.open(opts[:input]).each do |line|

  # New time stamp.
  
  if line =~ /^# Vortex time:\s+(\d+)-(\d+)-(\d+):(\d+):(\d+)/
    year = $1.to_i
    month = $2.to_i
    day = $3.to_i
    hour = $4.to_i
    mins = $5.to_i
    
    times << DateTime.new(year, month, day, hour, mins).strftime('%s').to_i
    time_index += 1
    next
  end

  # Skip comments
  
  next if line =~ /^#/

  # Line format: level,radius,param,value
  
  if line =~ /([^,]+),([^,]+),([^,]+),(\S+)/
    level = $1.to_f
    radius = $2.to_f
    param = $3
    value = $4.to_f
    value = FILL_VALUE if 
    s = $4 =~ /-?0\.?0*\s*$/
      
    store(coeffHash, time_index, 
          push(h_levels, level),
          push(h_radii, radius), 
          param, value)
  end
end

debug(opts[:debug], coeffHash) if opts.has_key?(:debug)

# netcdf doesn't like missing entries
# def_fill(coeffHash, times.size(), h_coeffs.size(), h_levels.size(), h_radii.size(), FILL_VALUE)

# Done with processing the input.
# Create and fill the NetCDF file

file = NetCDF.create(opts[:output], false)

# Dimentions

timeDim  = file.def_dim('ntimes', times.size())
levelDim = file.def_dim('nlevels', h_levels.size())
radiiDim = file.def_dim('nradii', h_radii.size())

# Variables

vlevel = file.def_var('levels', 'sfloat', [levelDim])
vlevel.put_att('min', "#{h_levels.keys[0]}")
vlevel.put_att('max', "#{h_levels.keys[h_levels.size - 1]}")

vtime =  file.def_var('times', 'int', [timeDim])
vtime.put_att('value', 'Seconds since Unix epoch')
vtime.put_att('standard_name', 'time')
vtime.put_att('long_name', 'Data time')
vtime.put_att('units', 'seconds since 1970-01-01T00:00:00Z')
vtime.put_att('bounds', 'time_bounds')

vradius = file.def_var('radii', 'int', [radiiDim])
vradius.put_att('min', "#{h_radii.keys[0]}")
vradius.put_att('max', "#{h_radii.keys[h_radii.size - 1]}")
vradius.put_att('units', 'km')

vHash = {}

coeffHash.keys.each do |coeff|
  vvalues = file.def_var(coeff, 'float', [radiiDim, levelDim, timeDim])
  vvalues.put_att('value', '[time][level][radius]')
  vvalues.put_att('_FillValue', FILL_VALUE)
  vvalues.put_att('missing_value', FILL_VALUE)
  vHash[coeff] = vvalues
end

# Done with defs
file.enddef

# Write the arrays

vlevel.put(h_levels.keys.sort)
vtime.put(times)
vradius.put(h_radii.keys.sort)

# Danger danger: The coeffs are in the order the keys were found, not the order of the sorted keys.
# So if first few entries started at level 1.5, and then toward the end we find level 1, the order
# in the tables are broken. reorder fixes that.

vHash.keys.each do |coeff|
  fill(coeffHash[coeff], times.size, h_levels.size, h_radii.size, FILL_VALUE)
  ordered = reorder(coeffHash[coeff], times, h_levels, h_radii)
  vHash[coeff].put(ordered)
end

# ALl done

file.close

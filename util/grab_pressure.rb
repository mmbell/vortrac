#!/usr/bin/env ruby

# Retrieve pressure information for a specified time range
# To be used with VORTRAC in manual mode.
# Bruno Melli 5/26/17

# TODO
# I meant to grab pressure data over the span of the radar data. But I can't find a server with historical data.
# So the code doesn't have a loop over the interval grabbing data at multiple times.
# It also doesn't post-process the data into a file usable by Vortrac.

require 'net/http'
require 'uri'

require 'date'
require 'optparse'
require 'nokogiri'
require 'pp'

require 'date'

class ArgParser < Hash
  
  def initialize(argv)
    OptionParser.new do |opts|
      opts.banner = 'Usage: grab_pressure -c <config file> [optional arguments'

      opts.on('-c', '--config <xml file>', 'Config File')  { |v| self[:config] = v }
      opts.on('-i', '--interval <secs>',   'Interval')     { |v| self[:interval] = v }
      opts.on('-s', '--start <date>',      'Start Time')   { |v| self[:start] = v }
      opts.on('-e', '--end <data>',        'End Time')     { |v| self[:end] = v }      
    end.parse!
  end
  
end

opts = ArgParser.new(ARGV)
unless opts.has_key?(:config)
  puts '-c option is required. Use the -h option for usage'
  exit 1
end

# Grab item of interest from the config file.
# TODO allow these to be given on the command line

doc = File.open(opts[:config]) { |f| Nokogiri::XML(f) }

dest = doc.xpath('//vortrac/pressure/dir').first.content
server_url = doc.xpath('//vortrac/pressure/madisurl').first.content
if server_url =~ /^(https?:\/\/)(.*)/
  prefix = $1
  server = $2
else
  prefix = 'http://'
  server = server_url
end

user = doc.xpath('//vortrac/pressure/madisuser').first.content
passwd = doc.xpath('//vortrac/pressure/madispassword').first.content

vortex_lat = doc.xpath('//vortrac/vortex/lat').first.content.to_f
vortex_lon = doc.xpath('//vortrac/vortex/lon').first.content.to_f

radar_lat = doc.xpath('//vortrac/radar/lat').first.content.to_f
radar_lon = doc.xpath('//vortrac/radar/lon').first.content.to_f

start_date = doc.xpath('//vortrac/radar/startdate').first.content
start_time = doc.xpath('//vortrac/radar/starttime').first.content
end_date = doc.xpath('//vortrac/radar/enddate').first.content
end_time = doc.xpath('//vortrac/radar/endtime').first.content

latll = radar_lat - 2.5
latur = radar_lat + 2.5
lonll = radar_lon - 2.5
lonur = radar_lon + 2.5

# Deal with the starting date. Format: YYYYMMDD_HHMM

date = DateTime.strptime("#{start_date}:#{start_time}", '%Y-%m-%d:%H:%M:%S')
time_str = date.strftime('%Y%m%d_%H%M')

puts "Grabbing data at #{time_str}"

dataurl = "madisPublic/cgi-bin/madisXmlPublicDir?rdr=&time=" + time_str +
          "&minbck=-45&minfwd=0&recwin=3&dfltrsel=1&latll=" +
          latll.to_s + "&lonll=" + lonll.to_s + "&latur=" + latur.to_s + "&lonur=" + lonur.to_s +
          "&stanam=&stasel=0&pvdrsel=0&varsel=1&qcsel=2&xml=2&csvmiss=0&nvars=P" +
          "&nvars=DD&nvars=FF&nvars=ELEV&nvars=LAT&nvars=LON";

full_url = "#{prefix}#{user}:#{passwd}@#{server}/#{dataurl}"
puts "Fetching data from #{full_url}"

uri = URI.parse(full_url)
response = Net::HTTP.get_response uri
pp response.body


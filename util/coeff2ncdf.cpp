// Generate a netcdf file from a csv file containing coefficients

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <map>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <netcdfcpp.h>

// a string splitting utility

template<typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

// Info is used for doing a first pass on the coeff file and finding out the number of things.

class Info {

public:
  
  Info(char *fname);
  bool valid() { return valid_flag; }
  
  int get_num_coeffs() { return num_coeffs; }
  int get_num_times()  { return num_times; }
  int get_num_levels() { return num_levels; }
  int get_num_radii()  { return num_radii; }

  float get_min_radius() { return min_radius; }
  float get_max_radius() { return max_radius; }

  float get_min_level() { return min_level; }
  float get_max_level() { return max_level; }

  char *get_fname()     { return input_path; }
  
private:
  
  char *input_path;
  bool valid_flag;
  
  int num_coeffs;
  int num_times;
  int num_levels;
  int num_radii;

  float min_radius;
  float max_radius;

  float min_level;
  float max_level;

  std::map<float, float> levels;
  std::map<float, float> radii;
  std::map<std::string, std::string> coeffs;
  
};

Info::Info(char *fname)
{
  input_path = fname;
  valid_flag = true;
  num_coeffs = 0;
  num_times = 0;
  num_levels = 0;
  num_radii = 0;

  min_radius = 999;
  max_radius = -999;
  
  min_level = 999;
  max_level = -999;
  
  std::ifstream file(fname);
  if (! file.good()) {
    std::cerr << "Unable to read '" << fname << "'" << std::endl;
    valid_flag = false;
    return;
  }
  std::string line;
  while (std::getline(file, line)) {
    if ( line.find("Vortex time:") != std::string::npos) {
      num_times += 1;
      continue;
    }
    if ( line.find("#") == 0)
      continue;
    
    std::vector<std::string> words = split(line, ',');

    if (words.size() != 4) {
      std::cerr << "Unexpected line '" << line.data() << "'" << std::endl;
      continue;
    }

    float level = std::stof(words[0]);
    float radius = std::stof(words[1]);
    double value = std::stod(words[3]);

    if ( coeffs.find(words[2]) == coeffs.end() ) {
      coeffs[words[2]] = words[2];
      num_coeffs += 1;
    }
    if ( levels.find(level) == levels.end() ) {
      num_levels += 1;
      levels[level] = level;
      if (level < min_level)
	min_level = level;
      if(level > max_level)
	max_level = level;
    }
    if ( radii.find(radius) == radii.end() ) {
      num_radii += 1;
      radii[radius] = radius;
      if (radius < min_radius)
	min_radius = radius;
      if(radius > max_radius)
	max_radius = radius;
    }
  }
}

class Data
{
  
public:
  
  Data(Info &info);
  ~Data();
  bool parse();

private:

  std::vector<float> levels;
  std::vector<float> radii;
  std::vector<long>  times;
};

Data::~Data()
{
}

Data::Data(Info &info)
{
}

bool Data::parse()
{
  return true;
}

void usage(char *s)
{
  std::cerr << "Usage: " << s << " -i <input csv file> -o <output netcdf file>" << std::endl;
}

int main(int argc, char *argv[])
{
  int opt;
  char *in = NULL, *out = NULL;
  
  while( (opt = getopt(argc, argv, "i:o:h")) != EOF)
    switch(opt){
    case 'i':
      in = strdup(optarg);
      break;
    case 'o':
      out = strdup(optarg);
      break;
    case 'h':
    case '?':
      usage(argv[0]);
      exit(0);
    }

  // Make sure we have the right options

  if ( (in == NULL) || (out == NULL) ) {
    usage(argv[0]);
    exit(1);
  }

  Info info(in);
  if ( ! info.valid() )
    exit(1);

   
  std::cout << "Number of times: " << info.get_num_times() << std::endl;
  std::cout << "Number of levels: " << info.get_num_levels() << std::endl;
  std::cout << "Number of radii: " << info.get_num_radii() << std::endl;
  std::cout << "Number of coeffs: " << info.get_num_coeffs() << std::endl;

  Data data(info);
}


#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <regex>
#include <vector>
#include <string>

#include <tclap/CmdLine.h>

// Requires gif.h: https://github.com/ginsweater/gif-h
#include "gif.h"


using namespace std;


const string VERSION = "0.0.0";


double min_x = numeric_limits<double>::max();
double max_x = numeric_limits<double>::lowest();
double min_y = numeric_limits<double>::max();
double max_y = numeric_limits<double>::lowest();
size_t max_type = 0;


struct Point {
  double x;
  double y;
  size_t type;
};

struct Frame {
  double time;
  vector<Point> points;
};

vector<Frame> parse_input(string filename) {
  fstream f;
  f.open(filename, fstream::in);
  string s;
  regex re_time(R"((\d+.?\d*e?-?\d*))");
  regex re_point(R"(([A-Z])\((-?\d+.?\d*e?-?\d*), (-?\d+.?\d*e?-?\d*)\))");
  vector<Frame> frames;
  while (getline(f, s)) {
    Frame frame;
    smatch m;
    regex_search(s, m, re_time);
    frame.time = stod(m[1]);
    while (regex_search(s, m, re_point)) {
      size_t type = static_cast<size_t>(string(m[1])[0] - 'A');
      double x = stod(m[2]);
      double y = stod(m[3]);
      min_x = min(min_x, x);
      max_x = max(max_x, x);
      min_y = min(min_y, y);
      max_y = max(max_y, y);
      max_type = max(type, max_type);
      frame.points.push_back(Point{x, y, type});
      s = m.suffix().str();
    }
    frames.push_back(frame);
  }

  double wx = (max_x - min_x) / 20.0;
  double wy = (max_y - min_y) / 20.0;

  min_x -= wx;
  max_x += wx;
  min_y -= wy;
  max_y += wy;

  return frames;
}


struct Arguments {
  string infile;
  string outfile;
  int width;
  int height;
  int delay;
  int frameskip;
};


double field(double r) {
  double f = 1.0 - r;
  f = max(f, 0.0);
  return f;
}

double distance2(const Point &a, const Point &b) {
  return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
}
double distance(const Point &a, const Point &b) {
  return sqrt(distance2(a, b));
}



int main(int argc, char **argv) {
  Arguments a;
  try {
    TCLAP::CmdLine cmd("General treatment simulator", ' ', VERSION);

    TCLAP::ValueArg<string> a_infile("i", "infile", "Timeline of cell positions", true, "n/a", "file with lines of: \"[Time] [Type]([XCOORD], [YCOORD]), [TYPE]([XCOORD], [YCOORD]), ...\"", cmd);
    TCLAP::ValueArg<string> a_outfile("o", "outfile", "Filename of output gif", true, "n/a", "filename", cmd);
    TCLAP::ValueArg<int> a_width("x", "width", "Width of output in pixels", false, 640, "integer", cmd);
    TCLAP::ValueArg<int> a_height("y", "height", "Height of output in pixels", false, 480, "integer", cmd);
    TCLAP::ValueArg<int> a_delay("d", "delay", "Delay between frames in milliseconds", false, 17, "integer", cmd);
    TCLAP::ValueArg<int> a_frameskip("f", "frameskip", "Frames to skip between rendered frames", false, 0, "integer", cmd);

    cmd.parse(argc, argv);

    a.infile = a_infile.getValue();
    a.outfile = a_outfile.getValue();
    a.width = a_width.getValue();
    a.height = a_height.getValue();
    a.delay = a_delay.getValue();
    a.frameskip = a_frameskip.getValue();

  } catch (TCLAP::ArgException &e) {
    cerr << "TCLAP Error: " << e.error() << endl << "\targ: " << e.argId() << endl;
    return 1;
  }

  auto frames = parse_input(a.infile);

  double xw = (max_x - min_x);
  double yw = (max_y - min_y);

  cout << min_x << ' ' << max_x << endl;
  cout << min_y << ' ' << max_y << endl;

  GifWriter gif;
  GifBegin(&gif, a.outfile.c_str(), a.width, a.height, a.delay);

  vector<uint8_t> buffer(a.width * a.height * 4, 0);
  cout << buffer.size() << endl;

  for (size_t i = 0; i < frames.size(); i += 1 + a.frameskip) {
    cout << "\rRendering frame " << i << "/" << frames.size() - 1;
    cout.flush();

    for (int y = 0; y < a.height; ++y) {
      for (int x = 0; x < a.width; ++x) {
        // double *f = new double[max_type + 1];
        vector<double> f(max_type + 1, 0.0);
        Point here{
              min_x + static_cast<double>(x) / static_cast<double>(a.width) * xw
            , min_y + static_cast<double>(y) / static_cast<double>(a.height) * yw
            , 0
            };
        // cout << here.x << ' ' << here.y << endl;
        for (auto &p: frames[i].points) {
          f[p.type] += field(distance2(here, p));
        }
        double total_f = accumulate(f.begin(), f.end(), 0.0);
        cout << x << ' ' << y << ' ' << total_f << endl;
        cout << y*a.width + x << endl;
        buffer[(y*a.width + x)*4] = total_f * numeric_limits<uint8_t>::max();
        // buffer[x*a.height + y] = total_f * numeric_limits<uint8_t>::max();
        // delete f;
      }
    }

    GifWriteFrame(&gif, buffer.data(), a.width, a.height, a.delay);
    cout << "frame done" << endl;
    // SDL_RenderDrawPoint(renderer, (point.x - min_x) / xw * WIDTH, (point.y - min_y) / yw * HEIGHT);
  }

  GifEnd(&gif);

  cout << endl;
}

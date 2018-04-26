#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <vector>
#include <string>

#include <SDL2/SDL.h>
#include <tclap/CmdLine.h>


using namespace std;


const string VERSION = "0.0.0";


double min_x = numeric_limits<double>::max();
double max_x = numeric_limits<double>::lowest();
double min_y = numeric_limits<double>::max();
double max_y = numeric_limits<double>::lowest();


const int WIDTH = 1024;
const int HEIGHT = 768;


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
    cout << s << '\n';
    smatch m;
    regex_search(s, m, re_time);
    frame.time = stod(m[1]);
    cout << frame.time << '\n';
    while (regex_search(s, m, re_point)) {
      for (auto p: m) {
        cout << p << ' ';
      } cout << '\n';
      size_t type = static_cast<size_t>(string(m[1])[0] - 'A');
      double x = stod(m[2]);
      double y = stod(m[3]);
      min_x = min(min_x, x);
      max_x = max(max_x, x);
      min_y = min(min_y, y);
      max_y = max(max_y, y);
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
  // if (min_x < 0)
  //   min_x *= 1.1;
  // else
  //   min_x *= 0.9;
  // if (max_x < 0)
  //   max_x *= 0.9;
  // else
  //   max_x *= 1.1;
  // if (min_y < 0)
  //   min_y *= 1.1;
  // else
  //   min_y *= 0.9;
  // if (max_y < 0)
  //   max_y *= 0.9;
  // else
  //   max_y *= 1.1;

  return frames;
}


struct Arguments {
  string filename;
};


int main(int argc, char **argv) {
  Arguments a;
  try {
    TCLAP::CmdLine cmd("General treatment simulator", ' ', VERSION);

    TCLAP::ValueArg<string> a_file("i", "forestfile", "Forest of cell growth", true, "n/a", "; separated trees", cmd);

    cmd.parse(argc, argv);

    a.filename = a_file.getValue();

  } catch (TCLAP::ArgException &e) {
    cerr << "TCLAP Error: " << e.error() << endl << "\targ: " << e.argId() << endl;
    return 1;
  }

  auto frames = parse_input(a.filename);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("Point visualization"
                                        , SDL_WINDOWPOS_UNDEFINED
                                        , SDL_WINDOWPOS_UNDEFINED
                                        , WIDTH
                                        , HEIGHT
                                        , SDL_WINDOW_SHOWN
                                        );
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_Event event;


  cout << min_x << ' ' << max_x << endl;
  cout << min_y << ' ' << max_y << endl;

 BEGIN_FRAMES:

  for (auto frame: frames) {
    SDL_SetRenderDrawColor(renderer, 0x6, 0x18, 0x20, 0xFF);
    SDL_RenderClear(renderer);

    for (auto point: frame.points) {
      double xw = (max_x - min_x);
      double yw = (max_y - min_y);
      switch (point.type) {
      case 0:
        SDL_SetRenderDrawColor(renderer, 0xAA, 0xAA, 0x3A, 0xFF);
        break;
      case 1:
        SDL_SetRenderDrawColor(renderer, 0xAA, 0x3A, 0xAA, 0xFF);
      }
      SDL_RenderDrawPoint(renderer, (point.x - min_x) / xw * WIDTH, (point.y - min_y) / yw * HEIGHT);
    }

    SDL_RenderPresent(renderer);

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        goto END_FRAMES;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_r) {
          goto BEGIN_FRAMES;
        }
      }
    }

    SDL_Delay(10);
  }


  while (true) {
    // Keep displaying until user quits
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        goto END_FRAMES;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_r) {
          goto BEGIN_FRAMES;
        }
      }
    }

    SDL_Delay(16);
  }

  // Breaking out of nested loops is an ok usage right?
 END_FRAMES:

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

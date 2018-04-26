#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <regex>
#include <string>
#include <vector>

#include <tclap/CmdLine.h>


using namespace std;


const string VERSION = "0.0.0";


struct Node {
  Node(size_t type, double birthtime=0.0)
    : type(type)
    , birthtime(birthtime) { }

  size_t type;
  double birthtime;
  shared_ptr<Node> left = nullptr;
  shared_ptr<Node> right = nullptr;
};

std::ostream &operator<<(std::ostream &out, const std::shared_ptr<Node> n) {
  if (n->left == nullptr && n->right == nullptr) {
    out << static_cast<char>('A' + n->type) << ':' << n->birthtime;
  } else {
    out << '(' << n->left << ',' << n->right << ')' << static_cast<char>('A' + n->type) << ':' << n->birthtime;
  }
  return out;
}


size_t name_to_number(string s) {
  assert(s.size() == 1);
  return static_cast<size_t>(s[0] - 'A');
}


// TODO memory usage is terrible like this, due to excessive string replication
// but at the moment, regex does not support string_view, and I don't want to create a workaround
shared_ptr<Node> parse_tree(string s, double time) {
  // cout << endl;
  static regex re_parens(R"([\(\)])");
  static regex re_leaf(R"(([A-Z]):(\d+\.?\d*e?-?\d*))");
  // static regex re_branch(R"(\((.*)\)([A-Z]):(\d+\.?\d*e?-?\d*))");
  // cout << s << endl;
  // check if we have a leaf (recursion end condition)
  smatch m;
  if (!regex_search(s, m, re_parens)) {
    // cout << "LEAF!" << endl;
    // return a leaf node
    regex_match(s, m, re_leaf);
    // cout << m[1] << '\t' << m[2] << endl;
    double dt = stod(m[2]);
    // cout << name_to_number(m[1]) << '\t' << time + dt << endl;
    return make_shared<Node>(name_to_number(m[1]), time + dt);
  }

  // otherwise, recursively parse further branches
  // cout << "not leaf" << endl;
  // cout << s.size() << endl;
  int end = s.size() - 1;
  while (s[end] != ')')
    --end;
  string ss(s, 1, end - 1);
  string infosec(s, end + 1, s.size() - end - 1);
  regex_match(infosec, m, re_leaf);
  // cout << m[1] << '\t' << m[2] << endl;
  double dt = stod(m[2]);
  auto branch = make_shared<Node>(name_to_number(m[1]), time + dt);
  // cout << ss << endl;
  // cout << infosec << endl;

  // now split by central comma
  int n_paren = 0;
  size_t i = 0;
  while (i < ss.size()) {
    if (ss[i] == ',' && n_paren == 0)
      break;
    else if (ss[i] == '(')
      ++n_paren;
    else if (ss[i] == ')')
      --n_paren;
    i++;
  }
  string left(ss, 0, i);
  string right(ss, i + 1, ss.size() - i);
  // remove outermost parenthesis if present
  // if (left[0] == '(' && left[left.size() - 1] == ')')
  //   left = string(left, 1, left.size() - 2);
  // if (right[0] == '(' && right[right.size() - 1] == ')')
  //   right = string(right, 1, right.size() - 2);
  // unneccessary
  // cout << left << '\t' << right << endl;
  branch->left = parse_tree(left, time + dt);
  branch->right = parse_tree(right, time + dt);
  return branch;
  // return nullptr;
}


vector< shared_ptr<Node> > parse_forest(const string &s) {
  // First, split forest into trees (semicolon delimited)
  vector< shared_ptr<Node> > forest;
  auto it = s.begin();
  auto itp = it;
  do {
    it = find(itp, s.end(), ';');
    if (it == itp)
      break;
    forest.push_back(parse_tree(string(itp, it), 0.0));
    itp = it;
    ++itp;
  } while (it != s.end());

  return forest;
}


struct Point {
  double x;
  double y;
  shared_ptr<Node> cell = nullptr;
};
struct Vector {
  double x;
  double y;
};


ostream &operator<<(ostream &out, Point p) {
  out << static_cast<char>('A' + p.cell->type) << '(' << p.x << ", " << p.y << ')';
  return out;
}
ostream &operator<<(ostream &out, Vector p) {
  out << '(' << p.x << ", " << p.y << ')';
  return out;
}

// Parameters for the Lennard-Jones potential
constexpr double sigma = 0.2;
constexpr double epsilon4 = 20.0 * 4.0;
constexpr double sigma6 = pow(sigma, 6.0);

// Potential between a and b
double lj_potential(Point a, Point b) {
  double r2 = (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
  double r6 = pow(r2, 3.0);
  double sbyr6 = sigma6 / r6; // I wonder if reusing r2 here is faster? (avoids allocation?)
  return epsilon4 * (sbyr6*sbyr6 - sbyr6);
}


Vector set_magnitude(Vector v, double m) {
  double theta;
  if (v.x == 0)
    theta = 1.571;
  else
    theta = atan(v.y / v.x);
  if (v.y < 0)
    theta += 3.14159;
  Vector n;
  n.x = cos(theta) * m;
  n.y = sin(theta) * m;
  return n;
}


// Force on a because of potential between a and b
Vector lj_force(Point a, Point b) {
  // cout << a << '\t' << b << endl;
  double r2 = (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
  if (r2 <= 0.0)
    return Vector {0.01, 0.01}; // prevent particles sticking together if they land exactly on top (unlikely)
  // cout << "dist " << r2 << endl;
  double r6 = pow(r2, 3.0);
  double f =  epsilon4 * 6.0 * sigma6 * (r6 - 2.0 * sigma6) / (r6 * r6 * sqrt(r2));
  Vector q {b.x - a.x, b.y - a.y};
  double qmag = sqrt(q.x*q.x + q.y*q.y);
  // cout << q << '\t' << f << '\t' << qmag << endl;
  q.x /= qmag;
  q.y /= qmag;
  q.x *= f;
  q.y *= f;
  // cout << q << endl;
  return q;
}


Vector cap_velocity(Vector v, double cap) {
  double mag = sqrt(v.x*v.x + v.y*v.y);
  // cout << mag << endl;
  if (mag <= cap)
    return v;
  // cout << 'n' << endl;
  // double theta;
  // if (v.x == 0)
  //   theta = 1.571;
  // else
  //   theta = atan(v.y / v.x);
  // if (v.y < 0)
  //   theta += 3.14159;
  Vector n;
  n.x = v.x * cap / mag;
  n.y = v.y * cap / mag;
  // n.x = cos(theta) * cap;
  // n.y = sin(theta) * cap;
  return n;
}



struct Arguments {
  string forest;
  string forestfile;
  double end_time;
  double timestep;
  double friction;
  double max_velocity;
  double max_acceleration;
};


int main(int argc, char **argv) {

  Arguments a;
  try {
    TCLAP::CmdLine cmd("General treatment simulator", ' ', VERSION);

    TCLAP::ValueArg<string> a_forest("f", "forest", "Forest of cell growth", false, "n/a", "; separated trees", cmd);
    TCLAP::ValueArg<string> a_forestfile("i", "forestfile", "Forest of cell growth", false, "n/a", "; separated trees", cmd);
    TCLAP::ValueArg<double> a_end_time("t", "endtime", "Max time to run physics", false, 10.0, "double", cmd);
    TCLAP::ValueArg<double> a_timestep("d", "timestep", "Physics timestep", false, 0.005, "double", cmd);
    TCLAP::ValueArg<double> a_friction("r", "friction", "Particle friction multiplier", false, 0.95, "double", cmd);
    TCLAP::ValueArg<double> a_max_velocity("v", "max-velocity", "Max particle velocity", false, 3.0, "double", cmd);
    TCLAP::ValueArg<double> a_max_acceleration("a", "max-acceleration", "Max particle acceleration", false, 1.5, "double", cmd);

    cmd.parse(argc, argv);

    a.forest = a_forest.getValue();
    a.forestfile = a_forestfile.getValue();
    a.end_time = a_end_time.getValue();
    a.timestep = a_timestep.getValue();
    a.friction = a_friction.getValue();
    a.max_velocity = a_max_velocity.getValue();
    a.max_acceleration = a_max_acceleration.getValue();

    if (a.forest == "n/a" && a.forestfile == "n/a") {
      // cout << "provide a forest directly or in a file via -f or -i" << endl;
      return 0;
    }
    if (a.forest != "n/a" && a.forestfile != "n/a") {
      // cout << "two forests provided, use -i or -f, not both" << endl;
      return 0;
    }
    if (a.forest =="n/a") {
      fstream f;
      f.open(a.forestfile, fstream::in);
      getline(f, a.forest);
    }

  } catch (TCLAP::ArgException &e) {
    cerr << "TCLAP Error: " << e.error() << endl << "\targ: " << e.argId() << endl;
    return 1;
  }


  // Parse record of branching process
  auto forest = parse_forest(a.forest);
  // cout << endl;
  // cout << "Resulting trees:" << endl;
  // for (auto t: forest) cout << t << endl;
  // exit(0);


  // Set up starting particles
  vector<Point> points;
  {
    int box_edge = ceil(sqrt(forest.size()));
    double xx = -box_edge / 4.0;
    double yy = -box_edge / 4.0;
    for (size_t i = 0; i < forest.size(); ++i) {
      points.push_back(Point{xx, yy, forest[i]});
        xx += sigma;
      if (xx >= box_edge / 2.0) {
        xx = -box_edge / 4.0 + 0.5;
        yy += sigma * 0.866;
      }
    }
  }

  vector<Vector> velocities;
  for (size_t i = 0; i < forest.size(); ++i) {
    velocities.push_back(Vector{0.0, 0.0});
  }

  double time = 0;

  cout << time << ' ';
  for (size_t i = 0; i < points.size() - 1; ++i)
    cout << points[i] << ", ";
  cout << points.back() << endl;

  while (time < a.end_time) {

    // cout << time << ' ';
    // for (size_t i = 0; i < points.size() - 1; ++i) {
    //   cout << points[i] << ":";
    //   cout << velocities[i] << ", ";
    // }
    // cout << points.back() << ":";
    // cout << velocities.back() << endl;

    // physics simulation
    vector<Point> old_points = points;
    for (size_t i = 0; i < points.size(); ++i) {
      Point &p = points[i];

      // find acceleration
      Vector acc {0.0, 0.0};
      for (size_t j = 0; j < old_points.size(); ++j) {
        if (i == j) continue; // Points do not interact with themselves
        Point &p2 = old_points[j];
        Vector f = lj_force(p, p2);
        // cout << i << ' ' << j << '\t' << f.x << ' ' << f.y << endl;
        // assume mass = 1
        acc.x += f.x;
        acc.y += f.y;
      }

      acc = cap_velocity(acc, a.max_acceleration);

      // update velocity
      Vector &v = velocities[i];
      v.x += acc.x * a.timestep;
      v.y += acc.y * a.timestep;

      v = cap_velocity(v, a.max_velocity);

      // update position
      p.x += v.x * a.timestep + acc.x * a.timestep * a.timestep;
      p.y += v.y * a.timestep + acc.y * a.timestep * a.timestep;

      v.x *= a.friction;
      v.y *= a.friction;
    }

    time += a.timestep;

    cout << time << ' ';
    for (size_t i = 0; i < points.size() - 1; ++i)
      cout << points[i] << ", ";
    cout << points.back() << endl;

    random_device rd;
    mt19937 rng(rd());
    std::uniform_real_distribution<double> random_angle(0.0, 3.14159);

    // divide/kill cells if relevant
    for (size_t i = 0; i < points.size(); ++i) {
      Point p = points[i];
      if (p.cell->birthtime > time)
        continue; // your time has note come yet young one

      if (p.cell->left == nullptr && p.cell->right == nullptr) {
        // cout << "killing" << endl;
        // kill the cell
        points.erase(points.begin() + i);
        velocities.erase(velocities.begin() + i);
        --i;
      } else if (p.cell->left != nullptr && p.cell->right != nullptr) {
        // cout << "dividing" << endl;
        // kill and divide the cell
        // cout << p.cell->left << endl;
        // cout << p.cell->right << endl;
        double angle = random_angle(rng);
        double x_offset = cos(angle) * sigma * 0.00005;
        double y_offset = sin(angle) * sigma * 0.00005;
        // cout << p << ' ' << x_offset << ' ' << y_offset << endl;
        points.push_back(Point{p.x + x_offset, p.y + y_offset, p.cell->left});
        // cout << points.back() << endl;
        points.push_back(Point{p.x - x_offset, p.y - y_offset, p.cell->right});
        // cout << points.back() << endl;
        velocities.push_back(Vector{0.0, 0.0});
        velocities.push_back(Vector{0.0, 0.0});
        // cout << "division done, killing" << endl;
        points.erase(points.begin() + i);
        velocities.erase(velocities.begin() + i);
        --i;
      } else {
        cerr << "invalid subtree: " << p.cell << endl;
        cout << endl;
      }
    }

  }
}

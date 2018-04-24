#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
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


shared_ptr<Node> parse_tree(string s, double time) {
  cout << endl;
  static regex re_parens(R"([\(\)])");
  static regex re_leaf(R"(([A-Z]):(\d+\.?\d*))");
  // static regex re_branch(R"((\(.*\))([A-Z]):(\d+\.?\d*))");
  static regex re_branch(R"(\((.+),(.+)\)([A-Z]):(\d+\.?\d*))");
  cout << s << endl;
  // check if we have a leaf (recursion end condition)
  smatch m;
  if (!regex_search(s, m, re_parens)) {
    cout << "LEAF!" << endl;
    // return a leaf node
    regex_match(s, m, re_leaf);
    cout << m[1] << '\t' << m[2] << endl;
    double dt = stod(m[2]);
    cout << name_to_number(m[1]) << '\t' << time + dt << endl;
    return make_shared<Node>(name_to_number(m[1]), time + dt);
  }

  // otherwise, recursively parse further branches
  cout << "not leaf" << endl;
  regex_match(s, m, re_branch);
  cout << m[1] << '\t' << m[2] << '\t' << m[3] << '\t' << m[4] << endl;
  double dt = stod(m[4]);
  cout << dt << endl;
  auto branch = make_shared<Node>(name_to_number(m[3]), time + dt);
  branch->left = parse_tree(string(m[1]), time + dt);
  branch->right = parse_tree(string(m[2]), time + dt);
  return branch;
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


struct Arguments {
  string forest;
};


int main(int argc, char **argv) {

  Arguments a;
  try {
    TCLAP::CmdLine cmd("General treatment simulator", ' ', VERSION);

    TCLAP::ValueArg<string> a_forest("f", "forest", "Forest of cell growth", true, "n/a", "; separated trees", cmd);

    cmd.parse(argc, argv);

    a.forest = a_forest.getValue();

  } catch (TCLAP::ArgException &e) {
    cerr << "TCLAP Error: " << e.error() << endl << "\targ: " << e.argId() << endl;
    return 1;
  }

  auto forest = parse_forest(a.forest);
  cout << endl;
  cout << "Resulting tree:" << endl;
  for (auto t: forest) cout << t << endl;

}

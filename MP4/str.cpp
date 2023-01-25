#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(void) {
    string s = "/.";
    char delimeter = '/';
    vector<string> path;

    size_t pos = 0, nxt;
    while ((nxt = s.find(delimeter, pos + 1)) != string::npos) {
        path.push_back(s.substr(pos + 1, nxt - pos - 1));
        pos = nxt;
    }
    path.push_back(s.substr(pos + 1));

    for (auto _s: path)
        cout << _s << ", ";
    cout << endl;
}
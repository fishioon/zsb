
#include <string>
using namespace std;

class VBox {
public:
    VBox();
    VBox(const string &name);
    ~VBox();

    int Init(const string &name);
    int Start();
    int Stop();

private:
    int m_status;
    int m_mode;
    string m_name;
};



#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;

void replace_all(std::string& str, const std::string& old, const std::string& new_s) {
	size_t n;
	while((n = str.find(old)) != string::npos) {
		str.replace(n, old.length(), new_s);	
	}
}

int main(int argc, char* argv[]) {
	if(argc == 1) {
		cout << "Usage: " << argv[0] << " filename" << endl;
		return 0;
	}

	ifstream xmldata(argv[1]);
	string curr_line;
	
	std::string file;

	while(getline(xmldata, curr_line)) {
		file.append(curr_line);
	}
	xmldata.close();

	
	std::string curr_router_name;
	std::string curr_script;


	size_t n = 0;
	while((n = file.find("<void index=\"0\">", n + 1)) != string::npos) {
		n = file.find("<string>", n + 1) + 8;
		curr_router_name = file.substr(n, file.find("</string>", n) - n);
		n = file.find("<void index=\"1\">", n);
		n = file.find("<string>", n + 1) + 8;
		curr_router_name += " - " + file.substr(n, file.find("</string>", n) - n);
		n = file.find("<void index=\"2\">", n);
		n = file.find("<string>", n) +  8;
		size_t end = file.find("</string>", n);
		curr_script = file.substr(n, end - n);
		replace_all(curr_script, "&#13;", "");
		replace_all(curr_script, "&quot;", "\"");
		replace_all(curr_script, "&lt;", "<");
		replace_all(curr_script, "&gt;", ">");
		replace_all(curr_script, "&apos;", "'");
		replace_all(curr_script, "&amp;", "&");
		ofstream out(string("scripts/" + curr_router_name).c_str());
		out << curr_script;
		out.close();


	}











}

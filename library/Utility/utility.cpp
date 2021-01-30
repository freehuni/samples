#include "utility.h"
#include <sstream>
#include <regex>

using namespace std;

namespace  Utility
{
	std::vector<std::string> GetTokens(const std::string& data, const char delimiter)
	{
		vector<string> result;
		string token;
		stringstream ss(data);

		while(getline(ss, token, delimiter))
		{
			result.push_back(token);
		}

		return result;
	}

	void ParsePath(const std::string& inPath, std::string& path, std::string& file, std::string& ext)
	{
		if (inPath.empty()) return;

		int pos = inPath.find_last_of("/");	// Check Directory
		if (pos == string::npos) // In case of No Directory
		{
			pos = inPath.find_last_of(".");	// Check File Extension
			if (pos != string::npos)	// In case of existing File Extension
			{
				file= inPath.substr(0, pos);
				ext = inPath.substr(pos + 1, inPath.size());
			}
			else
			{
				file = inPath;
			}
		}
		else
		{
			path = inPath.substr(0, pos);
			ParsePath(inPath.substr(pos+1, inPath.size()), path,  file, ext);
		}
	}

	bool IsNumber(string param)
	{
		regex pattern("[0-9]+");
		smatch m;

		return regex_match(param, m, pattern);
	}
}

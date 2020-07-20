#include <vector>
#include <string>

namespace Utility
{
	std::vector<std::string> GetTokens(const std::string& Text, const char delimiter);
	void ParsePath(const std::string& inPath, std::string& path, std::string& file, std::string& ext);
}

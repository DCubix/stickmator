#include "CommandFile.h"

#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>

struct LineParser {
    std::vector<char> buffer;

    LineParser(const std::string& input) {
        buffer = std::vector<char>(input.begin(), input.end());
    }

    char Peek(size_t offset = 0) {
        if (buffer.empty()) return '\0';
        if (offset >= buffer.size()) return '\0';
        return buffer[offset];
    }

    char Pop() {
        if (buffer.empty()) return '\0';
        char c = buffer[0];
        buffer.erase(buffer.begin());
        return c;
    }

    bool CanReadMore() {
        SkipWhitespace();
        return Peek() != '\0' && Peek() != '#';
    }

    void SkipWhitespace() {
        while (Peek() != '\0' && ::isspace(Peek())) {
            Pop();
        }
    }

    std::string ReadString() {
        SkipWhitespace();
        std::string str;
        while (Peek() != '\0' && !::isspace(Peek())) {
            str += Pop();
        }
        return str;
    }

    std::string ReadWhile(std::function<bool(char)> predicate) {
        SkipWhitespace();
        std::string str;
        while (Peek() != '\0' && predicate(Peek())) {
            str += Pop();
        }
        return str;
    }

    std::string ReadQuotedString() {
        SkipWhitespace();
        if (Peek() != '\"') return ReadString();
        Pop(); // pop the quote
        std::string str = ReadWhile([](char c) {
            return c != '\"';
        });
        Pop(); // pop the quote
        return str;
    }

    int ReadInt() {
        SkipWhitespace();
        std::string str = ReadWhile([](char c) {
            return ::isdigit(c) || c == '-';
        });
        return std::stoi(str);
    }

    double ReadDouble() {
        SkipWhitespace();
        std::string str = ReadWhile([](char c) {
            return ::isdigit(c) || c == '.' || c == '-';
        });
        return std::stod(str);
    }

    std::string PeekString() {
        std::string str = "";
        SkipWhitespace();

        size_t off = 0;
        while (Peek(off) != '\0' && !::isspace(Peek(off))) {
            str += Peek(off++);
        }

        return str;
    }

    bool IsDouble() {
		std::string str = PeekString();
        return !str.empty() &&
            str.find_first_not_of("0123456789.-") == std::string::npos;
	}
};

void CommandFile::LoadFromString(const std::string& data) {
    std::istringstream iss(data);

    while (!iss.eof()) {
        std::string line;
        std::getline(iss, line);
        if (line.empty()) continue;

        LineParser lp(line);

        if (::isalpha(line[0])) {
            std::string cmdName = lp.ReadString();
            std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);

            Command cmd{};
            cmd.name = cmdName;

            lp.SkipWhitespace();

            while (lp.CanReadMore()) {
                // check string
                if (lp.Peek() == '\"' || ::isalpha(lp.Peek())) {
                    std::string val = lp.ReadQuotedString();
                    std::string lowerVal = val;
                    std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);

                    // boolean check
                    if (lowerVal == "true" || lowerVal == "false") {
						cmd.args.push_back(lowerVal == "true");
					}
                    else if (lowerVal == "yes" || lowerVal == "no") {
                        cmd.args.push_back(lowerVal == "yes");
                    }
                    else {
						cmd.args.push_back(val);
					}
				}
                // check number
                else if (lp.IsDouble()) {
					cmd.args.push_back(lp.ReadDouble());
				}
			}

            m_commands.push_back(cmd);
        }
        else if (lp.Peek() == '#') {
            while (lp.Peek() != '\0') lp.Pop();
        }
        else {
            lp.Pop();
        }
    }
}

void CommandFile::LoadFromFile(const std::string& fileName) {
    std::string data{};

    if (std::ifstream in{ fileName, std::ios::in | std::ios::ate }) {
        size_t len = in.tellg();
        in.seekg(0, std::ios::beg);

        data.resize(len);
        in.read(data.data(), len);
    }

    LoadFromString(data);
}

std::string CommandFile::SaveToString(const std::string* headerComments, size_t headerCommentsCount) const {
    std::string output{};

    if (headerComments) {
        for (size_t i = 0; i < headerCommentsCount; i++) {
			output += "# " + headerComments[i] + "\n";
		}
	}

    output += "\n";

    for (const Command& cmd : m_commands) {
		output += cmd.name;

        for (const CommandArg& arg : cmd.args) {
			output += " ";

            std::visit([&output](const auto& arg) {
				using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, double>) {
                    // if we do not have a fractional part, we can output as int
                    double intPart;
                    if (std::modf(arg, &intPart) == 0) {
                        output += std::to_string(static_cast<int>(arg));
                    }
                    else {
                        output += std::to_string(arg);
                    }
				}
                else if constexpr (std::is_same_v<T, bool>) {
					output += arg ? "true" : "false";
				}
                else if constexpr (std::is_same_v<T, std::string>) {
                    // string has spaces, so we need to quote it
                    if (arg.find(' ') != std::string::npos || arg[0] == '#') {
                        output += "\"" + arg + "\"";
                    }
                    else {
						output += arg;
					}
				}
			}, arg);
		}

		output += "\n";
	}
    
	return output;
}

void CommandFile::SaveToFile(const std::string& fileName, const std::string* headerComments, size_t headerCommentsCount) const {
    std::string data = SaveToString(headerComments, headerCommentsCount);
    if (std::ofstream out{ fileName, std::ios::out }) {
		out.write(data.data(), data.size());
	}
}

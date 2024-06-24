#pragma once

#include <string>
#include <vector>
#include <variant>
#include <stdexcept>

using CommandArg = std::variant<double, bool, std::string>;

struct Command {
	std::string name;
	std::vector<CommandArg> args;

	template <typename T>
	T GetOptionalArg(size_t index, T defaultValue) const {
		if (index >= args.size()) {
			return defaultValue;
		}
		return std::get<T>(args[index]);
	}

	template <typename T>
	T GetArg(size_t index) const {
		if (index >= args.size() || args.empty()) {
			throw std::runtime_error("Command argument index out of range");
		}
		return std::get<T>(args[index]);
	}
};

class CommandFile {
public:
	CommandFile() = default;

	void LoadFromString(const std::string& data);
	void LoadFromFile(const std::string& fileName);

	std::string SaveToString(const std::string* headerComments = nullptr, size_t headerCommentsCount = 0) const;
	void SaveToFile(const std::string& fileName, const std::string* headerComments = nullptr, size_t headerCommentsCount = 0) const;

	template <typename... Args>
	void AddCommand(const std::string& name, Args&&... args) {
		std::vector<CommandArg> argsList = { std::forward<Args>(args)... };
		AddCommandVec(name, argsList);
	}

	void AddCommandVec(const std::string& name, const std::vector<CommandArg>& args) {
		m_commands.push_back({ name, args });
	}

	const std::vector<Command>& GetCommands() const { return m_commands; }

private:
	std::vector<Command> m_commands;
};

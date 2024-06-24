#pragma once

#include <vector>
#include <memory>
#include <functional>

class IActionCommand {
public:
	virtual void Execute() = 0;
	virtual void Undo() = 0;
	void Redo() { Execute(); }
};

using IActionCommandPtr = std::shared_ptr<IActionCommand>;

class UndoRedo {
public:
	void Undo();
	void Redo();
	IActionCommand* AddCommand(IActionCommand* cmd);
private:
	std::vector<IActionCommandPtr> m_undoStack;
	std::vector<IActionCommandPtr> m_redoStack;
};

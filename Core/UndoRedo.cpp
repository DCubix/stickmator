#include "UndoRedo.h"

void UndoRedo::Undo() {
	if (m_undoStack.empty()) return;
	auto& cmd = m_undoStack.back();
	cmd->Undo();
	m_redoStack.push_back(cmd);
	m_undoStack.pop_back();
}

void UndoRedo::Redo() {
	if (m_redoStack.empty()) return;
	auto& cmd = m_redoStack.back();
	cmd->Redo();
	m_undoStack.push_back(cmd);
}

IActionCommand* UndoRedo::AddCommand(IActionCommand* cmd) {
	m_undoStack.push_back(IActionCommandPtr(cmd));
	m_redoStack.clear();
	return m_undoStack.back().get();
}

#include <olcPixelGameEngine.h>
#include <olcPGEX_TinyGUI.h>
#include <Stick.h>
#include <UndoRedo.h>

#include <tinyFileDialogs.h>

#include <iostream>

#ifdef None
#undef None
#endif

#pragma region UndoRedo Commands

class FigureEditor;
class AddStickCommand : public IActionCommand {
public:
	AddStickCommand(FigureEditor* editor, Stick* parent, int len, double angle) :
		editor(editor),
		len(len),
		angle(angle),
		parent(parent)
	{}

	void Execute() override;
	void Undo() override;

	int len{ 0 };
	double angle{ 0.0 };
	Stick* child{ nullptr };
	Stick* parent{ nullptr };
	FigureEditor* editor{ nullptr };
};

class DelStickCommand : public IActionCommand {
public:
	DelStickCommand(FigureEditor* editor, Stick* parent, Stick* child) :
		editor(editor),
		parent(parent),
		child(child)
	{}

	void Execute() override;
	void Undo() override;

	int len{ 0 };
	double angle{ 0.0 };
	Stick* child{ nullptr };
	Stick* parent{ nullptr };
	FigureEditor* editor{ nullptr };
};

class ChangeStickCommand : public IActionCommand {
public:
	ChangeStickCommand(
		FigureEditor* editor,
		Stick* stick,
		int oldLen, double oldAngle,
		int len, double angle,
		olc::Pixel oldColor, olc::Pixel color
	) :
		editor(editor),
		len(len),
		angle(angle),
		oldLen(oldLen),
		oldAngle(oldAngle),
		color(color),
		oldColor(oldColor),
		stick(stick)
	{}

	void Execute() override;
	void Undo() override;

	int len{ 0 }, oldLen{ 0 };
	double angle{ 0.0 }, oldAngle{ 0.0 };
	olc::Pixel color{ olc::BLACK }, oldColor{ olc::BLACK };

	Stick* stick{ nullptr };
	FigureEditor* editor{ nullptr };
};

#pragma endregion

class FigureEditor : public olc::PixelGameEngine
{
public:
	FigureEditor()
	{
		// Name your application
		sAppName = "Figure Editor | StickMator";
	}

public:
	bool OnUserCreate() override
	{
		gui.baseColor = olc::Pixel(207, 194, 157);

		figure.name = "Untitled";
		figure.root = std::make_unique<Stick>();
		figure.root->pos = olc::vi2d{ ScreenWidth() / 2, ScreenHeight() / 2 };

		selectedStick = figure.root.get();

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		std::string mnuMotionType[] = {
			"None",
			"Normal",
			"Kinematic"
		};

		Clear(olc::WHITE);
		SetPixelMode(olc::Pixel::Mode::ALPHA);

		DrawFigure(figure);

		if (selectedStick) {
			switch (selectionMode) {
				case ManipulatorMode::Rotate: {
					auto vec = selectedStick->WorldPos() - GetMousePos();
					double angle = std::atan2(vec.y, vec.x) + hPi * 2.0f;

					// constrain angle to -180 to 180
					angle = std::fmod(angle + Pi, 2.0 * Pi) - Pi;

					selectedStick->SetWorldAngle(angle, false);
					if (editLength) selectedStick->len = std::clamp(vec.mag(), 0, 100);
					SelectStick(selectedStick);
					isSaved = false;
				} break;
				default: break;
			}
		}

        auto menuArea = gui.RectCutTop(15);
        gui.PushRect(menuArea.Expand(-2));

        FillRect(
            menuArea.Position(),
            menuArea.Size(),
            gui.baseColor
        );

        if (gui.Button("mnu_file", gui.RectCutLeft(30), "File")) {
            gui.ShowPopup("popup_file");
        }
        if (gui.Button("mnu_edit", gui.RectCutLeft(30), "Edit")) {
            gui.ShowPopup("popup_edit");
        }

		gui.RectCutLeft(2);

		gui.Label(
			gui.PeekRect(),
			utils::StringFormat(
				"File: %s%s",
				fileName.empty() ? "Untitled" : fileName.c_str(),
				isSaved ? "" : " **"
			)
		);

        gui.PopRect(); // menu area

		auto toolsArea = gui.RectCutBottom(60);
		gui.PushRect(toolsArea.Expand(-2));

		FillRect(
			toolsArea.Position(),
			toolsArea.Size(),
			gui.baseColor
		);

		auto stickButtonsArea = gui.RectCutLeft(56);
		gui.PushRect(stickButtonsArea.Expand(-2));

		gui.Label(gui.RectCutTop(8), "Edit");

		if (gui.Button("btn_add_stick", gui.RectCutTop(11), "Add Stick")) {
			undoRedo.AddCommand(new AddStickCommand(this, selectedStick, 16, 0.0))->Execute();
		}
		if (gui.Button("btn_del_stick", gui.RectCutTop(11), "Del Stick")) {
			if (selectedStick != figure.root.get() && selectedStick->parent) {
				undoRedo.AddCommand(new DelStickCommand(this, selectedStick->parent, selectedStick))->Execute();
			}
		}
		gui.RectCutTop(2);
		gui.Toggle("chk_edit_len", gui.RectCutTop(11), "Edit Length", editLength);

		gui.PopRect(); // stick buttons area

		gui.RectCutLeft(3);

		Rect stickPropsArea = gui.RectCutLeft(72);
		gui.PushRect(stickPropsArea.Expand(-2));

		gui.Label(gui.RectCutTop(8), "Properties");

		if (selectedStick && selectedStick->parent) {
			std::string colorStr = utils::StringFormat(
				"#%02X%02X%02X",
				selectedStick->color.r, selectedStick->color.g, selectedStick->color.b
			);
			if (gui.Button("btn_stk_color", gui.RectCutTop(11), colorStr, selectedStick->color)) {
				oldColor = selectedStick->color;
				unsigned char pickedColor[3] = { selectedStick->color.r, selectedStick->color.g, selectedStick->color.b };
				tinyfd_colorChooser("Choose Color", nullptr, pickedColor, pickedColor);
				undoRedo.AddCommand(
					new ChangeStickCommand(
						this, selectedStick,
						oldLen, oldAngle,
						selectedStick->len, selectedStick->angle,
						oldColor, olc::Pixel(pickedColor[0], pickedColor[1], pickedColor[2])
					)
				)->Execute();
			}

			gui.Toggle("chk_visible", gui.RectCutTop(11), "Is Visible", selectedStick->isVisible);
			gui.Toggle("chk_circle", gui.RectCutTop(11), "Is Circle", selectedStick->isCircle);
			//gui.Toggle("chk_driver", gui.RectCutTop(11), "Is Driver", selectedStick->isDriver);
		} else {
			gui.Label(gui.RectCutTop(11), "[Root Stick]", false, olc::BLACK);
		}

		gui.PopRect(); // stick props area

		stickPropsArea = gui.RectCutLeft(72);
		gui.PushRect(stickPropsArea.Expand(-2));
		gui.RectCutTop(8);
		if (selectedStick && selectedStick->parent) {
			int motTypeIdx = int(selectedStick->motionType);
			if (gui.Button("btn_mot_type", gui.RectCutTop(11), "[" + mnuMotionType[motTypeIdx] + "]")) {
				gui.ShowPopup("popup_motion_type");
			}

			gui.Spinner("spn_len", gui.RectCutTop(11), selectedStick->len, 0, 100, 1, "Len. %d");
			if (gui.Spinner("spn_ang", gui.RectCutTop(11), angleDeg, -180, 180, 1, "Ang. %d")) {
				selectedStick->angle = angleDeg * Pi / 180.0f;
			}

			if (
				gui.WidgetPressed("spn_len") || gui.WidgetPressed("spn_len_left") ||
				gui.WidgetPressed("spn_ang") || gui.WidgetPressed("spn_ang_left") ||
				gui.WidgetPressed("spn_len_right") || gui.WidgetPressed("spn_ang_right")
				) {
				oldLen = selectedStick->len;
				oldAngle = selectedStick->angle;
			}

			if (
				gui.WidgetReleased("spn_len") || gui.WidgetReleased("spn_len_left") ||
				gui.WidgetReleased("spn_ang") || gui.WidgetReleased("spn_ang_left") ||
				gui.WidgetReleased("spn_len_right") || gui.WidgetReleased("spn_ang_right")
				) {
				if (selectedStick && (oldLen != selectedStick->len || oldAngle != selectedStick->angle)) {
					undoRedo.AddCommand(
						new ChangeStickCommand(
							this, selectedStick,
							oldLen, oldAngle,
							selectedStick->len, selectedStick->angle,
							oldColor, selectedStick->color
						)
					)->Execute();
				}
			}

			// TODO: undo/redo
			gui.Spinner("spn_draworder", gui.RectCutTop(11), selectedStick->drawOrder, 0, 999, 1, "Order: %d");
		}
		gui.PopRect(); // stick props area

		stickPropsArea = gui.RectCutLeft(72);
		gui.PushRect(stickPropsArea.Expand(-2));
		gui.RectCutTop(8);
		if (selectedStick && selectedStick->parent) {
			if (!selectedStick->driver) {
				std::string btnText = "Set Driver";
				if (pickingDriver) btnText = "Picking...";
				if (gui.Button(
					"btn_sel_driver",
					gui.RectCutTop(11),
					btnText
				) && !pickingDriver) {
					pickingDriver = true;
				}
			}
			else {
				if (gui.Button("btn_del_driver", gui.RectCutTop(11), "Clear Driver")) {
					selectedStick->driver->isDriver = false;
					selectedStick->driver->driver = nullptr;
					selectedStick->driver = nullptr;
				}
				gui.Label(gui.RectCutTop(11), "Stk #" + std::to_string(selectedStick->driver->id));

				gui.SpinnerF("spn_driver_influence", gui.RectCutTop(11), selectedStick->driverInfluence, -1.0f, 1.0f, 0.01f, "Infl.: %.2f");
				gui.SpinnerF("spn_driver_angle_offset", gui.RectCutTop(11), selectedStick->driverAngleOffset, -180.0f, 180.0f, 1.0f, "Off.: %.2f");
			}
		}
		gui.PopRect(); // stick props area

		auto figurePropsArea = gui.RectCutLeft(120);
		gui.PushRect(figurePropsArea.Expand(-2));

		gui.RectCutTop(8);

		gui.PushRect(gui.RectCutTop(11));
		gui.Label(gui.RectCutLeft(25), "Name", true);
		gui.RectCutLeft(2);
		if (gui.EditBox("edt_name", gui.PeekRect(), figure.name)) {
			isSaved = false;
		}
		gui.PopRect();

		gui.PopRect(); // figure props area
		gui.PopRect(); // tools area

		// center figure
		if (figure.root) {
			auto bounds = gui.PeekRect();
			figure.root->pos = olc::vi2d{ bounds.width / 2 + bounds.x, bounds.height / 2 + bounds.y };
		}

		std::string mnuFileItems[] = {
			"New",
			"Open",
			"Save",
			"Save As...",
			"-",
			"Exit"
		};

		bool shouldExit = false;
		if (gui.MakePopup("popup_file", mnuFileItems, 6, mnuSelFile)) {
			switch (mnuSelFile) {
				case 0: mnu_NewAction(); break;
				case 1: mnu_OpenAction(); break;
				case 2: mnu_SaveAction(); break;
				case 3: mnu_SaveAsAction(); break;
				case 5:
					shouldExit = mnu_ExitAction();
					break;
				default: break;
			}
		}

		std::string mnuEditItems[] = {
			"Undo",
			"Redo"
		};
		if (gui.MakePopup("popup_edit", mnuEditItems, 2, mnuSelEdit)) {
			switch (mnuSelEdit) {
				case 0: undoRedo.Undo(); break;
				case 1: undoRedo.Redo(); break;
				default: break;
			}
		}

		if (gui.MakePopup("popup_motion_type", mnuMotionType, 3, mnuSelMotionType)) {
			selectedStick->motionType = MotionType(mnuSelMotionType);
		}

		if (GetMouse(0).bReleased && moving) {
			if (selectedStick && (oldLen != selectedStick->len || oldAngle != selectedStick->angle)) {
				undoRedo.AddCommand(
					new ChangeStickCommand(
						this, selectedStick,
						oldLen, oldAngle,
						selectedStick->len, selectedStick->angle,
						oldColor, selectedStick->color
					)
				)->Execute();
			}
			moving = false;
		}

		return !shouldExit;
	}

	void DrawFigure(Figure& fig) {
		auto sticks = fig.root->GetSticksRecursiveSorted();

		for (auto stk : sticks) {
			auto [mode, stick] = stk->GetStickForManipulation(this, { 0, 0 }, true);
			if (stick) {
				bool wasPickingDriver = pickingDriver;
				if (GetMouse(0).bPressed && !moving) {
					oldLen = stick->len;
					oldAngle = stick->angle;
					oldColor = stick->color;
					moving = true;

					if (pickingDriver && selectedStick) {
						stick->isDriver = true;
						selectedStick->driver = stick;
						pickingDriver = false;
					}
				}

				if (!wasPickingDriver) SelectStick(stick);
			}

			if (mode != ManipulatorMode::None) {
				selectionMode = mode;
			}
			else {
				if (GetMouse(0).bReleased) {
					selectionMode = ManipulatorMode::None;
				}
			}

			if (stick) break;
		}

		for (auto stk : sticks) {
			stk->Draw(this, selectedStick);
		}
		for (auto stk : sticks) {
			stk->DrawManipulatorsEditor(this);
		}
	}

	void SelectStick(Stick* stick) {
		selectedStick = stick;
		// angle from -180 to 180
		angleDeg = int(selectedStick->angle * 180.0f / Pi);
	}

	int CheckSavedDialog() {
		if (!isSaved) {
			return tinyfd_messageBox(
				"StickMator",
				"You have unsaved changes. Do you want to save them?",
				"yesnocancel",
				"question",
				0
			);
		}
		return 2;
	}

	void act_New() {
		figure = Figure{};
		figure.name = "Untitled";
		figure.root = std::make_unique<Stick>();
		figure.root->pos = olc::vi2d{ ScreenWidth() / 2, ScreenHeight() / 2 };
		selectedStick = figure.root.get();
		SelectStick(figure.root.get());
		fileName = "";
		isSaved = true;
	}

	bool mnu_ExitAction() {
		int res = CheckSavedDialog();
		if (res == 1) {
			mnu_SaveAction();
		}
		else if (res == 0) {
			return false;
		}
		return true;
	}

	void mnu_NewAction() {
		int res = CheckSavedDialog();
		if (res == 1) {
			mnu_SaveAction();
		}
		else if (res == 0) {
			return;
		}
		act_New();
	}

	void mnu_OpenAction() {
		int res = CheckSavedDialog();
		if (res == 1) {
			mnu_SaveAction();
		}
		else if (res == 0) {
			return;
		}

		char const* filterPatterns[1] = { "*.fig" };
		auto ofdRes = tinyfd_openFileDialog(
			"Open File",
			"",
			1,
			filterPatterns,
			"StickMator Figure File",
			0
		);

		if (ofdRes) {
			act_New();
			figure.LoadFromFile(ofdRes);
			SelectStick(figure.root.get());
			fileName = ofdRes;
			isSaved = true;
		}
	}

	void mnu_SaveAction() {
		char const* filterPatterns[1] = { "*.fig" };

		if (fileName.empty()) {
			auto res = tinyfd_saveFileDialog(
				"Save File",
				"",
				1,
				filterPatterns,
				"StickMator Figure File"
			);
			if (res) {
				fileName = res;
			}
		}
		if (!fileName.empty()) {
			figure.SaveToFile(fileName);
			isSaved = true;
		}
	}

	void mnu_SaveAsAction() {
		fileName = "";
		mnu_SaveAction();
	}

	Figure figure{};
	Stick* selectedStick{ nullptr };
	ManipulatorMode selectionMode{ ManipulatorMode::None };

	int oldLen{ 0 };
	double oldAngle{ 0.0 };
	olc::Pixel oldColor{ olc::BLACK };

	int angleDeg{ 0 };

	olcPGEX_TinyGUI gui{};
	size_t mnuSelMotionType{ 0 }, mnuSelFile{ 0 }, mnuSelEdit{ 0 };

	std::string fileName{ "" };
	bool isSaved{ false },
		editLength{ false },
		moving{ false },
		pickingDriver{ false };

	UndoRedo undoRedo{};
};

#if !defined(_WIN32)
int main()
#else 
int APIENTRY WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
#endif 
{
	FigureEditor editor;
	if (editor.Construct(480, 320, 2, 2))
		editor.Start();
	return 0;
}

void AddStickCommand::Execute() {
	child = &parent->AddChild(len, angle);
	editor->SelectStick(child);
}

void AddStickCommand::Undo() {
	parent->RemoveChild(child);
	editor->SelectStick(parent);
}

void DelStickCommand::Execute() {
	len = child->len;
	angle = child->angle;
	parent->RemoveChild(child);
	editor->SelectStick(parent);
}

void DelStickCommand::Undo() {
	child = &parent->AddChild(len, angle);
	editor->SelectStick(child);
}

void ChangeStickCommand::Execute() {
	stick->len = len;
	stick->angle = angle;
	stick->color = color;
}

void ChangeStickCommand::Undo() {
	stick->len = oldLen;
	stick->angle = oldAngle;
	stick->color = oldColor;
}

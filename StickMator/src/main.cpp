#include <olcPixelGameEngine.h>
#include <olcPGEX_TinyGUI.h>
#include <Stick.h>
#include <CommandFile.h>
#include <tinyFileDialogs.h>
#include <UndoRedo.h>

#include <gif.h>

#include <filesystem>

#ifdef None
#undef None
#endif

#define APP_VERSION "1.1"

constexpr int gScreenWidth = 320;
constexpr int gScreenHeight = 240;

#pragma region UndoRedo Commands

class StickMator;

class AddFigureCommand : public IActionCommand {
public:
    AddFigureCommand(StickMator* app, const std::vector<Command>& commands)
        : app(app), commands(commands) {}

    void Execute() override;
    void Undo() override;

    StickMator* app;
    std::shared_ptr<Figure> figure{ nullptr };
    olc::vi2d rootPos;
    std::vector<Command> commands;
    int savedId{ -1 };
};

class DelFigureCommand : public IActionCommand {
public:
    DelFigureCommand(StickMator* app, std::shared_ptr<Figure> figure)
		: app(app), figure(figure) {}

    void Execute() override;
    void Undo() override;

	StickMator* app;
	std::shared_ptr<Figure> figure{};
    olc::vi2d rootPos;
    std::vector<Command> commands;
    int savedId{ -1 };
};

class MoveStickCommand : public IActionCommand {
public:
	MoveStickCommand(Stick* stick, olc::vi2d prevPos, double prevAngle, olc::vi2d pos, double angle)
		: stick(stick),
        prevPos(prevPos),
        prevAngle(prevAngle),
        pos(pos),
        angle(angle)
    {}

    void Execute() override;
    void Undo() override;

	Stick* stick;
	olc::vi2d pos, prevPos;
    double angle, prevAngle;
};

#pragma endregion

void Launch(const std::string& path);

#ifdef _WIN32
#include <Windows.h>
void Launch(const std::string& path) {
	STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // current directory
    std::string currentDir = std::filesystem::current_path().string();

    CreateProcessA(
        nullptr,
        (LPSTR)path.c_str(),
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
        currentDir.c_str(),
		&si,
		&pi
	);
    CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
#endif

#ifdef __linux__
#include <unistd.h>
void Launch(const std::string& path) {
    if (fork() == 0) {
        execl(path.c_str(), 0);
    }
}
#endif

class StickMator : public olc::PixelGameEngine
{
public:
    StickMator()
    {
        // Name your application
        sAppName = "Stickman";
    }

public:
    bool OnUserCreate() override
    {
        mnuFigureEditorItems.push_back("Load Figure");
        mnuFigureEditorItems.push_back("-");
        mnuFigureEditorItems.push_back("Launch \"Figure Editor\"");
        mnuFigureEditorItems.push_back("-");

        gui.baseColor = olc::Pixel(207, 194, 157);

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        Clear(gui.PixelBrightness(gui.baseColor, 0.4f));
        
        Stick* selectedRoot = selectedStick ? selectedStick->GetRoot() : nullptr;

        auto animTickDrawFn = [&](olc::PixelGameEngine* pge, int value, int min, int max, Rect bounds) {
            int tickX = bounds.x + (bounds.width * value) / (max - min);
            int tickY = bounds.y + bounds.height / 2;
            int tickHeight = value % 10 == 0 ? 10 : 6;
            int tickWidth = (int)std::round(float(bounds.width) / (max - min));
            if (tickWidth <= 0) return;

            if (selectedRoot) {
                pge->FillRect(
                    tickX, tickY, tickWidth, 11,
                    selectedRoot->HasAnimationRecursive(value) ? olc::YELLOW : olc::DARK_GREY
                );
            }

            pge->DrawLine(tickX, tickY, tickX, tickY + tickHeight, olc::BLACK);
        };

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
        if (gui.Button("mnu_figure_edit", gui.RectCutLeft(40), "Figures")) {
            gui.ShowPopup("popup_figure_edit");
        }
        if (gui.Button("mnu_edit", gui.RectCutLeft(30), "Edit")) {
            gui.ShowPopup("popup_edit");
        }
        if (gui.Button("mnu_about", gui.RectCutLeft(30), "About")) {
			gui.ShowPopup("popup_about");
		}

        gui.PopRect(); // menu area

        auto playbackArea = gui.RectCutBottom(44);
        gui.PushRect(playbackArea.Expand(-2));

        FillRect(
            playbackArea.Position(),
            playbackArea.Size(),
            gui.baseColor
        );

        auto buttonArea = gui.RectCutLeft(70);
        gui.PushRect(buttonArea);

        gui.PushRect(gui.RectCutTop(13));
        if (gui.Button("play_pause", gui.RectCutLeft(gui.PeekRect().width / 2), playing ? "Pause" : "Play")) {
            playing = !playing;
            timer = 0.0f;
        }
        if (gui.Button("stop", gui.PeekRect(), "Stop")) {
            playing = false;
            currentFrame = 0;
            AnimateAll(currentFrame);
            timer = 0.0f;
        }
        gui.PopRect();

        gui.RectCutTop(2);

        gui.PushRect(gui.RectCutTop(11));
        if (gui.Spinner(
            "frame",
            gui.PeekRect(),
            currentFrame, 0, std::max(int(MaxFramesAll()), 100), 1,
            "Fr. %d",
            selectedRoot && selectedRoot->HasAnimationRecursive(currentFrame) ? olc::YELLOW : olc::Pixel(0, 0, 0, 0)
        )) {
            AnimateAll(currentFrame);
        }
        gui.PopRect();

        gui.RectCutTop(2);

        gui.PushRect(gui.RectCutTop(11));
        if (gui.Button("del_keyframe", gui.PeekRect(), "Del.KeyFrame")) {
            selectedRoot->DeleteKeyframe(currentFrame);
		}
        gui.PopRect();

        gui.PopRect(); // button area

        gui.PushRect(gui.PeekRect().Expand(-4));
        if (gui.Slider(
            "frame_slider",
            gui.RectCutTop(14),
            currentFrame, 0, std::max(int(MaxFramesAll()), 100),
            animTickDrawFn
        )) {
            AnimateAll(currentFrame);
        }
        gui.PopRect(); // slider area

        gui.PopRect(); // playback area

        auto viewportArea = gui.PeekRect();

        int rectHeight = viewportArea.height;
        int screenCenterX = viewportArea.x + (viewportArea.width / 2 - gScreenWidth / 2);
        int screenCenterY = viewportArea.y + (viewportArea.height / 2 - gScreenHeight / 2);

        FillRect(screenCenterX, screenCenterY, gScreenWidth, gScreenHeight, olc::WHITE);

        auto offset = olc::vi2d(screenCenterX, screenCenterY);
        for (auto& fig : figures) {
            DrawOnionSkin(*fig, offset);
        }

        for (auto& fig : figures) {
            DrawFigure(*fig, offset);
        }

        // ----- POPUPS -----
        std::string mnuFileItems[] = {
			"New",
			"Open",
			"Save",
			"Save As...",
            "-",
            "Export GIF",
            "-",
			"Exit"
		}; // BRB!!
        if (gui.MakePopup("popup_file", mnuFileItems, 8, mnuSelFile)) {
            switch (mnuSelFile) {
                case 0: mnu_FileNewAction(); break;
                case 1: mnu_FileOpenAction(); break;
                case 2: mnu_FileSaveAction(); break;
                case 3: mnu_FileSaveAsAction(); break;
                case 5: mnu_ExportGIFAction(); break;
				case 7: if (mnu_FileExitAction()) return false;
			}
        }

        std::string mnuEditItems[] = {
            "Undo",
            "Redo",
            "-",
            "Delete Selected"
        };
        if (gui.MakePopup("popup_edit", mnuEditItems, 4, mnuSelEdit)) {
            switch (mnuSelEdit) {
                case 0: undoRedo.Undo(); break;
                case 1: undoRedo.Redo(); break;
                case 3: mnu_EditDeleteFigureAction(); break;
                default: break;
			}
        }

        if (gui.MakePopup("popup_figure_edit", mnuFigureEditorItems.data(), mnuFigureEditorItems.size(), mnuSelFigure)) {
            switch (mnuSelFigure) {
                case 2: {
                    Launch("FigureEditor.exe");
                } break;
                case 0: mnu_FigureLoadAction(); break;
                default: {
                    size_t index = mnuSelFigure - 4;
                    act_FigureLoadFile(tempFigureFileNames[index]);
                } break;
			}
        }

        std::string mnuAboutItems[] = {
			"About \"StickMator\""
		};
        if (gui.MakePopup("popup_about", mnuAboutItems, 1, selectedMenu)) {
            tinyfd_messageBox(
                "About StickMator",
                    "StickMator is a simple stickman animation tool written in C++ using the olcPixelGameEngine.\n"
                    "\n\n"
                    "Created by Diego Lopes\n"
                    "Version: " APP_VERSION,
                "ok",
                "info",
                0
            );
        }
        //

        olc::vi2d mousePosRel = GetMousePos() - offset;
        if (selectedStick) {
            switch (selectionMode) {
                case ManipulatorMode::Move: {
                    selectedStick->pos += (mousePosRel - prevMouse);
                    stickMoved = true;
                } break;
                case ManipulatorMode::Rotate: {
                    auto vec = selectedStick->WorldPos() - mousePosRel;
                    double angle = std::atan2(vec.y, vec.x) + hPi * 2.0;

                    // constrain angle to -180 to 180
                    angle = std::fmod(angle + Pi, 2.0 * Pi) - Pi;

                    selectedStick->SetWorldAngle(angle, selectedStick->motionType == MotionType::Kinematic);
                    stickMoved = true;
                } break;
                default: break;
            }
        }

        if (playing) {
            AnimateAll(currentFrame);

            timer += fElapsedTime;
            if (timer >= 1.0f / FrameRate) {
                timer = 0.0f;
                if (currentFrame++ >= MaxFramesAll()) {
                    currentFrame = 0;
                }
            }
        }

        if (GetMouse(0).bReleased && stickMoved && selectedRoot) {
            selectedStick->SetKeyframe(currentFrame);
            if (oldPos != selectedStick->pos || oldAngle != selectedStick->angle) {
                undoRedo.AddCommand(
                    new MoveStickCommand(
                        selectedStick,
                        oldPos, oldAngle,
                        selectedStick->pos, selectedStick->angle
                    )
                )->Execute();
            }
            stickMoved = false;
        }

        prevMouse = mousePosRel;
        return true;
    }

    void DrawFigure(Figure& figure, const olc::vi2d& offset) {
        DrawFigure(figure, olc::BLANK, offset);
	}

    void DrawOnionSkin(Figure& figure, const olc::vi2d& offset) {
        if (playing) return;

        auto& fig = *figure.root;

        const int framesToDraw[] = {
            fig.NearFrameLeft(currentFrame),
        };
        const olc::Pixel colors[] = { olc::Pixel(133, 161, 255), olc::Pixel(255, 153, 153) };

        int i = 0;
        for (int frame : framesToDraw) {
            fig.SaveState();
            AnimateAllFigureSticks(figure, frame);
            DrawFigure(figure, colors[i++], offset, false);
            fig.RestoreState();
        }
    }

    void AnimateAllFigureSticks(Figure& fig, int frame) {
        auto sticks = fig.root->GetSticksRecursiveSorted();
        for (auto stk : sticks) {
            stk->Animate(frame);
        }
    }

    void AnimateAll(int frame) {
		for (auto& fig : figures) {
            AnimateAllFigureSticks(*fig, frame);
		}
	}

    int MaxFramesAll() {
        int maxFrames = 0;
		for (auto& fig : figures) {
			maxFrames = std::max(maxFrames, fig->root->MaxFrames());
		}
		return maxFrames;
	}

    void DrawFigure(Figure& fig, olc::Pixel color, const olc::vi2d& offset = { 0, 0 }, bool manipulate = true) {
        auto sticks = fig.root->GetSticksRecursiveVisibleSorted();
        bool selected = selectedStick && selectedStick->GetRoot() == fig.root.get();

        for (auto stk : sticks) {
            auto [mode, stick] = stk->GetStickForManipulation(this, offset);
            if (stick) {
                if (GetMouse(0).bPressed && !moving) {
                    oldPos = stick->pos;
                    oldAngle = stick->angle;
                }
                selectedStick = stick;
            }
            if (mode != ManipulatorMode::None) {
                selectionMode = mode;
            }
            else {
                if (GetMouse(0).bReleased) selectionMode = ManipulatorMode::None;
            }

            if (stick) break;
        }

        for (auto stk : sticks) {
            stk->Draw(this, nullptr, offset, color);
        }

        if (!playing && manipulate && selected) {
            for (auto stk : sticks) {
                stk->DrawManipulators(this, offset);
            }
        }
    }

    void act_FigureLoadFile(const std::string& fileName) {
        CommandFile cf{};
        cf.LoadFromFile(fileName);

        undoRedo.AddCommand(
			new AddFigureCommand(this, cf.GetCommands())
		)->Execute();

        currentFrame = 0;
        AnimateAll(currentFrame);
    }

    void mnu_FigureLoadAction() {
        char const* filterPatterns[1] = { "*.fig" };
        auto res = tinyfd_openFileDialog(
            "Open File",
            "",
            1,
            filterPatterns,
            "StickMator Figure File",
            0
        );

        if (res) {
            act_FigureLoadFile(res);
            if (std::find(tempFigureFileNames.begin(), tempFigureFileNames.end(), res) == tempFigureFileNames.end()) {
                tempFigureFileNames.push_back(res);

                auto path = std::filesystem::path(res);
                mnuFigureEditorItems.push_back("+ " + path.filename().generic_string());
            }
        }
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

    void act_FileNew() {
        timer = 0.0f;
        currentFrame = 0;
        playing = false;
        figures.clear();
        selectedStick = nullptr;
        fileName = "";
    }

    bool mnu_FileExitAction() {
        int res = CheckSavedDialog();
        if (res == 1) {
			mnu_FileSaveAction();
		}
        else if (res == 0) {
			return false;
		}
        return true;
    }

    void mnu_FileNewAction() {
        int res = CheckSavedDialog();
        if (res == 1) {
			mnu_FileSaveAction();
		}
        else if (res == 0) {
			return;
		}
        act_FileNew();
    }

    void mnu_FileOpenAction() {
		int res = CheckSavedDialog();
        if (res == 1) {
            mnu_FileSaveAction();
        }
        else if (res == 0) {
            return;
        }

        char const* filterPatterns[1] = { "*.stk" };
        auto ofdRes = tinyfd_openFileDialog(
			"Open File",
			"",
			1,
			filterPatterns,
			"StickMator Animation File",
			0
		);

        if (ofdRes) {
            act_FileNew();
            LoadAnimation(ofdRes);
            AnimateAll(currentFrame);
            fileName = ofdRes;
            isSaved = true;
		}
    }

    void mnu_EditDeleteFigureAction() {
        if (!selectedStick) return;

        int res = tinyfd_messageBox(
			"StickMator",
			"Are you sure you want to delete the selected figure?",
			"yesno",
			"question",
			0
		);
        if (res == 2) return;

        auto figPos = std::find_if(figures.begin(), figures.end(), [&](auto& fig) {
            return fig->root.get() == selectedStick->GetRoot();
        });
        if (figPos == figures.end()) return;

        undoRedo.AddCommand(
            new DelFigureCommand(this, (*figPos))
        )->Execute();
        selectedStick = nullptr;
        selectionMode = ManipulatorMode::None;
    }

    void mnu_ExportGIFAction() {
        char const* filterPatterns[1] = { "*.gif" };
		auto sfdRes = tinyfd_saveFileDialog(
            "Export GIF File",
            "",
            1,
            filterPatterns,
            "GIF Animation File"
        );
        if (sfdRes) {
            auto path = std::filesystem::path(sfdRes);
            if (!path.has_extension()) {
				path += ".gif";
			}
			SaveGIF(path.generic_string());

            tinyfd_messageBox(
				"StickMator",
				"GIF file exported successfully!",
				"ok",
				"info",
				0
			);
        }
    }

    void mnu_FileSaveAction() {
		if (isSaved) return;

        if (fileName.empty()) {
            char const* filterPatterns[1] = { "*.stk" };
            auto sfdRes = tinyfd_saveFileDialog(
                "Save File",
                "",
                1,
                filterPatterns,
                "StickMator Animation File"
            );
            if (sfdRes) {
				fileName = sfdRes;
			}
        }

        if (!fileName.empty()) {
            // add extension if not present
            if (std::filesystem::path(fileName).extension().empty()) {
				fileName += ".stk";
			}
			SaveAnimation(fileName);
            isSaved = true;
		}
    }

    void mnu_FileSaveAsAction() {
		char const* filterPatterns[1] = { "*.stk" };
        auto sfdRes = tinyfd_saveFileDialog(
			"Save File",
			"",
			1,
			filterPatterns,
			"StickMator Animation File"
		);
        if (sfdRes) {
			fileName = sfdRes;
            // add extension if not present
            if (std::filesystem::path(fileName).extension().empty()) {
                fileName += ".stk";
            }
			SaveAnimation(fileName);
			isSaved = true;
		}
	}

    void SaveAnimation(const std::string& fileName) {
        const std::string headers[] = {
            "StickMator Animation File",
            "Generated by StickMator",
            ""
        };

		CommandFile cf{};
        for (auto& fig : figures) {
			CommandFile figCf = fig->Save(true);
            for (auto& cmd : figCf.GetCommands()) {
				cf.AddCommand(cmd.name, cmd.args);
			}
		}
		cf.SaveToFile(fileName, headers, 3);
	}

    void LoadAnimation(const std::string& fileName) {
        CommandFile cf{};
        cf.LoadFromFile(fileName);

        std::vector<Command> commands = cf.GetCommands();
        for (size_t i = 0; i < commands.size(); i++) {
            auto& cmd = commands[i];
            if (cmd.name == "fig") {
                std::vector<Command> newCommands;
                while (i < commands.size() && commands[i].name != "figend") {
                    newCommands.push_back(commands[i++]);
				}

                auto figure = std::make_shared<Figure>();
                figure->id = gFigureId++;
				figure->LoadFromCommands(newCommands);
				figures.push_back(figure);
			}
            else if (cmd.name == "figend") {
                continue;
            }
		}
	}

    void SaveGIF(const std::string& fileName) {
        const int delay = 100 / FrameRate;

        GifWriter gif;
        GifBegin(&gif, fileName.c_str(), gScreenWidth, gScreenHeight, delay);

        olc::Sprite* buf = new olc::Sprite(gScreenWidth, gScreenHeight);
        SetDrawTarget(buf);

        for (int frame = 0; frame < MaxFramesAll(); frame++) {
			Clear(olc::WHITE);
			AnimateAll(frame);

            for (auto& fig : figures) {
                DrawFigure(*fig.get(), olc::BLANK, {0, 0}, false);
            }

            GifWriteFrame(&gif, (uint8_t*)GetDrawTarget()->GetData(), gScreenWidth, gScreenHeight, delay);
		}

        SetDrawTarget(nullptr);

        GifEnd(&gif);

        AnimateAll(currentFrame);
        delete buf;
	}

    float timer = 0.0f;

    std::vector<std::shared_ptr<Figure>> figures{};
    Stick* selectedStick{ nullptr };

    ManipulatorMode selectionMode{ ManipulatorMode::None };

    UndoRedo undoRedo{};

    olc::vi2d prevMouse;

    // animation
    int currentFrame = 0;

    bool stickMoved = false;
    bool playing{ false }, isSaved{false}, moving{false};

    olc::vi2d oldPos{ 0, 0 };
    double oldAngle{ 0.0 };

    olcPGEX_TinyGUI gui{};
    size_t selectedMenu{ 0 }, mnuSelFigure{ 0 }, mnuSelFile{ 0 }, mnuSelEdit{ 0 };

    std::vector<std::string> mnuFigureEditorItems{};
    std::vector<std::string> tempFigureFileNames{};

    std::string fileName{};

    int gFigureId{ 0 };
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
	StickMator demo;
	if (demo.Construct(640, 360, 2, 2))
		demo.Start();
	return 0;
}

void AddFigureCommand::Execute() {
    figure = std::make_shared<Figure>();
    figure->id = savedId == -1 ? app->gFigureId++ : savedId;
    figure->LoadFromCommands(commands);
    app->figures.push_back(figure);

    rootPos = { gScreenWidth / 2, gScreenHeight / 2 };
    figure->root->pos = rootPos;

    app->selectedStick = figure->root.get();
}

void AddFigureCommand::Undo() {
    if (!figure) return;
    
    savedId = figure->id;
    rootPos = figure->root->pos;

    app->figures.erase(std::remove_if(app->figures.begin(), app->figures.end(), [&](const std::shared_ptr<Figure>& fig) {
        return fig->id == figure->id;
    }), app->figures.end());

    app->selectedStick = nullptr;
    app->selectionMode = ManipulatorMode::None;

    figure.reset();
}

void DelFigureCommand::Execute() {
    if (!figure) return;

    savedId = figure->id;
    rootPos = figure->root->pos;
    commands = figure->Save().GetCommands();

    app->figures.erase(std::remove_if(app->figures.begin(), app->figures.end(), [&](auto& fig) {
        return fig->id == figure->id;
    }), app->figures.end());

    app->selectedStick = nullptr;
    app->selectionMode = ManipulatorMode::None;

    figure.reset();
}

void DelFigureCommand::Undo() {
    if (figure) return;

    figure = std::make_shared<Figure>();
    figure->id = savedId != -1 ? savedId : app->gFigureId++;
    figure->LoadFromCommands(commands);
    app->figures.push_back(figure);

    figure->root->pos = rootPos;

    app->selectedStick = figure->root.get();
}

void MoveStickCommand::Execute() {
    stick->pos = pos;
    stick->angle = angle;
}

void MoveStickCommand::Undo() {
    stick->pos = prevPos;
    stick->angle = prevAngle;
}

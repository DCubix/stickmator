#include "Stick.h"

#include <functional>
#include <algorithm>
#include <fstream>
#include <memory>
#include <cmath>
#include <map>

#include "olcPGEX_TinyGUI.h"

int Stick::gStickId = 100;

Stick& Stick::AddChild(int len, double angle, MotionType motionType, bool isCircle) {
    Stick* stick = new Stick();
    stick->id = gStickId++;
    stick->pos = olc::vi2d{ 0, 0 };
    stick->angle = angle;
    stick->len = len;
    stick->motionType = motionType;
    stick->isCircle = isCircle;
    stick->parent = this;
    children.push_back(std::unique_ptr<Stick>(stick));
    return *children.back().get();
}

void Stick::RemoveChild(Stick* stick) {
    auto stkPos = std::find_if(children.begin(), children.end(), [&](const std::unique_ptr<Stick>& a) {
		return a.get() == stick;
	});
	if (stkPos != children.end()) {
		children.erase(stkPos);
	}
}

Stick* Stick::GetChild(size_t id) {
    if (this->id == id) return this;

    for (auto&& child : children) {
		Stick* stk = child->GetChild(id);
        if (stk) return stk;
	}

	return nullptr;
}

Stick* Stick::GetRoot() {
    Stick* curr = this;
    while (curr->parent) {
		curr = curr->parent;
	}
    return curr;
}

olc::vi2d Stick::Tip() const {
    if (len <= 0) return olc::vi2d{ 0, 0 };
    double c = std::cos(WorldAngle()) * len;
    double s = std::sin(WorldAngle()) * len;
    return olc::vd2d{ c, s };
}

olc::vi2d Stick::WorldPos() const {
    olc::vi2d parentPos = olc::vi2d{ 0, 0 };
    if (parent) {
        parentPos = parent->WorldPos() + parent->Tip();
    }
    return pos + parentPos;
}

void Stick::SetWorldPos(olc::vi2d newPos) {
    olc::vi2d parentPos = olc::vi2d{ 0, 0 };
    if (parent) {
        parentPos = parent->WorldPos() + parent->Tip();
    }
    pos = newPos - parentPos;
}

double Stick::Angle() const {
    if (!IsDriven()) return angle;
    float offsetRadians = driverAngleOffset * Pi / 180.0f;
    return driver->angle * driverInfluence + offsetRadians;
}

double Stick::WorldAngle() const {
    double parentAngle = 0.0f;
    if (parent) {
        parentAngle = parent->WorldAngle();
    }
    return Angle() + parentAngle;
}

void Stick::SetWorldAngle(double newAngle, bool compensateChildren) {
    if (IsDriven()) return; // ignore driven sticks

    double parentAngle = 0.0f;
    if (parent) {
        parentAngle = parent->WorldAngle();
    }
    double prevAngle = angle;
    angle = newAngle - parentAngle;

    if (compensateChildren) {
        for (auto& child : children) {
            child->SetWorldAngle(child->WorldAngle() - (angle - prevAngle));
        }
    }
}

bool Stick::HasAnimation(int frame) const {
    auto stkPos = std::find_if(animation.begin(), animation.end(), [&](const StickKeyframe& a) {
        return a.frame == frame;
    });
    return stkPos != animation.end();
}

bool Stick::HasAnimationRecursive(int frame) {
    bool hasAnim = HasAnimation(frame);
    if (hasAnim) return true;

    for (auto& child : children) {
        if (child->HasAnimationRecursive(frame)) return true;
    }

    return false;
}

void Stick::Animate(int frame) {
    if (animation.empty() || IsDriven()) return;

    for (size_t i = 0; i < animation.size() - 1; i++) {
        auto& skf = animation[i];
        auto& ekf = animation[i + 1];

        if (frame >= skf.frame && frame < ekf.frame) {
            float t = float(frame - skf.frame) / (ekf.frame - skf.frame);
            angle = utils::LerpAngle(skf.angle, ekf.angle, t);
            if (!parent)
                pos = olc::vf2d{ skf.pos }.lerp(olc::vf2d{ ekf.pos }, t);
        }
    }
}

void Stick::SetKeyframe(int frame) {
    if (IsDriven()) return; // ignore driven sticks

    auto stkPos = std::find_if(animation.begin(), animation.end(), [&](const StickKeyframe& a) {
        return a.frame == frame;
    });
    if (stkPos == animation.end()) { // then add it
        StickKeyframe kf{};
        kf.frame = frame;
        kf.pos = pos;
        kf.angle = angle;
        animation.push_back(kf);
    }
    else {
        // edit it
        auto& kf = *stkPos;
        kf.pos = pos;
        kf.angle = angle;
    }

    //for (auto& child : children) {
    //    child->SetKeyframe(frame);
    //}

    auto compareKeyframes = [](const StickKeyframe& a, const StickKeyframe& b) {
        return a.frame < b.frame;
    };
    std::sort(animation.begin(), animation.end(), compareKeyframes);
}

void Stick::SetKeyframeSingle(int frame, const olc::vi2d& pos, double angle) {
    if (IsDriven()) return; // ignore driven sticks

    auto stkPos = std::find_if(animation.begin(), animation.end(), [&](const StickKeyframe& a) {
        return a.frame == frame;
    });
    if (stkPos == animation.end()) { // then add it
    	StickKeyframe kf{};
    	kf.frame = frame;
    	kf.pos = pos;
    	kf.angle = angle;
    	animation.push_back(kf);
    }
    else {
		// edit it
		auto& kf = *stkPos;
		kf.pos = pos;
		kf.angle = angle;
	}

    auto compareKeyframes = [](const StickKeyframe& a, const StickKeyframe& b) {
        return a.frame < b.frame;
    };
    std::sort(animation.begin(), animation.end(), compareKeyframes);
}

void Stick::DeleteKeyframe(int frame) {
    if (IsDriven()) return; // ignore driven sticks

    auto stkPos = std::find_if(animation.begin(), animation.end(), [&](const StickKeyframe& a) {
		return a.frame == frame;
	});
	if (stkPos != animation.end()) {
		animation.erase(stkPos);
	}

	for (auto& child : children) {
		child->DeleteKeyframe(frame);
	}
}

int Stick::MaxFrames() const {
    int maxFram = 0;
    for (auto& kf : animation) {
        maxFram = std::max(maxFram, kf.frame);
    }

    for (auto& child : children) {
        maxFram = std::max(maxFram, child->MaxFrames());
    }
    return maxFram;
}

int Stick::NearFrameLeft(int frame) const {
    if (animation.empty()) return 0;

    if (frame > 0) frame--;
    int lastFrame = animation.back().frame;
    for (size_t i = 0; i < animation.size() - 1; i++) {
        auto& skf = animation[i];
        auto& ekf = animation[i + 1];

        if (frame >= skf.frame && frame < ekf.frame) {
            return skf.frame;
        }

        lastFrame = skf.frame;
    }
    return lastFrame;
}

int Stick::NearFrameRight(int frame) const {
    if (animation.empty()) return 0;

    if (frame > 0) frame--;
    int lastFrame = animation.back().frame;
    for (size_t i = 0; i < animation.size() - 1; i++) {
        auto& skf = animation[i];
        auto& ekf = animation[i + 1];

        if (frame >= skf.frame && frame < ekf.frame) {
            return ekf.frame;
        }

        lastFrame = ekf.frame;
    }
    return lastFrame;
}

void Stick::SaveState() {
    if (IsDriven()) return; // ignore driven sticks
    tempAngle = angle;
    tempPos = pos;
    for (auto& child : children) {
        child->SaveState();
    }
}

void Stick::RestoreState() {
    if (IsDriven()) return; // ignore driven sticks
    angle = tempAngle;
    pos = tempPos;
    for (auto& child : children) {
        child->RestoreState();
    }
}

static void DrawThickLine(
    olc::PixelGameEngine* pge,
    const olc::vi2d& p1, const olc::vi2d& p2,
    const olc::Pixel& color = olc::BLACK,
    int width = 3
) {
    auto steps = (p2 - p1).mag() / width;

    auto fp1 = olc::vf2d{ p1 };
    auto fp2 = olc::vf2d{ p2 };
    for (size_t i = 0; i < steps; i++) {
        float fac = float(i) / steps;
        auto p = fp1.lerp(fp2, fac);
        pge->FillCircle(p, width, color);
    }
}

void Stick::Draw(olc::PixelGameEngine* pge, Stick* selected, const olc::vi2d& offset, const olc::Pixel& colorOverride) {
    if (len <= 0 || !isVisible || isDriver) return;

    olc::Pixel color = colorOverride.a > 0 ? colorOverride : this->color;
    if (!isCircle) {
        if (selected == this) {
            DrawThickLine(pge, WorldPos() + offset, WorldPos() + Tip() + offset, olc::BLUE, 4);
        }
        DrawThickLine(pge, WorldPos() + offset, WorldPos() + Tip() + offset, color, 3);
    }
    else {
        if (selected == this) {
            pge->FillCircle(WorldPos() + Tip() / 2 + offset, len / 2 + 1, olc::BLUE);
        }
        pge->FillCircle(WorldPos() + Tip() / 2 + offset, len / 2, color);
    }
}

void Stick::DrawManipulators(olc::PixelGameEngine* pge, const olc::vi2d& offset) {
    if (canMove()) {
        if (parent && len > 0) {
            auto col = motionType == MotionType::Kinematic ? olc::CYAN : olc::RED;
            pge->FillCircle(WorldPos() + Tip() + offset, 2, col);
        }
        else {
            auto orange = olc::Pixel(255, 165, 0);
            pge->FillCircle(WorldPos() + offset, 2, orange);
        }
    }
    else {
        auto grey = olc::Pixel(128, 128, 128);
		pge->FillCircle(WorldPos() + offset, 2, grey);
    }
}

void Stick::DrawManipulatorsEditor(olc::PixelGameEngine* pge) {
    if (parent && len > 0) {
        auto col = motionType == MotionType::Kinematic ? olc::CYAN : olc::RED;
        if (!canMove()) col = olc::GREY;
        pge->FillCircle(WorldPos() + Tip(), 2, col);
    }
    else {
        auto orange = olc::Pixel(255, 165, 0);
        pge->FillCircle(WorldPos(), 2, orange);
    }
}

static bool PointInCircle(olc::vi2d pt, olc::vi2d center, int radius) {
    return (pt - center).mag() <= radius;
}

std::pair<ManipulatorMode, Stick*> Stick::GetStickForManipulation(
    olc::PixelGameEngine* pge,
    const olc::vi2d& offset,
    bool bypassMotionCheck
) {
    olc::vi2d mousePosRel = pge->GetMousePos() - offset;
    bool canRotate = canMove();
    if (!parent) {
        if (pge->GetMouse(0).bPressed && PointInCircle(mousePosRel, WorldPos(), 3)) {
            return { ManipulatorMode::Move, this };
        }
        else if (pge->GetMouse(0).bReleased) {
            return { ManipulatorMode::None, nullptr };
        }
    }
    else {
        if (pge->GetMouse(0).bPressed && PointInCircle(mousePosRel, WorldPos() + Tip(), 3)) {
            if (!bypassMotionCheck && canRotate) {
                return { ManipulatorMode::Rotate, this };
            }
            else if (bypassMotionCheck) {
                return { ManipulatorMode::Rotate, this };
            }
        }
        else if (pge->GetMouse(0).bReleased) {
            return { ManipulatorMode::None, nullptr };
        }
    }

    return { ManipulatorMode::None, nullptr };
}

std::vector<Stick*> Stick::GetSticksRecursive() {
    std::vector<Stick*> sticks;
	sticks.push_back(this);
    for (auto& child : children) {
		auto childSticks = child->GetSticksRecursive();
		sticks.insert(sticks.end(), childSticks.begin(), childSticks.end());
	}
	return sticks;
}

std::vector<Stick*> Stick::GetSticksRecursiveSorted() {
    std::vector<Stick*> sticks;
	sticks.push_back(this);
    for (auto& child : children) {
		auto childSticks = child->GetSticksRecursiveSorted();
		sticks.insert(sticks.end(), childSticks.begin(), childSticks.end());
	}

	// sort by id
    std::sort(sticks.begin(), sticks.end(), [](const Stick* a, const Stick* b) {
		return a->drawOrder < b->drawOrder;
	});

	return sticks;
}

std::vector<Stick*> Stick::GetSticksRecursiveVisibleSorted() {
    std::vector<Stick*> sticks;
    if (isVisible) sticks.push_back(this);
    for (auto& child : children) {
        auto childSticks = child->GetSticksRecursiveVisibleSorted();
        for (auto& childStick : childSticks) {
            if (childStick->isVisible) sticks.push_back(childStick);
		}
	}

    // sort by draw order
    std::sort(sticks.begin(), sticks.end(), [](const Stick* a, const Stick* b) {
		return a->drawOrder < b->drawOrder;
	});

    return sticks;
}

void Figure::LoadFromString(const std::string& data) {
    CommandFile cf;
    cf.LoadFromString(data);
    LoadFromCommands(cf.GetCommands());
}

void Figure::LoadFromFile(const std::string& fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) return;

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    LoadFromString(data);
}

void Figure::LoadFromCommands(const std::vector<Command>& commands) {
    std::map<int, std::unique_ptr<Stick>> sticks;

    root = std::make_unique<Stick>();
    root->id = 0;
    root->pos.x = 0;
    root->pos.y = 0;
    root->angle = 0;
    root->len = 0;

    for (auto&& cmd : commands) {
        if (cmd.name == "fig") {
            name = cmd.GetArg<std::string>(0);
        }
        else if (cmd.name == "s") {
            // stick definition
            // s <id> <len> <angle deg> [<motion-type:none / normal / kinematic> <is-circle:1 / 0> <colorhex>]
            int id = int(cmd.GetArg<double>(0));
            int len = int(cmd.GetArg<double>(1));
            double angle = cmd.GetArg<double>(2);

            if (id == 0) {
                throw std::runtime_error("Root stick cannot be defined");
            }

            std::string motionType = "normal";
            bool isCircle = false, isDriver = false, isVisible = false;
            int drawOrder = 0;
            std::string colorHex = "#000000";

            motionType = cmd.GetOptionalArg<std::string>(3, "normal");
            std::transform(motionType.begin(), motionType.end(), motionType.begin(), ::tolower);

            isCircle = cmd.GetOptionalArg<bool>(4, false);
            colorHex = cmd.GetOptionalArg<std::string>(5, "#000000");
            isDriver = cmd.GetOptionalArg<bool>(6, false);
            isVisible = cmd.GetOptionalArg<bool>(7, true);
            drawOrder = int(cmd.GetOptionalArg<double>(8, 0.0));

            olc::Pixel color = olc::BLACK;
            if (colorHex[0] == '#' && colorHex.size() == 7) {
				uint8_t r = std::stoi(colorHex.substr(1, 2), nullptr, 16);
                uint8_t g = std::stoi(colorHex.substr(3, 2), nullptr, 16);
                uint8_t b = std::stoi(colorHex.substr(5, 2), nullptr, 16);
                color = olc::Pixel(r, g, b);
			}

            auto stick = std::make_unique<Stick>();
            stick->id = id;
            stick->pos.x = 0;
            stick->pos.y = 0;
            stick->angle = (angle * Pi / 180.0f);
            stick->len = len;
            stick->isCircle = isCircle;
            stick->color = color;
            stick->isDriver = isDriver;
            stick->isVisible = isVisible;
            stick->drawOrder = drawOrder;

            if (motionType == "none") {
                stick->motionType = MotionType::None;
            }
            else if (motionType == "normal") {
                stick->motionType = MotionType::Normal;
            }
            else if (motionType == "kinematic") {
                stick->motionType = MotionType::Kinematic;
            }

            sticks[id] = std::move(stick);
        }
        else if (cmd.name == "d") {
            // stick drivers
            // d <stick-id> <driver-id> <influence> [<angle-offset>]
            int stickId = int(cmd.GetArg<double>(0));
            int driverId = int(cmd.GetArg<double>(1));
            float influence = float(cmd.GetOptionalArg<double>(2, 1.0));
            float angleOffset = float(cmd.GetOptionalArg<double>(3, 0.0));

            Stick* stick = root->GetChild(stickId);
            Stick* driver = root->GetChild(driverId);

            if (!stick || !driver) {
				throw std::runtime_error("Stick or driver not found");
			}

            stick->driver = driver;
            stick->driverInfluence = influence;
            stick->driverAngleOffset = angleOffset;
            driver->isDriver = true;
        }
        else if (cmd.name == "r") {
            // stick relationships
            // r <parent-id> <child-id>
            // assumes root = 0

            int parentId = int(cmd.GetArg<double>(0));
            int childId = int(cmd.GetArg<double>(1));

            auto& child = sticks[childId];

            if (parentId == 0) {
                child->parent = root.get();
                root->children.push_back(std::move(child));
            }
            else {
                Stick* parent = sticks[parentId].get();
                if (!parent) {
                    parent = root->GetChild(parentId);
                }
                parent->children.push_back(std::move(child));
                parent->children.back()->parent = parent;
            }
        }
        else if (cmd.name == "figend") {
            continue;
		}
        else if (cmd.name == "kf") { // keyframe
            // kf <stick-id> <frame> <pos.x> <pos.y> <angle>
            int id = int(cmd.GetArg<double>(0));
            int frame = int(cmd.GetArg<double>(1));
            int posX = int(cmd.GetArg<double>(2)),
                posY = int(cmd.GetArg<double>(3));
            double angle = cmd.GetArg<double>(4);

            Stick* stick = root->GetChild(id);
            if (stick) {
				stick->SetKeyframeSingle(frame, olc::vi2d{ posX, posY }, angle * Pi / 180.0f);
			}
        }
        else if (cmd.name == "pos") {
			root->pos.x = int(cmd.GetArg<double>(0));
			root->pos.y = int(cmd.GetArg<double>(1));
		}
    }
}

static void SaveStick(CommandFile& cf, Stick* stick) {
    if (!stick->parent) return;

    std::vector<CommandArg> args;
    args.push_back(double(stick->id));
    args.push_back(double(stick->len));
    args.push_back(stick->angle * 180.0 / Pi);
        
    switch (stick->motionType) {
        case MotionType::None: args.push_back("none"); break;
        case MotionType::Normal: args.push_back("normal"); break;
        case MotionType::Kinematic: args.push_back("kinematic"); break;
    }

    args.push_back(stick->isCircle);
    args.push_back(utils::StringFormat("#%02X%02X%02X", stick->color.r, stick->color.g, stick->color.b));

    args.push_back(stick->isDriver);
    args.push_back(stick->isVisible);
    args.push_back(double(stick->drawOrder));

    cf.AddCommandVec("s", args);
}

static void SaveSticks(CommandFile& cf, Stick* stick) {
    Stick* root = stick->GetRoot();
    auto sticks = root->GetSticksRecursive();

    // sort it in a way that drivers are saved first before the driven sticks
    // then for non-driven sticks, sort them by their ids
    std::sort(sticks.begin(), sticks.end(), [](const Stick* a, const Stick* b) {
        if (a->IsDriven() && !b->IsDriven()) return true;
        if (!a->IsDriven() && b->IsDriven()) return false;
        return a->id < b->id;
    });

    for (auto& stick : sticks) {
		SaveStick(cf, stick);
	}
}

static void SaveRelations(CommandFile& cf, Stick* stick) {
    for (auto& child : stick->children) {
		cf.AddCommand("r", double(stick->id), double(child->id));
		SaveRelations(cf, child.get());
	}
}

static void SaveDriverDecl(CommandFile& cf, Stick* stick) {
    if (stick->IsDriven()) {
		cf.AddCommand("d", double(stick->id), double(stick->driver->id), stick->driverInfluence, stick->driverAngleOffset);
	}

    for (auto& child : stick->children) {
		SaveDriverDecl(cf, child.get());
	}
}

static void SaveKeyframes(CommandFile& cf, Stick* stick) {
    for (auto& kf : stick->animation) {
		cf.AddCommand(
            "kf",
            double(stick->id),
            double(kf.frame),
            double(kf.pos.x), double(kf.pos.y),
            kf.angle * 180.0 / Pi
        );
	}

    for (auto& child : stick->children) {
		SaveKeyframes(cf, child.get());
	}
}

void Figure::SaveToFile(const std::string& fileName) {
    const std::string headers[] = {
        "Figure definition file",
        "Generated by StickMator's Figure Editor",
        ""
    };

    CommandFile cf = Save();
    cf.SaveToFile(fileName, headers, 3);
}

CommandFile Figure::Save(bool saveRootPosition) const {
    CommandFile cf;
    cf.AddCommand("fig", name);

    if (saveRootPosition) {
        cf.AddCommand("pos", double(root->pos.x), double(root->pos.y));
    }

    SaveSticks(cf, root.get());
    SaveRelations(cf, root.get());
    SaveDriverDecl(cf, root.get());
    SaveKeyframes(cf, root.get());

    cf.AddCommand("figend", name);
    return cf;
}

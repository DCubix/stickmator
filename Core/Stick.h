#pragma once

#include "Utility.h"
#include "olcPixelGameEngine.h"

#include "CommandFile.h"

#include <string>

struct StickKeyframe {
    int frame;
    olc::vi2d pos;
    double angle;
};

enum class MotionType {
    None = 0,
    Normal,
    Kinematic
};

enum class ManipulatorMode {
    None = 0,
    Move,
    Rotate
};

struct Stick {
    size_t id{ 0 };

    olc::vi2d pos;
    double angle{ 0.0f };
    int len{ 0 };
    olc::Pixel color{ olc::BLACK };

    bool isCircle{ false };

    MotionType motionType{ MotionType::Normal };

    olc::vi2d tempPos;
    double tempAngle;

    Stick* parent{ nullptr };
    std::vector<std::unique_ptr<Stick>> children;

    std::vector<StickKeyframe> animation{};

    Stick& AddChild(int len, double angle, MotionType motionType = MotionType::Normal, bool isCircle = false);
    void RemoveChild(Stick* stick);
    Stick* GetChild(size_t id);
    Stick* GetRoot();

    olc::vi2d Tip() const;
    olc::vi2d WorldPos() const;
    void SetWorldPos(olc::vi2d newPos);
    double WorldAngle() const;
    void SetWorldAngle(double newAngle, bool compensateChildren = false);

    bool HasAnimation(int frame) const;
    bool HasAnimationRecursive(int frame);

    void Animate(int frame);

    void SetKeyframe(int frame);
    void SetKeyframeSingle(int frame, const olc::vi2d& pos, double angle);
    void DeleteKeyframe(int frame);

    int MaxFrames() const;

    int NearFrameLeft(int frame) const;
    int NearFrameRight(int frame) const;

    void SaveState();
    void RestoreState();

    void Draw(
        olc::PixelGameEngine* pge,
        Stick* selected = nullptr,
        const olc::vi2d& offset = { 0, 0 },
        const olc::Pixel& colorOverride = olc::BLANK
    );
    void DrawManipulators(
        olc::PixelGameEngine* pge,
        const olc::vi2d& offset = { 0, 0 }
    );
    void DrawManipulatorsEditor(olc::PixelGameEngine* pge);

    std::pair<ManipulatorMode, Stick*> GetStickForManipulation(
        olc::PixelGameEngine* pge,
        const olc::vi2d& offset,
        bool bypassMotionCheck = false
    );

    static int gStickId;
};

struct Figure {
    int id{ 0 };

    std::string name;
    std::unique_ptr<Stick> root;

    Figure() {}

    /// <summary>
    /// Loads a stick figure from a string
    /// </summary>
    /// <param name="data"></param>
    void LoadFromString(const std::string& data);

    /// <summary>
    /// Loads a stick figure from a file
    /// </summary>
    /// <param name="fileName"></param>
    void LoadFromFile(const std::string& fileName);

    void LoadFromCommands(const std::vector<Command>& commands);

    /// <summary>
    /// Saves a stick figure to a file
    /// </summary>
    /// <param name="fileName"></param>
    void SaveToFile(const std::string& fileName);

    /// <summary>
    /// Saves a stick figure to a command file
    /// </summary>
    /// <returns></returns>
    CommandFile Save(bool saveRootPosition = false) const;
};

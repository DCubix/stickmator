#pragma once

#include "olcPixelGameEngine.h"

#include <optional>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>

#ifdef DrawText
#undef DrawText
#endif

namespace utils {
    template <typename... Args>
    std::string StringFormat(const std::string& format, Args... args) {
        size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        std::string result(size, '\0');
        std::snprintf(result.data(), size, format.c_str(), args...);
        return result.substr(0, size - 1);
    }

    float Luma(const olc::Pixel& color);
}

#pragma region Font
constexpr uint8_t FONT_X = 4;
constexpr uint8_t FONT_Y = 6;
constexpr uint8_t FONT_OFF = 0x20;
constexpr uint8_t FONT_CHRCOUNT = 0x5f;
constexpr uint8_t FONT[] = {
    0x00, 0x00, 0x00, 0x03, 0xa0, 0x00, 0xc0, 0x0c, 0x00, 0xf9, 0x4f, 0x80, 0x6b, 0xeb, 0x00, 0x98, 0x8c, 0x80, 0x52, 0xa5, 0x80, 0x03, 0x00, 0x00,  // Space, !"#$%&'
    0x01, 0xc8, 0x80, 0x89, 0xc0, 0x00, 0x50, 0x85, 0x00, 0x21, 0xc2, 0x00, 0x08, 0x40, 0x00, 0x20, 0x82, 0x00, 0x00, 0x20, 0x00, 0x18, 0x8c, 0x00,  // ()*+,-./
    0xfa, 0x2f, 0x80, 0x4b, 0xe0, 0x80, 0x5a, 0x66, 0x80, 0x8a, 0xa5, 0x00, 0xe0, 0x8f, 0x80, 0xea, 0xab, 0x00, 0x72, 0xa9, 0x00, 0x9a, 0x8c, 0x00,  // 01234567
    0xfa, 0xaf, 0x80, 0x4a, 0xa7, 0x00, 0x01, 0x40, 0x00, 0x09, 0x40, 0x00, 0x21, 0x48, 0x80, 0x51, 0x45, 0x00, 0x89, 0x42, 0x00, 0x42, 0x66, 0x00,  // 89:;<=>?
    0x72, 0xa6, 0x80, 0x7a, 0x87, 0x80, 0xfa, 0xa5, 0x00, 0x72, 0x25, 0x00, 0xfa, 0x27, 0x00, 0xfa, 0xa8, 0x80, 0xfa, 0x88, 0x00, 0x72, 0x2b, 0x00,  // @ABCDEFG
    0xf8, 0x8f, 0x80, 0x8b, 0xe8, 0x80, 0x8b, 0xe8, 0x00, 0xf8, 0x8d, 0x80, 0xf8, 0x20, 0x80, 0xf9, 0x0f, 0x80, 0xf9, 0xcf, 0x80, 0x72, 0x27, 0x00,  // HIJKLMNO
    0xfa, 0x84, 0x00, 0x72, 0x27, 0x40, 0xfa, 0x85, 0x80, 0x4a, 0xa9, 0x00, 0x83, 0xe8, 0x00, 0xf0, 0x2f, 0x00, 0xe0, 0x6e, 0x00, 0xf0, 0xef, 0x00,  // PQRSTUVW
    0xd8, 0x8d, 0x80, 0xc0, 0xec, 0x00, 0x9a, 0xac, 0x80, 0x03, 0xe8, 0x80, 0xc0, 0x81, 0x80, 0x8b, 0xe0, 0x00, 0x42, 0x04, 0x00, 0x08, 0x20, 0x80,  // XYZ[\]^_
    0x02, 0x04, 0x00, 0x31, 0x23, 0x80, 0xf9, 0x23, 0x00, 0x31, 0x24, 0x80, 0x31, 0x2f, 0x80, 0x31, 0x62, 0x80, 0x23, 0xea, 0x00, 0x25, 0x53, 0x80,  // `abcdefg
    0xf9, 0x03, 0x80, 0x02, 0xe0, 0x00, 0x06, 0xe0, 0x00, 0xf8, 0x42, 0x80, 0x03, 0xe0, 0x00, 0x79, 0x87, 0x80, 0x39, 0x03, 0x80, 0x31, 0x23, 0x00,  // hijklmno
    0x7d, 0x23, 0x00, 0x31, 0x27, 0xc0, 0x78, 0x84, 0x00, 0x29, 0x40, 0x00, 0x43, 0xe4, 0x00, 0x70, 0x27, 0x00, 0x60, 0x66, 0x00, 0x70, 0x67, 0x00,  // pqrstuvw
    0x48, 0xc4, 0x80, 0x74, 0x57, 0x80, 0x59, 0xe6, 0x80, 0x23, 0xe8, 0x80, 0x03, 0x60, 0x00, 0x8b, 0xe2, 0x00, 0x61, 0x0c, 0x00                     // zyx{|}~
};
#pragma endregion

struct Rect {
    int x, y, width, height;

    bool HasPoint(int x, int y) const;
    bool HasPoint(const olc::vi2d& pos) const;
    Rect Expand(int amount = 1);

    olc::vi2d Position() const { return { x, y }; }
    olc::vi2d Size() const { return { width, height }; }
};

class olcPGEX_TinyGUI : olc::PGEX {
public:
    explicit olcPGEX_TinyGUI() : olc::PGEX(true) {}

    // ------- WIDGETS -------
    void Label(
        Rect bounds,
        const std::string& text,
        bool alignFar = false,
        olc::Pixel bg = olc::Pixel(0, 0, 0, 0)
    );
    bool Button(
        const std::string& name,
        Rect bounds,
        const std::string& text,
        const std::optional<olc::Pixel>& color = std::nullopt
    );
    bool Slider(
        const std::string& name,
        Rect bounds,
        int& value, int min = 0, int max = 100,
        /* tickDrawFn(pge, value, min, max, bounds) */
        std::function<void(olc::PixelGameEngine*, int, int, int, Rect)> tickDrawFn = nullptr
    );
    bool Toggle(const std::string& name, Rect bounds, const std::string& text, bool& value);
    bool Spinner(
        const std::string& name,
        Rect bounds,
        int& value, int min = 0, int max = 100, int step = 1,
        const std::string& fmt = "%d",
        const olc::Pixel& bg = olc::Pixel(0, 0, 0, 0)
    );
    bool SpinnerF(
        const std::string& name,
		Rect bounds,
		float& value, float min = 0.0f, float max = 1.0f, float step = 0.01f,
		const std::string& fmt = "%.2f",
		const olc::Pixel& bg = olc::Pixel(0, 0, 0, 0)
    );
    bool EditBox(const std::string& name, Rect bounds, std::string& text);

    bool MakePopup(
        const std::string& name,
        std::string* items, size_t numItems, size_t& selected
    );
    void ShowPopup(const std::string& name, olc::vi2d position);
    void ShowPopup(const std::string& name);

    bool WidgetPressed(const std::string& name) {
		return GetWidgetByName(name).pressed;
	}

    bool WidgetReleased(const std::string& name) {
        return GetWidgetByName(name).released;
    }

    // ------- LAYOUT -------
    void PushRect(Rect rect);
    Rect& PeekRect();
    void PopRect();
    Rect RectCutLeft(int amount);
    Rect RectCutRight(int amount);
    Rect RectCutTop(int amount);
    Rect RectCutBottom(int amount);

    // ------- DRAWING -------
    olc::Pixel PixelBrightness(olc::Pixel color, float amount);
    void DrawChar(int x, int y, char value, olc::Pixel color = olc::WHITE);
    void DrawText(
        int x, int y,
        const std::string& text,
        olc::Pixel color = olc::WHITE
    );
    std::pair<int, int> TextSize(const std::string& text);

    // ------- STATE -------
    bool OnBeforeUserUpdate(float& fElapsedTime) override;
    void OnAfterUserUpdate(float fElapsedTime) override;

    olc::Pixel baseColor{ olc::Pixel(95, 134, 176) };

    bool GetMouseState() const { return m_state.mouseDown; }
    std::vector<std::string> GetLogs() const { return m_logs; }

private:
    enum class WidgetState {
        Idle = 0,
        Hovered,
        Active,
        Clicked
    };

    struct Widget {
        size_t id;
        WidgetState state{ WidgetState::Idle };
        bool pressed{ false }, released{ false };
        int value{ 0 };
    };

    struct Popup {
        std::string name;
        olc::vi2d position;
        olc::vi2d size;
        std::vector<std::string> items;
    };

    struct {
        size_t hoveredId, activeId, focusedId;
        bool mouseDown;
        olc::vi2d mousePos, lastClickedWidgetPosition, mouseDelta;
    } m_state{};

    Widget& GetWidget(const std::string& name, Rect bounds, bool blockInputByPopup = true);
    Widget& GetWidgetByName(const std::string& name);

    void DrawPopups();

    void ThreeDFrame(Rect bounds, olc::Pixel color, WidgetState state);

    template <typename T>
    bool SpinnerImpl(
        const std::string& name,
        Rect bounds,
        const std::function<void(T&, T)> fnIncrement,
        T& value, T min, T max, T step,
        const std::string& fmt = "%d",
        const olc::Pixel& bg = olc::Pixel(0, 0, 0, 0)
    ) {
        const auto dark = PixelBrightness(baseColor, 0.5f);
        const int thumbWidth = 8;

        ThreeDFrame(
            Rect{
                bounds.x + 1, bounds.y,
                bounds.width - 2, bounds.height
            },
            bg.a > 0 ? bg : dark,
            bg.a > 0 ? WidgetState::Idle : WidgetState::Active
        );

        auto wid = GetWidget(name, Rect{ bounds.x + thumbWidth, bounds.y, bounds.width - thumbWidth * 2, bounds.height });

        bool changed = false;
        // left button
        if (Button(name + "_left", Rect{ bounds.x, bounds.y, thumbWidth, bounds.height }, "-")) {
            fnIncrement(value, -step);
            value = std::clamp(value, min, max);
            changed = true;
        }

        // right button
        if (Button(name + "_right", Rect{ bounds.x + bounds.width - thumbWidth, bounds.y, thumbWidth, bounds.height }, "+")) {
            fnIncrement(value, step);
            value = std::clamp(value, min, max);
            changed = true;
        }

        if (wid.state == WidgetState::Active) {
            fnIncrement(value, T(m_state.mouseDelta.x * step));
            value = std::clamp(value, min, max);
            changed = true;
        }

        wid.value = value;

        auto text = utils::StringFormat(fmt.c_str(), value);
        auto [tw, th] = TextSize(text);

        float luma = utils::Luma(bg);
        auto textColor = luma >= 0.5f ? PixelBrightness(baseColor, 0.15f) : PixelBrightness(baseColor, 2.5f);

        DrawText(
            bounds.x + bounds.width / 2 - tw / 2,
            bounds.y + bounds.height / 2 - th / 2,
            text,
            textColor
        );

        return changed;
    }
    
    size_t m_openPopup{ 0 };

    float m_blinkTimer{ 0.0f };
    bool m_blink{ false };

    std::map<size_t, Widget> m_widgets;
    std::map<size_t, Popup> m_popups;
    std::vector<Rect> m_rectStack;

    std::vector<std::string> m_logs;

    void Log(const std::string& text) {
        if (!m_logs.empty() && m_logs.back() == text) return;
		m_logs.push_back(text);
        if (m_logs.size() > 20) m_logs.erase(m_logs.begin());
	}
};

#ifdef OLC_PGEX_TINYGUI
#undef OLC_PGEX_TINYGUI

Rect Rect::Expand(int amount) {
	Rect rec = *this;
	rec.x -= amount;
	rec.y -= amount;
	rec.width += amount * 2;
	rec.height += amount * 2;
	return rec;
}

bool Rect::HasPoint(int x, int y) const {
	return x >= this->x && x < this->x + this->width && y >= this->y && y < this->y + this->height;
}

bool Rect::HasPoint(const olc::vi2d& pos) const {
	return HasPoint(pos.x, pos.y);
}

void olcPGEX_TinyGUI::Label(Rect bounds, const std::string& text, bool alignFar, olc::Pixel bg) {
    auto [tw, th] = TextSize(text);
    if (bg.a >= 1) {
        pge->FillRect(
            olc::vi2d{ bounds.x, bounds.y },
            olc::vi2d{ bounds.width, bounds.height },
            bg
        );
    }
    const float luma = utils::Luma(bg);
    const auto col = (bg.a >= 1) ? (luma >= 0.5f ? PixelBrightness(baseColor, 0.15f) : PixelBrightness(baseColor, 2.5f)) : PixelBrightness(baseColor, 0.15f);
    int offX = alignFar ? bounds.width - tw : 1;
    DrawText(bounds.x + offX, bounds.y + bounds.height / 2 - th / 2, text, col);
}

bool olcPGEX_TinyGUI::Button(const std::string& name, Rect bounds, const std::string& text, const std::optional<olc::Pixel>& color) {
    auto& wid = GetWidget(name, bounds);
    ThreeDFrame(bounds, baseColor, wid.state);

    auto [tw, th] = TextSize(text);

    const auto light = PixelBrightness(baseColor, 2.5f);
    const auto dark = PixelBrightness(baseColor, 0.15f);

    olc::Pixel textColor = wid.state == WidgetState::Idle || wid.state == WidgetState::Hovered ? dark : light;
    if (color.has_value()) {
        textColor = utils::Luma(color.value()) >= 0.5f ? dark : light;

        Rect innerBounds = bounds.Expand(-1);
        pge->FillRect(
            innerBounds.Position(),
            innerBounds.Size(),
            color.value()
        );
	}

    DrawText(
        bounds.x + bounds.width / 2 - tw / 2 + 1,
        bounds.y + bounds.height / 2 - th / 2 + 1,
        text,
        textColor
    );

    return wid.state == WidgetState::Clicked;
}

bool olcPGEX_TinyGUI::Toggle(const std::string& name, Rect bounds, const std::string& text, bool& value) {
    auto& wid = GetWidget(name, bounds);
    ThreeDFrame(bounds, baseColor, !value ? wid.state : WidgetState::Active);

    if (wid.state == WidgetState::Clicked) {
        value = !value;
    }

    auto [tw, th] = TextSize(text);

    const auto light = PixelBrightness(baseColor, 2.5f);
    const auto dark = PixelBrightness(baseColor, 0.15f);
    DrawText(
        bounds.x + bounds.width / 2 - tw / 2 + 1,
        bounds.y + bounds.height / 2 - th / 2 + 1,
        text,
        wid.state == WidgetState::Active || wid.state == WidgetState::Clicked || value ? light : dark
    );

    return wid.state == WidgetState::Clicked;
}

bool olcPGEX_TinyGUI::Slider(
    const std::string& name, Rect bounds,
    int& value, int min, int max,
    std::function<void(olc::PixelGameEngine*, int, int, int, Rect)> tickDrawFn
) {
    constexpr int thumbWidth = 7;
    constexpr int thumbHeight = 12;

    auto& wid = GetWidget(name, bounds);

    int oldVal = value;

    if (wid.state == WidgetState::Active) {
        int relX = m_state.mousePos.x - (bounds.x + thumbWidth / 2);
        float fac = float(relX) / (bounds.width - thumbWidth);
        value = min + int(fac * (max - min));
        value = std::clamp(value, min, max);
    }

    if (tickDrawFn) {
        Rect actualBounds{
            bounds.x + thumbWidth / 2, bounds.y,
            bounds.width - thumbWidth, bounds.height
        };
        for (int i = min; i <= max; i++) {
			tickDrawFn(pge, i, min, max, actualBounds);
		}
    }

    pge->DrawLine(
        bounds.x + thumbWidth / 2, bounds.y + bounds.height / 2,
        bounds.x + bounds.width - thumbWidth / 2, bounds.y + bounds.height / 2,
        PixelBrightness(baseColor, 0.15f)
    );

    auto state = wid.state == WidgetState::Active || wid.state == WidgetState::Clicked ?
        WidgetState::Hovered : wid.state;

    float fac = float(value - min) / (max - min);
    int thumbX = int(fac * (bounds.width - thumbWidth));
    ThreeDFrame(
        Rect{ bounds.x + thumbX, bounds.y + bounds.height / 2 - thumbHeight / 2, thumbWidth, thumbHeight },
        baseColor, state
    );

    return oldVal != value;
}

bool olcPGEX_TinyGUI::Spinner(
    const std::string& name,
    Rect bounds,
    int& value, int min, int max, int step,
    const std::string& fmt,
    const olc::Pixel& bg
) {
    return SpinnerImpl<int>(
		name, bounds,
		[&](int& val, int inc) { val += inc; },
        value, min, max, step, fmt, bg
	);
}

bool olcPGEX_TinyGUI::SpinnerF(const std::string& name, Rect bounds, float& value, float min, float max, float step, const std::string& fmt, const olc::Pixel& bg) {
    return SpinnerImpl<float>(
		name, bounds,
		[&](float& val, float inc) { val += inc; },
        value, min, max, step, fmt, bg
	);
}

bool olcPGEX_TinyGUI::EditBox(const std::string& name, Rect bounds, std::string& text) {
    auto& wid = GetWidget(name, bounds);

    const auto light = PixelBrightness(baseColor, 2.5f);
    bool textChanged = false;
    bool focused = m_state.focusedId == wid.id;

    ThreeDFrame(bounds, baseColor, WidgetState::Active);

    if (focused && !pge->IsTextEntryEnabled()) {
        pge->TextEntryEnable(true, text);
    }

    // text entry
    if (focused) {
        textChanged = pge->TextEntryGetString() != text;
        text = pge->TextEntryGetString();
    }
    DrawText(bounds.x + 2, bounds.y + bounds.height / 2 - FONT_Y / 2, text, light);

    // draw cursor
    if (focused && m_blink) {
        int cursorX = bounds.x + 2 + pge->TextEntryGetCursor() * FONT_X;
        DrawChar(cursorX, bounds.y + bounds.height / 2 - FONT_Y / 2, '_', light);
        DrawChar(cursorX, bounds.y + bounds.height / 2 - FONT_Y / 2 - 1, '_', light);
    }

    if (!focused && pge->IsTextEntryEnabled()) {
        pge->TextEntryEnable(false);
	}

    return textChanged;
}

bool olcPGEX_TinyGUI::MakePopup(const std::string& name, std::string* items, size_t numItems, size_t& selected) {
    auto id = std::hash<std::string>()(name);
    auto&& pos = m_popups.find(id);
    if (pos == m_popups.end()) {
        m_popups[id] = Popup();
        pos = m_popups.find(id);
    }

    auto& popup = pos->second;
    popup.name = name;
    popup.items = std::vector<std::string>(items, items + numItems);

    const int itemHeight = 11;

    int width = 0, height = 0;
    for (size_t i = 0; i < numItems; i++) {
        auto item = items[i];

        width = std::max(width, TextSize(item).first);

        if (item.find_first_not_of('-') == std::string::npos) {
            height += itemHeight / 2;
        }
        else {
            height += itemHeight;
        }
    }

    Rect bounds{ popup.position.x, popup.position.y, width + 6, height + 4 };

    popup.size = bounds.Size();

    if (m_openPopup != id) return false;

    if (m_state.mouseDown && !bounds.HasPoint(m_state.mousePos)) {
        m_openPopup = 0;
        return false;
    }

    int y = bounds.y + 2;
    for (size_t i = 0; i < numItems; i++) {
        auto item = items[i];

        if (item.find_first_not_of('-') == std::string::npos) {
            y += itemHeight / 2;
            continue;
        }

        Rect itemBounds{ bounds.x, y, bounds.width, itemHeight };
        auto wd = GetWidget(item + name, itemBounds, false);

        if (wd.state == WidgetState::Clicked) {
            m_openPopup = 0;
            selected = i;
            return true;
        }

        y += itemHeight;
    }

    return false;
}

void olcPGEX_TinyGUI::ShowPopup(const std::string& name, olc::vi2d position) {
	auto id = std::hash<std::string>()(name);
	auto&& pos = m_popups.find(id);
	if (pos == m_popups.end()) {
        return;
	}

	auto& popup = pos->second;
	popup.position = position;

    if (popup.position.x + popup.size.x > pge->ScreenWidth()) {
		popup.position.x = pge->ScreenWidth() - popup.size.x;
	}

    if (popup.position.y + popup.size.y > pge->ScreenHeight()) {
		popup.position.y = pge->ScreenHeight() - popup.size.y;
	}

	m_openPopup = id;
}

void olcPGEX_TinyGUI::ShowPopup(const std::string& name) {
    ShowPopup(name, m_state.lastClickedWidgetPosition);
}

bool olcPGEX_TinyGUI::OnBeforeUserUpdate(float& fElapsedTime) {
    m_state.mouseDown = pge->GetMouse(0).bHeld;
    m_state.mouseDelta = pge->GetMousePos() - m_state.mousePos;
	m_state.mousePos = pge->GetMousePos();

    m_rectStack.push_back(Rect{ 0, 0, pge->ScreenWidth(), pge->ScreenHeight() });
    if (!m_state.mouseDown) m_state.hoveredId = 0;

    for (auto&& [wid, widget] : m_widgets) {
        widget.pressed = false;
        widget.released = false;
	}

	return false;
}

void olcPGEX_TinyGUI::OnAfterUserUpdate(float fElapsedTime) {
    DrawPopups();
    if (!m_state.mouseDown) {
        m_state.activeId = 0;
    }
    m_rectStack.clear();

    m_blinkTimer += fElapsedTime;
    if (m_blinkTimer >= 0.4f) {
		m_blink = !m_blink;
		m_blinkTimer = 0.0f;
	}
}

void olcPGEX_TinyGUI::PushRect(Rect rect) {
	m_rectStack.push_back(rect);
}

Rect& olcPGEX_TinyGUI::PeekRect() {
	return m_rectStack.back();
}

void olcPGEX_TinyGUI::PopRect() {
	m_rectStack.pop_back();
}

Rect olcPGEX_TinyGUI::RectCutLeft(int amount) {
    Rect& orig = PeekRect();
    Rect rec{ orig.x, orig.y, amount, orig.height };
    orig.x += amount;
    orig.width -= amount;
    return rec;
}

Rect olcPGEX_TinyGUI::RectCutRight(int amount) {
	Rect& orig = PeekRect();
	Rect rec{ orig.x + orig.width - amount, orig.y, amount, orig.height };
	orig.width -= amount;
	return rec;
}

Rect olcPGEX_TinyGUI::RectCutTop(int amount) {
	Rect& orig = PeekRect();
	Rect rec{ orig.x, orig.y, orig.width, amount };
	orig.y += amount;
	orig.height -= amount;
	return rec;
}

Rect olcPGEX_TinyGUI::RectCutBottom(int amount) {
	Rect& orig = PeekRect();
	Rect rec{ orig.x, orig.y + orig.height - amount, orig.width, amount };
	orig.height -= amount;
	return rec;
}

olcPGEX_TinyGUI::Widget& olcPGEX_TinyGUI::GetWidget(const std::string& name, Rect bounds, bool blockInputByPopup) {
    auto id = std::hash<std::string>()(name);

    auto pos = m_widgets.find(id);
    if (pos == m_widgets.end()) {
        m_widgets[id] = Widget{};
        pos = m_widgets.find(id);
    }
    auto& widget = pos->second;
    widget.id = id;
    widget.state = WidgetState::Idle;

    bool blocked = blockInputByPopup && m_openPopup != 0;
    if (bounds.HasPoint(m_state.mousePos) && !blocked) {
        m_state.hoveredId = id;
        widget.state = WidgetState::Hovered;
        Log(name + " is [Hovered]");
        if (m_state.activeId == 0 && m_state.mouseDown) {
            m_state.activeId = id;
            m_state.focusedId = id;
            m_state.lastClickedWidgetPosition.x = bounds.x;
            m_state.lastClickedWidgetPosition.y = bounds.y + bounds.height;
            widget.state = WidgetState::Active;
            widget.pressed = true;
            Log(name + " is [Active]");
        }
    }
    else {
        if (m_state.focusedId == id && m_state.mouseDown) {
            m_state.focusedId = 0;
        }
    }

    if (m_state.activeId == id && m_state.hoveredId == id && !blocked) {
        widget.state = WidgetState::Active;
        Log(name + " is [Active]");
        if (!m_state.mouseDown) {
            widget.released = true;
            widget.state = WidgetState::Clicked;
            Log(name + " is [Clicked]");
        }
    }

    return widget;
}

olcPGEX_TinyGUI::Widget& olcPGEX_TinyGUI::GetWidgetByName(const std::string& name) {
    auto id = std::hash<std::string>()(name);
    auto pos = m_widgets.find(id);
    if (pos == m_widgets.end()) {
        m_widgets[id] = Widget{};
        pos = m_widgets.find(id);
    }
    return pos->second;
}

void olcPGEX_TinyGUI::DrawChar(int x, int y, char value, olc::Pixel color) {
    uint16_t index = (value - FONT_OFF) * ((FONT_X * FONT_Y) / 8);
    uint8_t byte = *(FONT + index);

    int16_t bit = 7;
    for (int cx = 0; cx < FONT_X; cx++) {
        for (int cy = 0; cy < FONT_Y; cy++) {
            if ((byte & (1 << bit)) != 0) {
                olc::PGEX::pge->Draw(x + cx, y + cy, color);
            }

            if (--bit < 0) {
                bit = 7;
                byte = *(FONT + (++index));
            }
        }
    }
}

void olcPGEX_TinyGUI::DrawText(int x, int y, const std::string& text, olc::Pixel color) {
    int tx = 0;
    int ty = 0;
    for (char c : text) {
        if (c == '\n') {
            tx = 0;
            ty += FONT_Y;
        }
        else {
            DrawChar(tx + x, ty + y, c, color);
            tx += FONT_X;
        }
    }
}

std::pair<int, int> olcPGEX_TinyGUI::TextSize(const std::string& text) {
    int ty = FONT_Y;
    int tx = 0;
    int maxX = -1;
    for (char c : text) {
        if (c == '\n') {
            ty += FONT_Y;
            maxX = std::max(maxX, tx);
            tx = 0;
        }
        else {
            tx += FONT_X;
        }
    }
    maxX = std::max(maxX, tx);
    return { maxX, ty };
}

olc::Pixel olcPGEX_TinyGUI::PixelBrightness(olc::Pixel color, float amount) {
    auto fnPerc = [amount](float value) {
        return std::clamp(value * amount, 0.0f, 1.0f);
    };

    const float r = float(color.r) / 255.0f;
    const float g = float(color.g) / 255.0f;
    const float b = float(color.b) / 255.0f;
    const float a = float(color.a) / 255.0f;
    return olc::PixelF(fnPerc(r), fnPerc(g), fnPerc(b), a);
}

void olcPGEX_TinyGUI::ThreeDFrame(Rect bounds, olc::Pixel color, WidgetState state) {
    bounds.width--;
    bounds.height--;

    const olc::Pixel darker = PixelBrightness(color, 0.25f);
    const olc::Pixel lighter = PixelBrightness(color, 1.75f);

    olc::Pixel tlColor = lighter;
    olc::Pixel brColor = darker;
    olc::Pixel cnColor = color;

    switch (state) {
        case WidgetState::Hovered:
            tlColor = PixelBrightness(tlColor, 1.25f);
            brColor = PixelBrightness(brColor, 1.25f);
            cnColor = PixelBrightness(cnColor, 1.25f);
            break;
        case WidgetState::Clicked:
        case WidgetState::Active:
            tlColor = PixelBrightness(darker, 0.5f);
            brColor = PixelBrightness(lighter, 0.75f);
            cnColor = darker;
            break;
        default: break;
    }

    auto innerBounds = bounds.Expand(-1);

    auto& pge = olc::PGEX::pge;

    pge->DrawLine(innerBounds.x, bounds.y, innerBounds.x + innerBounds.width, bounds.y, tlColor);
    pge->DrawLine(bounds.x, innerBounds.y, bounds.x, innerBounds.y + innerBounds.height, tlColor);

    pge->DrawLine(bounds.x + bounds.width, innerBounds.y, bounds.x + bounds.width, innerBounds.y + innerBounds.height, brColor);
    pge->DrawLine(innerBounds.x, bounds.y + bounds.height, innerBounds.x + innerBounds.width, bounds.y + bounds.height, brColor);

    innerBounds.width++;
    innerBounds.height++;
    pge->FillRect(
        olc::vi2d{ innerBounds.x, innerBounds.y },
        olc::vi2d{ innerBounds.width, innerBounds.height },
        cnColor
    );
}

void olcPGEX_TinyGUI::DrawPopups() {
    const int itemHeight = 11;

    auto& pge = olc::PGEX::pge;

    for (auto&& [wid, popup] : m_popups) {
        if (m_openPopup != wid) continue;

        int width = 0, height = 0;
        for (size_t i = 0; i < popup.items.size(); i++) {
            auto item = popup.items[i];
            width = std::max(width, TextSize(item).first);

            if (item.find_first_not_of('-') == std::string::npos) {
                height += itemHeight / 2;
            }
            else {
                height += itemHeight;
            }
        }

        Rect bounds{ popup.position.x, popup.position.y, width + 6, height + 4 };

        pge->FillRect(
            olc::vi2d{ bounds.x + 1, bounds.y + 1 },
            olc::vi2d{ bounds.width, bounds.height },
            PixelBrightness(baseColor, 0.7f)
        );
        pge->FillRect(
            olc::vi2d{ bounds.x, bounds.y },
            olc::vi2d{ bounds.width, bounds.height },
            PixelBrightness(baseColor, 1.3f)
        );

        const auto light = PixelBrightness(baseColor, 2.5f);
        const auto dark = PixelBrightness(baseColor, 0.15f);

        int y = bounds.y + 2;
        for (size_t i = 0; i < popup.items.size(); i++) {
            auto item = popup.items[i];

            if (item.find_first_not_of('-') == std::string::npos) {
                pge->DrawLine(
                    bounds.x, y + itemHeight / 4,
                    bounds.x + bounds.width, y + itemHeight / 4,
                    PixelBrightness(baseColor, 0.5f)
                );
                y += itemHeight / 2;
                continue;
            }

            auto wd = GetWidgetByName(item + popup.name);
            Rect wdBounds{ bounds.x, y, bounds.width, itemHeight };

            olc::Pixel fgColor = dark;
            switch (wd.state) {
                case WidgetState::Hovered:
                case WidgetState::Active:
                case WidgetState::Clicked:
                    fgColor = light;
                    pge->FillRect(
                        olc::vi2d{ wdBounds.x, wdBounds.y },
                        olc::vi2d{ wdBounds.width, wdBounds.height },
                        dark
                    );
                    break;
                default: break;
            }

            auto [tw, th] = TextSize(item);
            DrawText(bounds.x + 2, y + itemHeight / 2 - th / 2, item, fgColor);

            y += itemHeight;
        }
    }
}

float utils::Luma(const olc::Pixel& color) {
    float r = float(color.r) / 255.0f;
    float g = float(color.g) / 255.0f;
    float b = float(color.b) / 255.0f;
    return r * 0.299f + g * 0.587f + b * 0.114f;
}

#endif

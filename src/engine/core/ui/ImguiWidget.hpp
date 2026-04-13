#pragma once

struct ImguiWidget {
    virtual ~ImguiWidget() = default;
    virtual void draw() const = 0;
};

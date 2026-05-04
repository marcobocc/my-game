#pragma once

struct ImguiWidget {
    virtual ~ImguiWidget() = default;
    virtual void draw() = 0;
};

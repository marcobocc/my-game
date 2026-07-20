# Lua Scripting API Reference

## Script Lifecycle

A script is a Lua file that returns a table. The engine instantiates it once per entity it is attached to.

```lua
local MyScript = {}
return MyScript
```

### Callbacks

#### `MyScript:onStart()`
Called once when the entity is initialised. Use for setup.

#### `MyScript:onUpdate(dt: number)`
Called every frame. `dt` is elapsed time in seconds since the last frame.

#### `MyScript:onCollision(other: Entity)`
Called when this entity's collider overlaps another entity. `other` is the entity that was hit.

---

## Properties

A global `Properties` table declares inspector-editable values. The engine injects them into `self` before `onStart`.

```lua
Properties = {
    { name = "speed",     type = "float",  default = 5.0 },
    { name = "label",     type = "string", default = "Player" },
    { name = "active",    type = "bool",   default = true },
    { name = "count",     type = "int",    default = 0 },
    { name = "offset",    type = "vec3",   default = { 0.0, 1.0, 0.0 } },
    { name = "tint",      type = "color",  default = { 1.0, 1.0, 1.0, 1.0 } },
    { name = "target",    type = "entity" },
}
```

| Type     | Lua type                             |
|----------|--------------------------------------|
| `float`  | number                               |
| `int`    | number (integer)                     |
| `bool`   | boolean                              |
| `string` | string                               |
| `vec3`   | Vec3                                 |
| `color`  | table `{ r, g, b, a }` (0.0 – 1.0)  |
| `entity` | Entity                               |

---

## Vec3

Three-component vector.

**Constructor**

```lua
Vec3(x: number, y: number, z: number)
```

**Fields**

| Field | Type   |
|-------|--------|
| `x`   | number |
| `y`   | number |
| `z`   | number |

**Methods**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `v:length()` | number | Magnitude |
| `v:normalize()` | Vec3 | Unit vector copy |
| `v:dot(other: Vec3)` | number | Dot product |
| `v:cross(other: Vec3)` | Vec3 | Cross product |

**Operators:** `+`, `-`, `*` (scalar)

---

## Quat

Quaternion for representing rotations.

**Constructor**

```lua
Quat(x: number, y: number, z: number, w: number)
```

**Fields:** `x`, `y`, `z`, `w` (number)

**Operators**

| Expression | Result | Description |
|------------|--------|-------------|
| `q1 * q2` | Quat | Compose rotations |
| `q * v` | Vec3 | Rotate a vector |

**Global function**

```lua
quatAxisAngle(axis: Vec3, degrees: number) → Quat
```

Creates a quaternion from an axis and angle in degrees.

---

## Transform

Returned by value from `entity:getTransform()`. Modify then write back with `entity:setTransform()`.

**Fields**

| Field      | Type | Description   |
|------------|------|---------------|
| `position` | Vec3 | Local position |
| `rotation` | Quat | Local rotation |
| `scale`    | Vec3 | Local scale    |

**Methods**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `t:getForward()` | Vec3 | World-space forward direction |
| `t:getRight()` | Vec3 | World-space right direction |
| `t:getUp()` | Vec3 | World-space up direction |

---

## Entity

The entity this script is attached to is available as `self.entity`. Entities returned from world queries or passed to callbacks share the same API.

**Validation**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `e:isValid()` | bool | False if the entity has been destroyed |

**Transform**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `e:getTransform()` | Transform \| nil | Returns a copy of the transform |
| `e:setTransform(t: Transform)` | — | Writes the transform back |

**Hierarchy**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `e:setParent(parent: Entity)` | — | Attaches this entity to a parent |
| `e:clearParent()` | — | Detaches from parent |

**Components**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `e:getAnimator()` | Animator \| nil | Returns the Animator component |
| `e:getTextComponent()` | TextComponent \| nil | Returns a copy of the TextComponent |
| `e:setTextComponent(tc: TextComponent)` | — | Writes the TextComponent back |
| `e:addTextComponent(tc: TextComponent)` | — | Adds a TextComponent if none exists |
| `e:removeTextComponent()` | — | Removes the TextComponent |

**Physics**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `e:resolveCollisions()` | Vec3 | Returns the minimum translation vector to push this entity out of all overlapping colliders (box and capsule). Excludes this entity and its children. Returns zero vector when no overlaps. |

**Scripting**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `e:getScript(name: string)` | table \| nil | Returns the named script instance on this entity |

**Lifecycle**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `e:destroy()` | — | Destroys the entity immediately |

---

## Animator

Returned by `entity:getAnimator()`.

**Fields**

| Field          | Type   | Description |
|----------------|--------|-------------|
| `currentState` | string | Name of the currently playing clip |
| `currentTime`  | number | Playback position in seconds |
| `playing`      | bool   | Whether playback is active |
| `clipNames`    | table  | Array of available clip name strings |

**Methods**

| Signature | Description |
|-----------|-------------|
| `a:play(clipName: string)` | Start playing a clip |
| `a:pause()` | Pause playback |
| `a:stop()` | Stop and reset to start |
| `a:seek(time: number)` | Jump to a position in seconds |

---

## TextComponent

**Constructor**

```lua
TextComponent()
```

**Fields**

| Field       | Type   | Description |
|-------------|--------|-------------|
| `text`      | string | Text to render |
| `fontName`  | string | Asset name of the font |
| `fontSize`  | number | Size in pixels |
| `visible`   | bool   | Whether rendered |
| `billboard` | bool   | Rotate to face camera |
| `r`, `g`, `b`, `a` | number | Color (0.0 – 1.0) |
| `alignment` | number | `TextAlign.Left`, `TextAlign.Center`, or `TextAlign.Right` |

---

## World

Global object available as `World`.

| Signature | Returns | Description |
|-----------|---------|-------------|
| `World:createEntity()` | Entity | Creates a new entity with a default Transform |
| `World:findByName(name: string)` | Entity | Returns the first entity with the given name tag |

---

## Input

Global object available as `Input`.

**Keyboard**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `Input:isKeyDown(key: string)` | bool | True while the key is held |
| `Input:isKeyPressed(key: string)` | bool | True on the first frame the key is pressed |

Supported key names: `A`–`Z`, `0`–`9`, `SPACE`, `ENTER`, `ESCAPE`, `TAB`, `BACKSPACE`, `LEFT`, `RIGHT`, `UP`, `DOWN`, `LSHIFT`, `RSHIFT`, `LCTRL`, `RCTRL`, `LALT`, `RALT`, `F1`–`F12`

**Mouse**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `Input:isMouseButtonDown(btn: string)` | bool | True while the button is held. Buttons: `"LEFT"`, `"RIGHT"`, `"MIDDLE"` |
| `Input:getMouseDelta()` | number, number | `dx, dy` movement in pixels since last frame |
| `Input:getMousePosition()` | number, number | `x, y` cursor position in screen pixels |
| `Input:getScrollDelta()` | number | Scroll wheel delta |
| `Input:lockMouse()` | — | Locks cursor to window centre |
| `Input:unlockMouse()` | — | Releases cursor |

---

## UI

Global object available as `UI`. Builds a screen-space UI canvas (logical scene-viewport pixels, origin top-left). Input is hit-tested and consumed by the UI before reaching world/game scripts — e.g. `Input:isMouseButtonDown` for world interactions won't fire while the cursor is over a UI window.

The API is declarative/write-only: widgets are created with an `args` table and mutated through setter methods; there are no position/size getters, and individual buttons/labels/images can't be removed on their own — destroy the whole window instead.

| Signature | Returns | Description |
|-----------|---------|-------------|
| `UI:createWindow(args: table)` | UIWindow | Creates a window |
| `UI:clear()` | — | Destroys all windows |

**Common `args` fields** (shared by `createWindow`, `addButton`, `addLabel`, `addImage`)

| Field    | Type   | Default    | Description |
|----------|--------|------------|-------------|
| `x`      | number | `0`        | Offset X |
| `y`      | number | `0`        | Offset Y |
| `w`      | number | widget-specific | Width |
| `h`      | number | widget-specific | Height |
| `anchor` | string | `"TopLeft"` | One of `"TopLeft"`, `"Top"`, `"TopRight"`, `"Left"`, `"Center"`, `"Right"`, `"BottomLeft"`, `"Bottom"`, `"BottomRight"` |
| `visible`| bool   | `true`     | Initial visibility |

---

### UIWindow

Returned by `UI:createWindow(args)`.

**`createWindow` args** (in addition to the common fields above)

| Field           | Type   | Default | Description |
|-----------------|--------|---------|-------------|
| `title`         | string | `""`    | Title bar text; empty means no title bar |
| `w`             | number | `300`   | Width |
| `h`             | number | `200`   | Height |
| `bg`            | string | `""`    | Background texture path; empty means a flat colored quad |
| `border`        | number | `0`     | 9-slice inset in pixels |
| `bgColor`       | table  | `{0.10, 0.10, 0.12, 0.95}` | `{r, g, b, a}` background color |
| `titleBarColor` | table  | `{0.05, 0.05, 0.06, 1.0}`  | `{r, g, b, a}` title bar color |
| `draggable`     | bool   | `false` | Whether the window can be dragged by its title bar |

**Methods**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `window:addButton(args: table)` | UIButton | Adds a button (see UIButton args) |
| `window:addLabel(args: table)` | UILabel | Adds a label (see UILabel args) |
| `window:addImage(args: table)` | UIImage | Adds an image (see UIImage args) |
| `window:show()` | — | Makes the window visible |
| `window:hide()` | — | Hides the window |
| `window:isVisible()` | bool | Whether the window is currently visible |
| `window:setTitle(title: string)` | — | Updates the title bar text |
| `window:destroy()` | — | Removes the window (and all its widgets) |

---

### UIButton

Returned by `window:addButton(args)`.

**`addButton` args** (in addition to the common fields)

| Field          | Type     | Default | Description |
|----------------|----------|---------|-------------|
| `w`            | number   | `120`   | Width |
| `h`            | number   | `32`    | Height |
| `text`         | string   | `""`    | Button label |
| `font`         | string   | `""`    | Font asset name; empty means engine default |
| `fontSize`     | number   | `16`    | Font size in pixels |
| `texture`      | string   | `""`    | Texture path; empty means a flat colored quad |
| `color`        | table    | `{0.25, 0.25, 0.28, 1}` | `{r, g, b, a}` idle color |
| `hoverColor`   | table    | `{0.35, 0.35, 0.40, 1}` | `{r, g, b, a}` hover color |
| `pressedColor` | table    | `{0.15, 0.15, 0.18, 1}` | `{r, g, b, a}` pressed color |
| `textColor`    | table    | `{1, 1, 1, 1}` | `{r, g, b, a}` label color |
| `onClick`      | function | —       | Optional; equivalent to calling `button:onClick(fn)` after creation |

**Methods**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `button:onClick(fn: function())` | — | Registers a click callback. Errors thrown inside `fn` are caught and logged, not propagated. |
| `button:setText(text: string)` | — | Updates the label text |
| `button:setVisible(visible: bool)` | — | Shows or hides the button |

---

### UILabel

Returned by `window:addLabel(args)`.

**`addLabel` args** (in addition to the common fields)

| Field      | Type   | Default | Description |
|------------|--------|---------|-------------|
| `text`     | string | `""`    | Label text |
| `font`     | string | `""`    | Font asset name; empty means engine default |
| `fontSize` | number | `16`    | Font size in pixels |
| `color`    | table  | `{1, 1, 1, 1}` | `{r, g, b, a}` text color |
| `align`    | string | `"Left"` | One of `"Left"`, `"Center"`, `"Right"` |

**Methods**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `label:setText(text: string)` | — | Updates the text |
| `label:setColor(r: number, g: number, b: number, a: number)` | — | Updates the text color |
| `label:setVisible(visible: bool)` | — | Shows or hides the label |

---

### UIImage

Returned by `window:addImage(args)`.

**`addImage` args** (in addition to the common fields)

| Field     | Type   | Default | Description |
|-----------|--------|---------|-------------|
| `texture` | string | `""`    | Texture path |
| `tint`    | table  | `{1, 1, 1, 1}` | `{r, g, b, a}` tint applied to the texture |

**Methods**

| Signature | Returns | Description |
|-----------|---------|-------------|
| `image:setTexture(texture: string)` | — | Changes the displayed texture |
| `image:setVisible(visible: bool)` | — | Shows or hides the image |

---

### Example

```lua
local MyScript = {}

function MyScript:onStart()
    self.window = UI:createWindow{
        title = "Inventory",
        x = 20, y = 20, w = 240, h = 160,
        anchor = "TopLeft",
        draggable = true,
        visible = false,
    }

    self.countLabel = self.window:addLabel{
        text = "Clicks: 0",
        x = 10, y = 10,
    }

    local clicks = 0
    self.window:addButton{
        text = "Click me",
        x = 10, y = 40,
        onClick = function()
            clicks = clicks + 1
            self.countLabel:setText("Clicks: " .. clicks)
        end,
    }
end

function MyScript:onUpdate(dt)
    if Input:isKeyPressed("I") then
        if self.window:isVisible() then
            self.window:hide()
        else
            self.window:show()
        end
    end
end

return MyScript
```

---

## Debug

Global object available as `Debug`. All shapes are drawn for one frame only.

| Signature | Description |
|-----------|-------------|
| `Debug:line(from: Vec3, to: Vec3 [, color: Vec3])` | Draws a line segment |
| `Debug:box(center: Vec3, halfExtents: Vec3 [, color: Vec3])` | Draws a box wireframe |
| `Debug:sphere(center: Vec3, radius: number [, color: Vec3])` | Draws a sphere wireframe |

`color` is a Vec3 in RGB (0.0 – 1.0). Defaults to green for lines, cyan for boxes, yellow for spheres.

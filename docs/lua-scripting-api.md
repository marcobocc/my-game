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

## Debug

Global object available as `Debug`. All shapes are drawn for one frame only.

| Signature | Description |
|-----------|-------------|
| `Debug:line(from: Vec3, to: Vec3 [, color: Vec3])` | Draws a line segment |
| `Debug:box(center: Vec3, halfExtents: Vec3 [, color: Vec3])` | Draws a box wireframe |
| `Debug:sphere(center: Vec3, radius: number [, color: Vec3])` | Draws a sphere wireframe |

`color` is a Vec3 in RGB (0.0 – 1.0). Defaults to green for lines, cyan for boxes, yellow for spheres.

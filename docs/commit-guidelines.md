# Commit Guidelines

## Commit Message Header

```
<type>(<scope>): <short summary>
│       │             │
│       │             └─⫸ Summary in present tense. Not capitalized. No period at the end.
│       │
│       └─⫸ Scope: core|rendering|tools
│
└─⫸ Commit Type: feat|fix|refactor|style|docs|tests|build|revert
```

### Type

The type communicates the **main purpose** of the change. Must be one of the following:

| Type     | Description                                                                                                 |
|----------|-------------------------------------------------------------------------------------------------------------|
| feat     | Introduces a new feature or changes behavior                                                                |
| fix      | Patches a bug                                                                                               |
| refactor | Restructures code for organization, maintainability or readability without changing behavior or fixing bugs |
| format   | Affects only the formatting of the code itself (whitespace, indentation). No other change must be present   |
| docs     | Affects only documentation or comments in the source code                                                   |
| tests    | Adds missing tests or corrects existing tests, without changing behaviour of the source code itself         |
| build    | Affects the build system, dependencies and tooling                                                          |
| revert   | Undoes a previous commit                                                                                    |

### Scope

The scope is required for `feat`, `fix` and `refactor` and communicates the **main subsystem** affected. Must be one of the following:

| Scope     | Description                                                                                |
|-----------|--------------------------------------------------------------------------------------------|
| core      | Changes to the game engine's core logic, such as the entity-component system, input system |
| assets    | Changes involving the assets management system, asset definitions and asset loading        |
| rendering | Changes involving the rendering system, Vulkan, and other graphics-related code            |
| tools     | Changes to editors or other game development tools                                         |
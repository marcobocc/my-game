# Commit Guidelines

## Commit Message Header

```
<type>(<scope>): <short summary>
│       │             │
│       │             └─⫸ Summary in present tense. Not capitalized. No period at the end.
│       │
│       └─⫸ Game subsystem affected: core|rendering
│
└─⫸ Commit Type: feat|fix|refactor|style|docs|tests|build|revert
```

### Type

The type communicates the **main purpose** of the change. Must be one of the following:

| Type     | Description                                                                                                                     |
|----------|---------------------------------------------------------------------------------------------------------------------------------|
| feat     | Introduces a new feature or changes behavior                                                                                    |
| fix      | Patches a bug                                                                                                                   |
| refactor | Architectural or structural changes to source code that neither fix a bug nor change behavior                                   |
| style    | Only formatting, whitespace, variable renaming, and other changes to source code with the intent of improving human readability |
| docs     | Only changes documentation or comments                                                                                          |
| tests    | Only adds missing tests or corrects tests, without changing behavior                                                            |
| build    | Only changes to the build system and tooling                                                                                    |
| revert   | Undoes a previous commit                                                                                                        |

### Scope

The scope is optional and communicates the **main game subsystem** affected. Must be one of the following:

| Scope     | Description                                                                                                       |
|-----------|-------------------------------------------------------------------------------------------------------------------|
| core      | Changes to the game engine's core logic, such as the entity-component system, event system or resource management |
| rendering | Changes involving Vulkan, the rendering pipeline, shaders, or other graphics-related code                         |
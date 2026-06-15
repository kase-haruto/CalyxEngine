# Calyx Project Workflow

## Repository Roles

CalyxEngine is developed as the engine repository. It owns the runtime library,
editor executable, project browser, tools, shaders, and shared engine assets.

Each game should be developed in its own repository. A game repository owns its
game source, game assets, scenes, and one `.calyxproj` file. Game teams should
not edit engine source directly inside the game repository.

## Recommended User Flow

1. Install or clone a known engine version.
2. Launch `CalyxEditor.exe`.
3. Create or open a `.calyxproj` from the project browser.
4. Commit the game repository, including the `.calyxproj`, `Game`, and
   project-local `Resources` files.
5. Share the game repository with team members.
6. Team members use the same engine version and open the shared `.calyxproj`.
7. For C++ debugging, open the generated solution, set `<GameName>.Editor`
   as the startup project, and press F5. The launcher project builds the game
   DLL for the selected Visual Studio configuration and starts the editor with
   that configuration.

## Sharing Game Projects

A game project should be shared as a normal Git repository. The repository
should contain project-owned files only:

- `.calyxproj`
- `Game/`
- `Resources/Assets/`
- project-specific config files

The game repository should not contain a copied engine directory. Engine updates
should happen by changing the engine version used by the team, not by manually
copying engine files between game projects.

## Engine Version Policy

For early development, use one of these two policies:

- Local engine checkout: every developer keeps `CalyxEngine` cloned next to the
  game project and opens the game from the editor.
- Git submodule: the game repository pins `CalyxEngine` as a submodule when the
  team needs reproducible engine revisions.

The submodule approach is better once multiple people must build the same game
project reliably. The local checkout approach is simpler while the engine API is
still changing quickly.

## Controlled Engine Updates

Game projects should pin the required engine release in `.calyxproj`:

```json
{
  "engineVersion": "v1.0.1"
}
```

Do not auto-upgrade every user to the newest GitHub Release. The lead should
test a new engine release on an upgrade branch, update `engineVersion`, verify
the project, and then merge that change. Team members then pull the game
repository and the launcher resolves the pinned engine version.

The GitHub Release package is expected to publish `CalyxEngine-<version>.zip`
and `latest.json`. `latest.json` is useful for discovery, but the project file's
`engineVersion` remains the source of truth for team production.

## Future Packaging Direction

The long-term goal is for game repositories to depend on a packaged engine SDK:

- `CalyxEditor.exe`
- `CalyxEngineLib.lib`
- public headers under `CalyxEngine/`
- required DLLs and shader/tool resources

At that point, game developers will not need the full engine source unless they
are also engine contributors.

# Calyx Engine Submodule Workflow

## Goal

Small teams should be able to inspect and change engine code while developing a
game, without merging the engine repository and the game repository into one
ownership boundary. Use this structure:

```text
GameRepository/
  CalyxEngine/        # git submodule
  Game/
  Resources/
  project/
    Game.sln
    Game.vcxproj
```

The game repository owns game code, game assets, and its solution. The engine
repository owns `CalyxEngineLib.vcxproj`, editor/runtime code, shared tools,
third-party source, and release packaging.

## Repository Setup

Create the game repository first, then add the engine as a submodule:

```powershell
git submodule add <engine-repository-url> CalyxEngine
git submodule update --init --recursive
```

Clone workflow for team members:

```powershell
git clone --recurse-submodules <game-repository-url>
```

If the repository was cloned without submodules:

```powershell
git submodule update --init --recursive
```

## Visual Studio Layout

The game solution should reference the engine project inside the submodule, not
a packaged `.lib` during source-based development.

Recommended paths from `GameRepository/project/Game.vcxproj`:

```xml
<Import Project="..\CalyxEngine\project\CalyxEngineReference.props" />
```

Then use the properties from the imported file:

```xml
<AdditionalIncludeDirectories>$(ProjectDir)..\;$(CalyxEngineIncludeDirs);%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
<ProjectReference Include="$(CalyxEngineLibProject)">
  <Project>{a12f6624-6c62-41e3-9cb7-02e3f5dfbb7e}</Project>
  <LinkLibraryDependencies>true</LinkLibraryDependencies>
</ProjectReference>
<ProjectReference Include="$(CalyxDirectXTexProject)">
  <Project>{371b9fa9-4c90-4ac6-a123-aced756d6c77}</Project>
  <LinkLibraryDependencies>true</LinkLibraryDependencies>
</ProjectReference>
```

The `.sln` file still needs normal relative project entries because Visual
Studio solution files do not use MSBuild properties for project paths:

```text
Project(...) = "CalyxEngineLib", "..\CalyxEngine\project\CalyxEngineLib.vcxproj", "{A12F6624-6C62-41E3-9CB7-02E3F5DFBB7E}"
Project(...) = "DirectXTex", "..\CalyxEngine\project\externals\DirectXTex\DirectXTex_Desktop_2022_Win10.vcxproj", "{371B9FA9-4C90-4AC6-A123-ACED756D6C77}"
```

## Reflection Generation

Game projects should run the reflection generator from the submodule and write
game-generated files into the game repository:

```xml
<Command>powershell -NoProfile -ExecutionPolicy Bypass -File "$(CalyxReflectionTool)" -Root "$(ProjectDir)..\" -ScanRoots &quot;Game&quot; -OutputDir &quot;generated\Foundation\Reflection&quot; -OutputName CalyxGameObjectRegistry.generated -FunctionName RegisterGeneratedGameSceneObjects</Command>
```

`-Root` should point at the game repository root, not the engine submodule. This
keeps generated game registry files in the game repository while still using the
reflection tool from the engine submodule.

## Daily Workflow

Update engine revision used by the game:

```powershell
cd CalyxEngine
git fetch
git checkout <branch-or-commit>
cd ..
git add CalyxEngine
git commit -m "Update CalyxEngine submodule"
```

Make an engine change while working on a game:

```powershell
cd CalyxEngine
git checkout -b fix/runtime-issue
# edit, build, test
git commit -am "Fix runtime issue"
git push -u origin fix/runtime-issue
cd ..
git add CalyxEngine
git commit -m "Point game at runtime fix"
```

The game commit records only the engine commit pointer. The engine commit itself
must be pushed to the engine repository.

## Policy

- Keep engine source changes in the engine submodule.
- Keep game source, scenes, and project assets in the game repository.
- Do not commit generated build output from either repository.
- Prefer submodule commits for active development and packaged SDK references
  only for stable releases.

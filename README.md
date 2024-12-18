# Advanced Rendering of Line Data
## Information
This project is about advanced rendering of line data with the use of ambient
occlusion and transparency. It is written in C++ with DirectX11 + HLSL.

The Project was implemented by Elias Erhardt and Thomas Seirlehner for the Visualization 2 course of the TU Vienna.
## Download

## Setup
Our application uses a standard DirectX11 enviroment. We use Visual Studio 2022 with the Windows 10 SDK.
To install our project:
```
git clone https://github.com/EliasErhardt7/Tube11Vis2.git
```
If the cloning worked:
```
cd Tube11Vis2
start Tube11Vis2.sln
```
After that, start in Debug or Release. There should be an error that DirectXTK.lib is missing.
To fix that go to Project->manage NuGet-packages->installed: There should be directxtk_desktop_win10 installed.
Uninstall this package and then go to Search and search for "directx tool kit". Then look for the same directxtk_desktop_win10 and install it again. This should fix the error.

## Controls
To move around in the scene use W,A,S,D. To go up and down use Q,E. To look around hold down the right Mousebutton and move.

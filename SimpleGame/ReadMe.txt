Orbital Station / Double Ring Prototype

Current entry point: SimpleGame.cpp -> StationScene
Source: StationLayout.h / StationMission.h / StationScene.h

Controls
WASD move, Shift sprint, mouse look
E interact; hold E when prompted
Tab hold + mouse direction + Tab release: equip tool
Left mouse: use equipped tool
M map, V vent layer, wheel or +/- zoom, drag pan, C recenter
J station log, H help, ESC mouse capture/release
R restart after escape or health reaches zero
[ and ] adjust health for UI preview

Begin in LIFE SUPPORT and follow the current objective.
The map marks the next objective in gold. The scanner finds nearby actionable stations.

This is a local solo implementation of the draft's map and proposed progression.
Multiplayer networking and enemy AI are not implemented.
Build and execution checks were intentionally left to the user.
See ../우주정거장_재구성_구현안내_v0.2.txt for the full Korean guide.

캐비넷 정면에서 E: 들어가 숨기 / 내부에서 E: 나오기.
냉각 회로 수리: 드릴 또는 렌치를 장착하고 좌클릭 유지.

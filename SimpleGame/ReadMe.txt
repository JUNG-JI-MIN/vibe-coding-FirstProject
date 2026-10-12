우주정거장 - 구조 신호

구조: SimpleGame.cpp -> Framework -> Scene -> Object -> Mesh
Engine, AI, Game 및 렌더링 보조 코드: 헤더 선언 / 같은 이름의 CPP 구현
공유 메시: Engine/ResourceManager의 unordered_map 캐시
기준 맵: Res/Station_Early_Model.obj
AI: BehaviorTree, Blackboard, StationAgents

시작: 타이틀 -> 시작 -> 탐사 준비 -> 난이도·색 선택 -> 탐사 시작
플레이어 1명(0번 슬롯) + 동료 NPC 2명(1·2번 슬롯)으로 고정합니다.
두 NPC는 각자의 행동 트리로 추종·회피·협동 작업을 수행합니다.
방 참가·생성·검색과 네트워크 기능은 제거했습니다.

WASD 이동 / 마우스 시점 / Shift 달리기
E 누르기 또는 유지: 상호작용, 작업, 캐비넷 숨기·나오기
Tab 유지 + 마우스 선택 + Tab 해제: 도구 장착 / 중앙은 맨손
좌클릭: 손전등·스캐너·전압봉·구급상자 사용
M 지도 / 휠 확대·축소 / 드래그 또는 WASD 이동 / C 지도 중심
J 기록 / Esc 설정 및 일시정지 / V 에일리언·동료 AI 상태 표시
관제실 F CCTV / CCTV Q·Z 카메라 / N 격벽 선택 / B 격벽 개폐
최종 압력 조절: Q 증가 / Z 감소 / 50~54에서 E 확정
결과 화면 R: 플레이어·NPC·시나리오 재시작 / Esc: 타이틀

GLSL은 Shaders에서 관리합니다. 빌드 후 Shaders와 Res가 실행 파일 옆으로 복사됩니다.
배포 시 플랫폼에 맞는 GLEW/freeglut DLL과 해당 폴더를 포함하세요.

자세한 흐름: ../재구성_구현안내.md
행동 트리: ../AI_행동트리_설계.md
사용자 요청에 따라 게임 빌드와 실행 확인은 수행하지 않았습니다.

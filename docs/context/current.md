# 현재 공통 기준

갱신일: 2026-10-14 (Asia/Seoul). 최초 코드 확인 기준: `07f5b2c`.
이 문서는 현재 상태만 유지한다. 과거 변경은 Git과 짧은 인계 기록에서 찾는다.

## 확정된 현재 범위
- Windows / Visual Studio 2022 C++ / OpenGL / GLEW / freeglut 프로토타입.
- 플레이어 1명 + 독립적인 동료 NPC 2명. 네트워크·방 참가/생성은 현재 실행 범위에서 제거됨.
- 11장 시나리오: 정거장 탐사 → 전력 복구 → 은신 → 샘플/카드 교환 → 제한 시간 내 탈출.
- OBJ 정거장, 자동문·격벽·캐비넷, CCTV·지도·기록·도구 UI.
- 에일리언은 시야·마지막 목격·소음·목격한 캐비넷·지정 이벤트를 사용한다.
- 동료 NPC는 추종·회피·문 밀기·밸브 지원을 수행한다.
- 구현 사실과 수치의 근거는 관련 코드다. 모든 동작의 플레이 성공을 보증하는 목록은 아니다.

## 코드 경계
- 시작: `SimpleGame/SimpleGame.cpp → Game/SceneFactories.cpp`.
- Engine: Framework, Scene, Object, 공유 Mesh와 ResourceManager, 수학.
- AI: BehaviorTree, Blackboard, StationAgents.
- Game: GameState/GameSession, Simulation, StationWorld, PlayScene, 메뉴·UI·표현.
- 그래픽: Mesh/Object, MetalMaterial, PostProcessing, ShaderProgram, KoreanText, GLSL.
- 구형 Station/Ship 소스는 보관용. 새 기능의 기본 수정 대상은 Engine/AI/Game이다.
- CPP 추가 시 `.vcxproj`와 `.vcxproj.filters`에 함께 등록한다.

## 문서의 지위
- `우주정거장_맵기획서_준최종.txt`, `UI 기획안.xlsx`, OBJ: 재구성의 기준 자료.
- `재구성_구현안내.md`, `AI_행동트리_설계.md`: 2026-10-12 구현 설명과 당시 확인 범위.
- `우주정거장_클라이언트_확장기획_v0.3.md`: 제안. 확정 요구사항이나 구현 완료가 아님.
- 확장 기획의 v0.2 참조는 과거 기준이다. 그 문서를 현재 구현 안내로 자동 치환하지 않는다.
- 과거 안내의 “Commit/Push 하지 않았다” 등은 당시 작업 기록이며 현재 Git 상태의 근거가 아니다.
- 현재 결정 이유는 `docs/decisions/`, 작업 결과는 `docs/handoffs/`, 담당 파일은 `modules.json`.

## 검증과 한계
- 이번 초기화에서는 주요 문서·진입점·타입·의존성을 소스 수준에서 확인했다.
- 게임 빌드·실행·전체 플레이 검증은 이번 초기화에서 수행하지 않았다.
- 기본 인물/설비 표현, 단순 소음 감쇠, 고정 CCTV, 지상 A* 이동 단계다.
- 환풍구 이동, 정교한 공간 음향, 인물 애니메이션 등은 후속 범위다.

## 후속 DirectX 이식
- 사용자 방향: 후속 DirectX 이식. API 버전·작업 착수 시점·상세 이식 계획은 미정.
- 시나리오·AI·상호작용 규칙을 이식할 설계 자산으로 유지한다.
- `StationWorld.h → Engine/Object.h`, `StationWorld(const Mesh&)` 등 그래픽 계층 의존성이 남아 있다.
- 렌더러 교체만으로 이식 완료된다고 가정하지 않는다. 좌표·행렬·리소스·입력 경계도 작업 시 확인한다.

## 두 PC 작업
- 코드·설계·인계는 같은 Git 저장소로 공유한다. 동시 작업은 작업별 브랜치 사용.
- PC별 확인 위치는 Git에서 제외된 `.local/codex-context.json`에 저장한다.
- 시작 시 `tools/read-context.ps1`; 확인 완료 후에만 `-MarkReviewed -ExpectedHead <전체 SHA>`.
- 설정 파일도 코드와 함께 Commit/Push해야 다른 PC가 같은 기준으로 작업할 수 있다.
- 매 커밋은 요청 프롬프트·변경 내용·검증을 정리한 본문을 포함하고, 인계에도 요청 요약을 남긴다.

# 코드 작성 규칙

`SimpleGame`의 직접 작성한 C++와 GLSL은 루트의 `.clang-format`을 기준으로 관리합니다.
외부 라이브러리인 `Dependencies`와 생성된 빌드 파일에는 적용하지 않습니다.

- 들여쓰기는 공백 4칸을 사용합니다.
- 함수, 클래스, 조건문, 반복문의 중괄호는 다음 줄에 놓습니다(Allman).
- 한 줄짜리 조건문과 반복문도 중괄호와 줄바꿈을 사용합니다.
- 여러 실행문을 한 줄에 붙이지 않습니다.
- 함수 정의 사이에는 빈 줄을 넣고, 함수 안에서는 작업 단위 사이에 빈 줄을 넣습니다.
- 줄 길이는 110자를 기준으로 합니다.
- include 순서는 자동 정렬하지 않습니다. `.cpp`의 미리 컴파일된 헤더 위치를 유지합니다.

Visual Studio에 포함된 clang-format 19.1.5로 정리했습니다.
포맷 적용 시 프로젝트 루트에서 원하는 파일만 지정하세요.

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\clang-format.exe' -i --style=file SimpleGame\StationScene.h
```

## 셰이더 관리

GLSL 코드는 C++ 문자열에 넣지 않고 `SimpleGame/Shaders`에 저장합니다.

| 파일 | 역할 |
| --- | --- |
| `MetalMaterial.vs`, `MetalMaterial.fs` | 금속 재질, 조명, 안개 |
| `PostProcessing.vs`, `PostProcessing.fs` | HDR 톤 매핑, 블룸, 비네팅 |
| `SolidRect.vs`, `SolidRect.fs` | 기존 사각형 렌더러 |

금속 재질과 후처리는 `ShaderProgram.h/.cpp`의 공통 로더를 사용합니다.
실행 파일 옆 `Shaders`를 먼저 찾고, 개발 환경에서는 작업 디렉터리의
`Shaders`, `SimpleGame/Shaders`를 차례로 확인합니다. 두 단계의 셰이더는 같은 폴더에서 읽습니다.
파일은 UTF-8로 저장하며, 로더는 UTF-8 BOM도 처리합니다.
읽기·컴파일·링크 실패는 콘솔에 보고하고 기존 렌더링 대체 경로로 돌아갑니다.

Visual Studio 빌드 후 모든 `.vs`, `.fs` 파일을 `$(OutDir)Shaders`로 복사합니다.
배포할 때 실행 파일과 함께 이 폴더도 포함하세요. 변경된 셰이더를 적용하려면
다시 빌드하여 복사한 뒤 게임을 재시작합니다. 실행 중 자동 재로딩은 하지 않습니다.

## 선언과 구현

현재 실행되는 Engine·AI·Game 및 렌더링 보조 코드는 `.h` 선언과 `.cpp` 구현을 분리합니다.
구조체의 필드·기본값, 열거형, 순수 가상 함수 및 default/delete 선언은 헤더에 둡니다.
생성자·소멸자와 일반 함수의 동작은 같은 이름의 CPP에 둡니다. 기본 인수는 헤더에만 적습니다.
CPP의 첫 include는 `stdafx.h`이며 하위 폴더에서도 같은 이름을 사용합니다.
프로젝트의 추가 포함 경로에 `$(ProjectDir)`을 등록하여 미리 컴파일된 헤더 설정과 일치시킵니다.
새 CPP는 `.vcxproj`와 `.vcxproj.filters`에도 등록합니다.
C++14의 정적 constexpr 멤버가 저장소를 필요로 하면 CPP에 한 번 정의합니다.
구형 Station/Ship 소스는 보관용이며 현재 실행 구조의 규칙 적용 범위와 구분합니다.

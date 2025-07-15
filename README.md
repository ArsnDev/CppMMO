# CppMMO: Modern C++ MMORPG Server

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/CMake-3.15+-green)
![Docker](https://img.shields.io/badge/Docker-Enabled-blue)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)

C++20, Boost.Asio, 그리고 FlatBuffers를 기반으로 제작된 고성능 최신 MMORPG 서버입니다.

## 🌟 주요 특징

*   **Modern C++20:** C++20 표준과 코루틴을 적극적으로 활용하여 가독성 높고 깔끔한 비동기 코드를 구현합니다.
*   **마이크로서비스 아키텍처:** 게임 서버와 인증 서버가 분리된 확장 가능한 구조로 설계되었습니다.
*   **비동기 네트워크:** `Boost.Asio`를 기반으로 확장성 높은 고성능 네트워크 I/O를 처리합니다.
*   **확장 가능한 스레딩 모델:** 네트워크 I/O와 게임 로직 처리를 위한 스레드 풀을 분리하여, 로직의 병목 현상이 네트워크 응답성에 영향을 주지 않도록 설계되었습니다.
*   **효율적인 직렬화:** `Google FlatBuffers`를 사용하여 네트워크 패킷을 Zero-copy로 직렬화함으로써 CPU 부하를 최소화합니다.
*   **Pub/Sub 채팅 시스템:** `Redis`와 연동하여 여러 서버 인스턴스를 지원할 수 있는 확장성 있는 채팅 서비스를 제공합니다.
*   **Docker 기반 배포:** 완전한 Docker 컨테이너화로 개발과 배포가 용이합니다.
*   **크로스 플랫폼 지원:** Unity 게임 클라이언트와 Python 테스트 도구를 통해 다양한 환경에서 접근 가능합니다.

## 🏗️ 전체 아키텍처

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Unity Client   │    │  Python Tests   │    │  Web Client     │
│  (별도 프로젝트)  │    │  (Test/)        │    │  (Future)       │
└─────────┬───────┘    └─────────┬───────┘    └─────────┬───────┘
          │                      │                      │
          └──────────────────────┼──────────────────────┘
                                 │
                    ┌─────────────▼─────────────┐
                    │    CppMMO Game Server     │
                    │    (C++20 / Boost.Asio)  │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────▼─────────────┐
                    │    Auth Server (.NET)     │
                    │    (REST API / JWT)       │
                    └─────────────┬─────────────┘
                                 │
          ┌──────────────────────┼──────────────────────┐
          │                      │                      │
    ┌─────▼─────┐        ┌─────▼─────┐        ┌─────▼─────┐
    │   Redis   │        │   MySQL   │        │   Logs    │
    │  (Cache)  │        │  (Users)  │        │  (Files)  │
    └───────────┘        └───────────┘        └───────────┘
```

## 🎮 게임 서버 아키텍처

```
+-----------+      +-----------------+      +------------------+
|  Clients  |<---->|   TcpServer     |<---->|  PacketManager   |
+-----------+      | (I/O 스레드)    |      +------------------+
                   +-----------------+               |
                                                     | (Job 푸시)
                                                     v
                                             +------------------+
                                             |    JobQueue      |
                                             +------------------+
                                                     ^
                                                     | (Job 팝)
                                                     |
                                             +------------------+
                                             |    JobProcessor  |
                                             | (패킷 분류 및   |
                                             |  게임 커맨드 푸시)|
                                             +--------+---------+
                                                      |
                                                      | (게임 커맨드 푸시)
                                                      v
                                             +------------------+
                                             | GameLogicQueue   |
                                             +------------------+
                                                      ^
                                                      | (게임 커맨드 팝)
                                                      |
+-----------+      +-----------------+      +------------------+
|   Redis   |<---->| RedisChatService|<---->|    GameManager   |
+-----------+      +-----------------+      | (게임 로직 처리) |
                                             +------------------+
```

### 핵심 컴포넌트

1.  **TcpServer (I/O 스레드):** 전용 스레드 풀을 사용하여 클라이언트 연결을 관리하고 원시 네트워크 I/O를 처리합니다.
2.  **PacketManager:** 클라이언트로부터 받은 데이터를 검증하고, `Job` 형태로 감싸 `JobQueue`에 푸시합니다.
3.  **JobQueue:** I/O 스레드와 로직 스레드를 분리하는 스레드 안전 큐(`moodycamel::ConcurrentQueue`)입니다.
4.  **JobProcessor (패킷 처리 스레드):** 별도의 스레드 풀을 사용하여 `JobQueue`에서 작업을 가져와 처리합니다. 일반 패킷은 `PacketHandler`를 통해 처리하고, 게임 로직과 관련된 커맨드는 `GameLogicQueue`에 푸시합니다.
5.  **GameLogicQueue:** `JobProcessor`로부터 받은 게임 커맨드를 `GameManager`가 처리할 수 있도록 전달하는 스레드 안전 큐입니다.
6.  **GameManager (게임 로직 스레드):** `GameLogicQueue`에서 게임 커맨드를 가져와 실제 게임 로직(플레이어 이동, HP 업데이트, 존 변경 등)을 처리합니다.
7.  **RedisChatService:** Redis 서버와 상호작용하여 채팅 시스템을 위한 Pub/Sub 기능을 수행합니다.

## 🛠️ 시작하기

### 요구 사항

*   C++20을 지원하는 컴파일러 (MSVC v143 이상 또는 GCC 10+)
*   CMake (3.15 버전 이상)
*   [vcpkg](https://github.com/microsoft/vcpkg) (의존성 관리)
*   Docker & Docker Compose (권장)
*   Redis 서버 (채팅 기능용)
*   MySQL 서버 (사용자 데이터 저장용)

### 의존성 라이브러리

아래 라이브러리들은 `vcpkg`와 `CMake FetchContent`를 통해 자동으로 관리됩니다.
*   Boost (System, Thread, Program_options, Filesystem)
*   spdlog (로깅)
*   redis-plus-plus (Redis 클라이언트)
*   FlatBuffers (직렬화)
*   concurrentqueue (스레드 안전 큐)
*   nlohmann-json (JSON 파싱)

### 🐳 Docker를 이용한 실행 (권장)

1.  **저장소 복제:**
    ```bash
    git clone <your-repo-url>
    cd CppMMO
    ```

2.  **Docker Compose로 전체 시스템 실행:**
    ```bash
    docker-compose up -d
    ```

3.  **서비스 상태 확인:**
    ```bash
    docker-compose ps
    ```

### 🔧 로컬 빌드

1.  **CMake 설정:**
    vcpkg toolchain 파일의 경로를 지정하여 CMake 프로젝트를 설정합니다.
    ```bash
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="<vcpkg-경로>/scripts/buildsystems/vcpkg.cmake"
    ```

2.  **프로젝트 빌드:**
    ```bash
    cmake --build build
    ```
    빌드가 완료되면 실행 파일은 `build/bin/` 디렉터리에 생성됩니다.

### 서버 실행

`build` 디렉터리에서 아래 명령어로 서버를 실행할 수 있습니다.

```bash
./bin/CppMMO [옵션]
```

#### 커맨드 라인 옵션

| 옵션              | 설명                           | 기본값  |
| ----------------- | ------------------------------ | ------- |
| `-p`, `--port`    | 서버가 리스닝할 포트 번호입니다. | `8080`  |
| `--io-threads`    | 네트워크 I/O 스레드 개수입니다.  | `2`     |
| `--logic-threads` | 게임 로직 처리 스레드 개수입니다.| `4`     |
| `-h`, `--help`    | 도움말 메시지를 출력합니다.      |         |

**실행 예시:**
```bash
./bin/CppMMO --port 9000 --io-threads 4 --logic-threads 8
```

## 🔐 인증 서버 (.NET)

인증 서버는 ASP.NET Core 8을 기반으로 구축되었으며, 다음과 같은 기능을 제공합니다:

### API 엔드포인트

| 엔드포인트 | 메서드 | 설명 |
|-----------|--------|------|
| `/api/auth/register` | POST | 사용자 등록 |
| `/api/auth/login` | POST | 로그인 및 세션 토큰 발급 |
| `/api/auth/verify` | POST | 세션 토큰 검증 |
| `/api/auth/logout` | POST | 세션 종료 |

### 실행 방법

```bash
cd auth
dotnet run
```

## 🎮 클라이언트

### Unity 클라이언트

Unity 게임 클라이언트는 별도 프로젝트로 분리되어 있으며, 다음 기능들을 제공합니다:
- 로그인/인증 시스템
- 실시간 채팅
- 플레이어 이동 및 상호작용
- 존(Zone) 시스템

### Python 테스트 도구

`Test/` 디렉터리에는 서버 성능 테스트를 위한 Python 스크립트가 포함되어 있습니다:
- 프로토콜 정의 파일들 (C#/Python)
- 1000명 동시 접속 스트레스 테스트

**실행 예시:**
```bash
cd Test
python extreme_stress_test_1000.py
```

## 📡 통신 프로토콜

### FlatBuffers 기반 바이너리 프로토콜

프로젝트는 Google FlatBuffers를 사용하여 효율적인 직렬화를 구현합니다.

**지원하는 패킷 타입:**
- **인증**: `C_Login`, `S_LoginSuccess`, `S_LoginFailure`
- **채팅**: `C_Chat`, `S_Chat`
- **이동**: `C_Move`, `S_PlayerMove`
- **존 관리**: `C_ChangeZone`, `S_EnterZone`, `S_LeaveZone`
- **플레이어 상태**: `S_PlayerHpUpdate`, `S_PlayerEnterZone`, `S_PlayerLeaveZone`

### 프로토콜 정의

모든 패킷은 `src/Common/protocol.fbs`에 정의되어 있으며, 빌드 시점에 자동으로 C++ 및 C# 헤더가 생성됩니다.

## 🧪 테스트

### 스트레스 테스트

Python 테스트 도구를 사용한 대량 동시 접속 테스트:

```bash
# 1000명 극한 스트레스 테스트
cd Test
python extreme_stress_test_1000.py
```

**테스트 구성:**
- `Test/Protocol/`: FlatBuffers 프로토콜 정의 파일 (C#/Python)
- `extreme_stress_test_1000.py`: 1000명 동시 접속 스트레스 테스트 스크립트

## 📊 성능 특성

- **동시 접속자 수**: 1000+ 클라이언트 지원 (테스트 완료)
- **처리량**: 초당 수천 개의 패킷 처리 가능
- **메모리 사용량**: 효율적인 메모리 관리로 장시간 안정 운영
- **네트워크 지연**: 비동기 I/O로 최소 지연 시간 보장

## 🗂️ 프로젝트 구조

```
CppMMO/
├── src/                    # C++ 게임 서버 소스코드
│   ├── Common/            # 공통 프로토콜 정의 (protocol.fbs)
│   ├── Network/           # 네트워크 레이어 (TcpServer, Session, PacketManager)
│   ├── Game/              # 게임 로직 (GameManager, ChatManager, Player, World)
│   └── Utils/             # 유틸리티 클래스 (Logger, JobQueue, JobProcessor)
├── auth/                  # .NET 인증 서버 (ASP.NET Core)
├── Test/                  # Python 테스트 도구
│   ├── Protocol/          # FlatBuffers 프로토콜 정의 (C#/Python)
│   └── extreme_stress_test_1000.py  # 스트레스 테스트 스크립트
├── logs/                  # 로그 파일 저장소
├── docker-compose.yml     # Docker 컨테이너 설정
├── CMakeLists.txt         # CMake 빌드 설정
├── Dockerfile             # 게임 서버 Docker 이미지
├── Doxyfile               # 문서 생성 설정
└── README.md             # 이 파일
```

**주요 디렉터리 설명:**
- `src/`: C++ 게임 서버의 핵심 소스코드
- `auth/`: .NET 기반 인증 서버 (별도 마이크로서비스)
- `Test/`: 서버 성능 테스트를 위한 Python 도구들
- `logs/`: 서버 운영 로그 파일들

## 🚀 향후 계획

*   **핵심 게임 로직 구현:** `Player`, `World`와 같은 핵심 게임 모델을 구체화하고 주요 게임 메카닉을 구현합니다.
*   **설정 파일 도입:** Redis 접속 정보나 기본 포트 같은 설정 값을 외부 파일로 분리합니다.
*   **세션 타임아웃:** 응답 없는 클라이언트가 서버 자원을 계속 점유하는 것을 방지하기 위해 읽기/쓰기 타임아웃을 구현합니다.
*   **데이터베이스 연동 확장:** 플레이어 데이터를 영구적으로 저장하기 위한 데이터베이스 연동을 확장합니다.
*   **테스트 코드 강화:** 주요 컴포넌트에 대한 유닛 테스트 및 통합 테스트를 추가합니다.
*   **로드 밸런싱:** 다중 게임 서버 인스턴스 간 로드 밸런싱 시스템 구현
*   **모니터링 시스템:** 실시간 성능 모니터링 및 로깅 시스템 구축

## 📄 라이센스

이 프로젝트는 MIT 라이센스하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 🤝 기여하기

기여를 환영합니다! 이슈를 등록하거나 풀 리퀘스트를 보내주세요.

1. 이 저장소를 Fork하세요
2. 새로운 기능 브랜치를 생성하세요 (`git checkout -b feature/AmazingFeature`)
3. 변경 사항을 커밋하세요 (`git commit -m 'Add some AmazingFeature'`)
4. 브랜치에 푸시하세요 (`git push origin feature/AmazingFeature`)
5. 풀 리퀘스트를 생성하세요

## 📞 연락처

프로젝트에 대한 질문이나 제안사항이 있으시면 이슈를 통해 연락해주세요.
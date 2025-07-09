# CppMMO: Modern C++ MMORPG Server

C++20과 Boost.Asio, FlatBuffers를 기반으로 제작된 고성능 최신 MMORPG 서버입니다.

## 🌟 주요 특징

*   **Modern C++20:** C++20 표준과 코루틴을 적극적으로 활용하여 가독성 높고 깔끔한 비동기 코드를 구현합니다.
*   **비동기 네트워크:** `Boost.Asio`를 기반으로 확장성 높은 고성능 네트워크 I/O를 처리합니다.
*   **확장 가능한 스레딩 모델:** 네트워크 I/O와 게임 로직 처리를 위한 스레드 풀을 분리하여, 로직의 병목 현상이 네트워크 응답성에 영향을 주지 않도록 설계되었습니다.
*   **효율적인 직렬화:** `Google FlatBuffers`를 사용하여 네트워크 패킷을 Zero-copy로 직렬화함으로써 CPU 부하를 최소화합니다.
*   **Pub/Sub 채팅 시스템:** `Redis`와 연동하여 여러 서버 인스턴스를 지원할 수 있는 확장성 있는 채팅 서비스를 제공합니다.
*   **의존성 관리:** `CMake`와 `vcpkg`를 통해 프로젝트의 외부 라이브러리 의존성을 쉽고 안정적으로 관리합니다.

## 🏗️ 아키텍처

본 서버는 확장성과 유지보수성을 고려하여 각 컴포넌트의 역할이 명확하게 분리된 구조로 설계되었습니다.

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
+-----------+      +-----------------+      +------------------+
|   Redis   |<---->| RedisChatService|<---->|  JobProcessor    |
+-----------+      +-----------------+      | (로직 스레드)    |
                                             +------------------+
```

1.  **TcpServer (I/O 스레드):** 전용 스레드 풀을 사용하여 클라이언트 연결을 관리하고 원시 네트워크 I/O를 처리합니다.
2.  **PacketManager:** 클라이언트로부터 받은 데이터를 검증하고, 로직 스레드에서 처리할 수 있도록 `Job` 형태로 감쌉니다.
3.  **JobQueue:** I/O 스레드와 로직 스레드를 분리하는 스레드 안전 큐(`moodycamel::ConcurrentQueue`)입니다.
4.  **JobProcessor (로직 스레드):** 별도의 스레드 풀을 사용하여 `JobQueue`에서 작업을 가져와 해당 `PacketHandler`를 실행함으로써 실제 게임 로직을 처리합니다.
5.  **RedisChatService:** Redis 서버와 상호작용하여 채팅 시스템을 위한 Pub/Sub 기능을 수행합니다.

## 🛠️ 시작하기

### 요구 사항

*   C++20을 지원하는 컴파일러 (MSVC v143 이상)
*   CMake (3.15 버전 이상)
*   [vcpkg](https://github.com/microsoft/vcpkg) (의존성 관리)
*   채팅 기능 사용을 위한 Redis 서버

### 의존성 라이브러리

아래 라이브러리들은 `vcpkg`와 `CMake FetchContent`를 통해 자동으로 관리됩니다.
*   Boost (System, Thread, Program_options, Filesystem)
*   spdlog
*   redis-plus-plus
*   FlatBuffers
*   concurrentqueue

### 빌드 방법

1.  **저장소 복제:**
    ```bash
    git clone <your-repo-url>
    cd CppMMO
    ```

2.  **CMake 설정:**
    vcpkg toolchain 파일의 경로를 지정하여 CMake 프로젝트를 설정합니다.
    ```bash
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="<vcpkg-경로>/scripts/buildsystems/vcpkg.cmake"
    ```

3.  **프로젝트 빌드:**
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

## 🚀 향후 계획

*   **게임 로직 구현:** `Player`, `World`와 같은 핵심 게임 모델을 구체화하고 주요 게임 메카닉을 구현합니다.
*   **설정 파일 도입:** Redis 접속 정보나 기본 포트 같은 설정 값을 외부 파일(`.json`, `.ini` 등)로 분리합니다.
*   **세션 타임아웃:** 응답 없는 클라이언트가 서버 자원을 계속 점유하는 것을 방지하기 위해 읽기/쓰기 타임아웃을 구현합니다.
*   **데이터베이스 연동:** 플레이어 데이터를 영구적으로 저장하기 위한 데이터베이스를 연동합니다.
*   **테스트 코드 강화:** 주요 컴포넌트에 대한 유닛 테스트 및 통합 테스트를 추가합니다.

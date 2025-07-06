# CppMMO

## 프로젝트 개요

CppMMO는 C++로 개발되는 대규모 멀티플레이어 온라인(MMO) 게임 서버 프로젝트입니다. 이 프로젝트는 고성능, 확장성 및 안정성을 목표로 하며, `boost::asio`를 활용한 비동기 네트워크 통신과 워커 스레드 풀 기반의 게임 로직 처리를 핵심 아키텍처로 채택하고 있습니다.

## 주요 기능

*   **비동기 네트워크 I/O:** `boost::asio`를 사용하여 효율적인 클라이언트 연결 관리 및 데이터 송수신을 처리합니다.
*   **세션 관리:** 각 클라이언트 연결을 나타내는 `ISession` 인터페이스 및 `Session` 구현체를 통해 통신을 추상화합니다.
*   **패킷 관리:** `IPacketManager`를 통해 수신된 원시 데이터를 게임 로직이 처리할 수 있는 의미 있는 패킷으로 파싱합니다.
*   **워커 스레드 풀:** 네트워크 I/O 스레드와 분리된 별도의 스레드 풀에서 복잡한 게임 로직을 비동기적으로 처리하여 서버의 반응성과 처리량을 극대화합니다.
*   **모듈화된 아키텍처:** 각 컴포넌트(네트워크, 패킷, 스레드 등)가 명확한 책임을 가지도록 설계하여 유지보수 및 확장이 용이합니다.
*   **스레드 안전한 큐:** `moodycamel/concurrentqueue`를 사용하여 세션의 쓰기 큐(write queue)에 대한 스레드 안전한 접근을 보장하고 성능을 최적화합니다.

## 빌드 방법

이 프로젝트는 CMake를 사용하여 빌드됩니다. 외부 의존성 관리를 위해 `vcpkg`와 CMake의 `FetchContent` 모듈을 활용합니다.

1.  **vcpkg 설치 및 설정:**
    프로젝트의 `CMakeLists.txt`는 `vcpkg` 툴체인 파일을 사용하도록 설정되어 있습니다. `vcpkg`가 설치되어 있지 않다면 다음 링크를 참조하여 설치하십시오: [vcpkg GitHub](https://github.com/microsoft/vcpkg)
    설치 후, `vcpkg.cmake` 파일의 경로를 `CMAKE_TOOLCHAIN_FILE` 환경 변수 또는 CMake 구성 시 `-DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg.cmake>` 옵션으로 지정해야 합니다.

2.  **프로젝트 클론:**
    ```bash
    git clone https://github.com/ArsnDev/CppMMO.git
    cd CppMMO
    ```
3.  **빌드 디렉토리 생성 및 이동:**
    ```bash
    mkdir out
    cd out
    ```
4.  **CMake 구성:**
    ```bash
    cmake .. -DCMAKE_TOOLCHAIN_FILE=<path_to_your_vcpkg_installation>/scripts/buildsystems/vcpkg.cmake
    ```
    (예: `cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/Users/clums/Desktop/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake`)

5.  **프로젝트 빌드:**
    ```bash
    cmake --build .
    ```

## Doxygen 문서

프로젝트의 코드 문서화는 Doxygen을 사용하여 생성됩니다.

*   **로컬 문서 경로:** `out/docs/html/index.html` (Doxygen 설정에 따라 `docs` 디렉토리는 변경될 수 있습니다.)

## 라이선스

이 프로젝트는 [라이선스 타입, 예: MIT License]에 따라 라이선스가 부여됩니다. 자세한 내용은 `LICENSE` 파일을 참조하십시오.